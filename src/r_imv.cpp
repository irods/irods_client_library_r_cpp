/*
 * r_imv.cpp
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
 * imv - The irods mv utility
*/

#include <string>
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "mvUtil.h"

//' imv
//' imv moves/renames an irods data-object (file) or collection (directory) to
//' another, data-object or collection. The move works if both the source and
//' target are normal (registered in the iCAT) iRODS objects. It also works when
//' the source object is in a mounted collection (object not registered in the
//' iCAT) and the target is a normal object. In fact, this may provide a way
//' to design a drop box where data can be uploaded quickly into a mounted
//' collection and then in the background, moved to the eventual target
//' collection (where data are registered in the iCAT). But currently, the
//' move from a normal collection to a mounted collection is not supported.
//'
//' If you do a move and rename at the same time, for example,
//' 'imv file1 coll1/file2', it will normally succeed if there's no conflicting
//' data-object name in the source collection (file2) but fail (giving error
//' CAT_NAME_EXISTS_AS_DATAOBJ) if there is, since, internally IRODS is doing
//' a rename and then a move. Please handle this by running multiple separate
//' 'imv' commands.
//'
//' @param src_path Local source file path
//' @param dest_path iRODS destination file path
//' @param verbose verbose
//'
// [[Rcpp::export]]
std::string imv(
		  std::string src_path
		, std::string dest_path
		, bool verbose = false
		)
{
    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    rodsPathInp_t rodsPathInp;

    // Initialize the iRODS input path object
    memset( &rodsPathInp, 0, sizeof( rodsPathInp_t ) );

    // We have to reset all to 0s because we do not use
    // the "parse_program_options(...)" function
    // The mvUtil(...) function expects positional arguments parser initializing it beforehand
    memset( &myRodsArgs, 0, sizeof( rodsArguments_t ) );

    // Process input parameters
    if(src_path.empty()) {
      ::Rf_error( "Error iget: srcPath is empty!\n" );
    }
    if(dest_path.empty()) {
      ::Rf_error( "Error iget: destPath is empty!\n" );
    }
    if( verbose )
    	myRodsArgs.verbose = 1;

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
      ::Rf_error( "[imv] getRodsEnv error %d.\n", status );
    }

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName, myEnv.rodsZone, 1, &errMsg );

    if ( conn == NULL ) {
      ::Rf_error("[imv] Cannot connect\n");
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
      ::Rf_error("[imv] Connection failed with status %d\n", status);
    }

    // Validate src_path user input against iRODS server
    status = 0;
    rodsPath_t r_path;
    memset( &r_path, 0, sizeof( rodsPath_t ) );

    status = addSrcInPath((rodsPathInp_t*)(&rodsPathInp), src_path.c_str());
    status = parseRodsPath( &(rodsPathInp.srcPath[0]), &myEnv );
    if( status < 0 ) {
      ::Rf_error("[imv] Error: %d : invalid iRODS src_path %s!\n", status, rodsPathInp.srcPath[0]);
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
      ::Rf_error("[imv] Error: %d : invalid iRODS dest_path %s!\n", status, rodsPathInp.destPath->inPath);
    }

    status = mvUtil( conn, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
      ::Rf_error("[imv] Failed with status %d\n");
    }

    return( std::string(rodsPathInp.destPath->outPath) );
}
