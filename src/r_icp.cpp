/*
 * r_icp.cpp
 *
 *  Created on: Jan 18, 2016
 *      Author: radovan.chytracek@rd.nestle.com
 */

#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * icp - The irods cp utility
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "cpUtil.h"
#include "rcGlobalExtern.h"

#define VERIFY_DIV(_v1_,_v2_) ((_v2_)? (float)(_v1_)/(_v2_):0.0)

#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

/* iCommandProgStat - the guiProgressCallback for icommand */
guiProgressCallback
r_cp_iCommandProgStat( operProgress_t *operProgress ) {
    using namespace boost::filesystem;
    char myDir[MAX_NAME_LEN], myFile[MAX_NAME_LEN];
    int status;
    time_t myTime;
    struct tm *mytm;
    char timeStr[TIME_LEN];

    if ( strchr( operProgress->curFileName, '/' ) == NULL ) {
        /* relative path */
        rstrcpy( myFile, operProgress->curFileName, MAX_NAME_LEN );
    }
    else if ( ( status = splitPathByKey( operProgress->curFileName,
                                         myDir, MAX_NAME_LEN, myFile, MAX_NAME_LEN, '/' ) ) < 0 ) {
        ::Rf_error( "iCommandProgStat: splitPathByKey for %s error, status = %d",
                  operProgress->curFileName, status );
        return NULL;
    }

    myTime = time( 0 );
    mytm = localtime( &myTime );
    getLocalTimeStr( mytm, timeStr );
    if ( operProgress->flag == 0 ) {
        Rprintf(
            "%-lld/%-lld - %5.2f%% of files done   ",
            operProgress->totalNumFilesDone, operProgress->totalNumFiles,
            VERIFY_DIV
            ( operProgress->totalNumFilesDone, operProgress->totalNumFiles ) *
            100.0 );
        Rprintf( "%-.3f/%-.3f MB - %5.2f%% of file sizes done\n",
                ( float ) operProgress->totalFileSizeDone / 1048600.0,
                ( float ) operProgress->totalFileSize / 1048600.0,
                VERIFY_DIV
                ( operProgress->totalFileSizeDone, operProgress->totalFileSize ) *
                100.0 );
        Rprintf( "Processing %s - %-.3f MB   %s\n", myFile,
                ( float ) operProgress->curFileSize / 1048600.0, timeStr );
    }
    else if ( operProgress->flag == 1 ) {
        Rprintf( "%s - %-.3f/%-.3f MB - %5.2f%% done   %s\n", myFile,
                ( float ) operProgress->curFileSizeDone / 1048600.0,
                ( float ) operProgress->curFileSize / 1048600.0,
                VERIFY_DIV
                ( operProgress->curFileSizeDone, operProgress->curFileSize ) *
                100.0, timeStr );
        /* done. don't print again */
        if ( operProgress->curFileSizeDone == operProgress->curFileSize ) {
            operProgress->flag = 2;
        }
    }

    return NULL;
}


//' icp
//' Copies files on iRODS server. This implementation does not allow to user physical path nor Resource server option.
//' @param src_path Local source file path
//' @param dest_path iRODS destination file path
//' @param force write data-object even it exists already; overwrite it
//' @param calculate_checksum calculate a checksum on the data server-side, and store it in the catalog
//' @param checksum verify checksum - calculate and verify the checksum on the data,
//'                 both client-side and server-side, and store it in the catalog
//' @param progress output the progress of the download
//' @param verbose verbose
//'
// [[Rcpp::export]]
std::string icp(
		  std::string src_path
		, std::string dest_path
		, bool force = false
		, bool calculate_checksum = false
		, bool checksum = false
		, bool progress = false
		, bool verbose = false
		)
{
    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    rodsPathInp_t rodsPathInp;
    int reconnFlag;

    // Initialize the iRODS input path object
    memset( &rodsPathInp, 0, sizeof( rodsPathInp_t ) );

    // We have to reset all to 0s because we do not use
    // the "parse_program_options(...)" function
    // The putUtil(...) function expects positional arguments parser initializing it beforehand
    memset( &myRodsArgs, 0, sizeof( rodsArguments_t ) );

    // Process input parameters
    if(src_path.empty()) {
      ::Rf_error( "Error iget: srcPath is empty!" );
    }
    if(dest_path.empty()) {
      ::Rf_error( "Error iget: destPath is empty!" );
    }
    if( force )
    	myRodsArgs.force = 1;
    if( calculate_checksum )
    	myRodsArgs.checksum = 1;
    if( checksum )
    	myRodsArgs.verifyChecksum = 1;
    if( progress )
    	myRodsArgs.progressFlag = 1;
    if( verbose )
    	myRodsArgs.verbose = 1;

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
      ::Rf_error( "[icp] getRodsEnv error  %d.\n", status );
    }

    if ( myRodsArgs.reconnect == True ) {
        reconnFlag = RECONN_TIMEOUT;
    }
    else {
        reconnFlag = NO_RECONN;
    }

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName, myEnv.rodsZone, reconnFlag, &errMsg );

    if ( conn == NULL ) {
      ::Rf_error( "[icp] Cannot connect" );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
      rcDisconnect( conn );
      ::Rf_error( "[icd] Conection error %d", status );
    }

    if ( myRodsArgs.progressFlag == True ) {
        gGuiProgressCB = ( guiProgressCallback ) r_cp_iCommandProgStat;
    }

    // Validate src_path user input against iRODS server
    status = 0;
    rodsPath_t r_path;
    memset( &r_path, 0, sizeof( rodsPath_t ) );

    status = addSrcInPath((rodsPathInp_t*)(&rodsPathInp), src_path.c_str());
    status = parseRodsPath( &(rodsPathInp.srcPath[0]), &myEnv );
    if( status < 0 ) {
      ::Rf_error("[icp] Error: %d : invalid iRODS src_path %s!\n", status, rodsPathInp.srcPath[0]);
    }

    // Validate dest_path user input
    status = 0;
    rodsPath_t r_d_path;
    memset( &r_d_path, 0, sizeof( rodsPath_t ) );

    rodsPathInp.destPath = ( rodsPath_t* )malloc( sizeof( rodsPath_t ) );
    memset( rodsPathInp.destPath, 0, sizeof( rodsPath_t ) );
    strncpy (rodsPathInp.destPath->inPath, dest_path.c_str(), MAX_NAME_LEN);

	status = parseRodsPath( rodsPathInp.destPath, &myEnv );
    if( status < 0 ) {
      ::Rf_error("[icp] Error: %d : invalid iRODS dest_path %s!\n", status, rodsPathInp.destPath->inPath);
    }

    status = cpUtil( conn, &myEnv, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
      ::Rf_error("[icp] Error: %d\n", status);
    }
    else {
        return( std::string(rodsPathInp.destPath->outPath) );
    }
}
