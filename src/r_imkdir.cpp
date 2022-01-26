/*
 * r_imkdir.cpp
 *
 *  Created on: Jan 15, 2016
 *      Author: Radovan Chytracek
 */

#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * imkdir - The irods mkdir utility
*/

#include <string>
#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "mkdirUtil.h"

//' imkdir
//' Creates a new directory in iRODS
//' @param rods_path The iRODS new directory path
//' @param parents   Create automatically all required parent directories
//'
// [[Rcpp::export]]
std::string imkdir( std::string rods_path="", bool parents=false )
{
    int status;
    rodsEnv myEnv;
    rErrMsg_t errMsg;
    rcComm_t *conn;
    rodsArguments_t myRodsArgs;
    rodsPathInp_t rodsPathInp;

    // Process input parameters
    if(rods_path.empty()) {
      Rf_error("[imkdir] Error rods_path is empty!\n" );
    }

    // We have to reset all to 0s because we do not use
    // the "parse_program_options(...)" function
    // The mvUtil(...) function expects positional arguments parser initializing it beforehand
    memset( &myRodsArgs, 0, sizeof( rodsArguments_t ) );

    if( parents ) // Create automatically the required parent directories
    	myRodsArgs.physicalPath = 1;



    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
      Rf_error("[imkdir] getRodsEnv error %d. \n", status );
    }

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName, myEnv.rodsZone, 0, &errMsg );

    if ( conn == NULL ) {
      Rf_error("ils: Connection error %d", status );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
        Rf_error( "ils: Connection error %d", status );
    }

    // Prepare the directory path
    status = 0;
    memset( &rodsPathInp, 0, sizeof( rodsPathInp_t ) );

    status = addSrcInPath((rodsPathInp_t*)(&rodsPathInp), rods_path.c_str());
    status = parseRodsPath( &(rodsPathInp.srcPath[0]), &myEnv );
    if( status < 0 ) {
      Rf_error("[imkdir] Error: %d : invalid iRODS src_path %s!\n", status, rodsPathInp.srcPath[0]);
    }

    status = mkdirUtil( conn, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( status < 0 ) {
      Rf_error("[imkdir] Error: %d", status);
    }

    return(std::string(rodsPathInp.srcPath->outPath));
}
