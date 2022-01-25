#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * iget - The irods get utility
*/

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "getUtil.h"
#include "miscUtil.h"
#include "rcGlobalExtern.h"
#include "irods_client_api_table.hpp"
#include "irods_pack_table.hpp"
#include "irods_parse_command_line_options.hpp"

#define VERIFY_DIV(_v1_,_v2_) ((_v2_)? (float)(_v1_)/(_v2_):0.0)

#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

#include <string>
#include <vector>

/* iCommandProgStat - the guiProgressCallback for icommand */
guiProgressCallback
r_get_iCommandProgStat( operProgress_t *operProgress ) {
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
        Rf_error( "iCommandProgStat: splitPathByKey for %s error, status = %d",
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

// From irods_parse_command_line_options.cpp
typedef std::vector< std::string > path_list_t;

//' iget
//' Retrieves the file from iRODS
//' @param src_path iRODS source file path
//' @param dest_path local destination file path
//' @param force force - write local files even it they exist already (overwrite them)
//' @param checksum verify the checksum
//' @param progress output the progress of the download
//' @param verbose verbose
//'
std::string iget( std::string src_path
		, std::string dest_path
		, bool force = false
		, bool checksum = false
		, bool progress = false
		, bool verbose = false
		)
{
    int status;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    int reconnFlag;

    // We have to reset all to 0s because we do not use
    // the "parse_program_options(...)" function
    // The getUtil(...) function expects positional arguments parser initializing it beforehand
    // Legacy of the iget icommand logic
    memset( &myRodsArgs, 0, sizeof( rodsArguments_t ) );

   	boost::shared_ptr<rodsPathInp_t> rodsPathInp( new rodsPathInp_t());

    rodsEnv myEnv;
    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
      Rf_error("Error iget: getRodsEnv error %d ", status );
    }

    if(src_path.empty()) {
      Rf_error("Error iget: srcPath is empty!" );
    }

    if(dest_path.empty()) {
      Rf_error("Error iget: destPath is empty!" );
    }

    if( force )
    	myRodsArgs.force = 1;
    if( checksum )
    	myRodsArgs.verifyChecksum = 1;
    if( progress )
    	myRodsArgs.progressFlag = 1;
    if( verbose )
    	myRodsArgs.verbose = 1;

    // Reset
    memset( (void*)(rodsPathInp.get()), 0, sizeof( rodsPathInp_t ) );

    // Validate src_path user input against iRODS server
    status = 0;
    rodsPath_t r_path;
    memset( &r_path, 0, sizeof( rodsPath_t ) );

    status = addSrcInPath((rodsPathInp_t*)(rodsPathInp.get()), src_path.c_str());
    status = parseRodsPath( &(rodsPathInp->srcPath[0]), &myEnv );
    if( status < 0 ) {
      Rf_error("rods Error: %d : invalid iRODS src_path %s!\n", status, rodsPathInp->srcPath[0]);
    }

    // Validate dest_path user input
    status = 0;
    rodsPath_t r_d_path;
    memset( &r_d_path, 0, sizeof( rodsPath_t ) );

    rodsPathInp->destPath = ( rodsPath_t* )malloc( sizeof( rodsPath_t ) );
    memset( rodsPathInp->destPath, 0, sizeof( rodsPath_t ) );
    strncpy (rodsPathInp->destPath->inPath, dest_path.c_str(), MAX_NAME_LEN);

	  status = parseLocalPath( rodsPathInp->destPath );
    if( status < 0 ) {
      Rf_error("rods Error: %d : invalid iRODS dest_path %s!\n", status, rodsPathInp->destPath->inPath);
    }

    if ( myRodsArgs.reconnect == True ) {
        reconnFlag = RECONN_TIMEOUT;
    }
    else {
        reconnFlag = NO_RECONN;
    }

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName,
                      myEnv.rodsZone, reconnFlag, &errMsg );

    if ( conn == NULL ) {
      Rf_error("iget: Connection error");
    }

    if ( strcmp( myEnv.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
          Rf_error("iget: Authentication error");
        }
    }

    if ( myRodsArgs.progressFlag == True ) {
        gGuiProgressCB = ( guiProgressCallback ) r_get_iCommandProgStat;
    }

    status = getUtil( &conn, &myEnv, &myRodsArgs, (rodsPathInp_t*)(rodsPathInp.get()) );


    rcDisconnect( conn );

    if ( status < 0 ) {
      free(rodsPathInp->destPath);
      printErrorStack( conn->rError );
      Rf_error( "iget: Error %d", status );
    }
    else {
      std::string destination_path(rodsPathInp->destPath->outPath);
      if(rodsPathInp->destPath->objType == LOCAL_DIR_T){
        char dataObjName[MAX_NAME_LEN];
        char collectionName[MAX_NAME_LEN];
        splitPathByKey( rodsPathInp->srcPath->outPath, collectionName, MAX_NAME_LEN, dataObjName, MAX_NAME_LEN, '/' );
        destination_path += '/';
        destination_path += dataObjName;
      }
      free(rodsPathInp->destPath);
      return destination_path;
    }

}
