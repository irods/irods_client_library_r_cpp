/*
 * r_irm.cpp
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
 * irm - The irods rm utility
 */

#include "rodsClient.h"
#include "parseCommandLine.h"
#include "rodsPath.h"
#include "rmUtil.h"

// rmUtil code replicated here to raise R exception instead of print to log
int r_rmUtil( rcComm_t *conn, rodsArguments_t *myRodsArgs,
        rodsPathInp_t *rodsPathInp ) {
    if ( rodsPathInp == NULL ) {
        return USER__NULL_INPUT_ERR;
    }

    collInp_t collInp;
    dataObjInp_t dataObjInp;
    initCondForRm( myRodsArgs, &dataObjInp, &collInp );

    int savedStatus = 0;
    for ( int i = 0; i < rodsPathInp->numSrc; i++ ) {
        if ( rodsPathInp->srcPath[i].objType == UNKNOWN_OBJ_T ) {
            getRodsObjType( conn, &rodsPathInp->srcPath[i] );
            if ( rodsPathInp->srcPath[i].objState == NOT_EXIST_ST ) {
                ::Rf_error("[irm] r_rmUtil: srcPath '%s' does not exist.",
                           rodsPathInp->srcPath[i].outPath );
                savedStatus = USER_INPUT_PATH_ERR;
                continue;
            }
        }

        int status = 0;
        if ( rodsPathInp->srcPath[i].objType == DATA_OBJ_T ) {
            status = rmDataObjUtil( conn, rodsPathInp->srcPath[i].outPath,
                                    myRodsArgs, &dataObjInp );
        }
        else if ( rodsPathInp->srcPath[i].objType ==  COLL_OBJ_T ) {
            status = rmCollUtil( conn, rodsPathInp->srcPath[i].outPath,
                                 myRodsArgs, &collInp );
        }
        else {
            /* should not be here */
            rodsLog( LOG_ERROR,
                     "rmUtil: invalid rm objType %d for %s",
                     rodsPathInp->srcPath[i].objType, rodsPathInp->srcPath[i].outPath );
            return USER_INPUT_PATH_ERR;
        }
        /* XXXX may need to return a global status */
        if ( status < 0 &&
                status != CAT_NO_ROWS_FOUND ) {
          ::Rf_error("[irm] r_rmUtil: rm error for %s, status = %d",
                     rodsPathInp->srcPath[i].outPath, status );
            savedStatus = status;
        }
    }
    return savedStatus;
}



//' imkdir
//' Creates a new directory in iRODS
//' @param rods_path The iRODS new directory path
//' @param recursive Remove all recursively
//' @param verbose   Verbose output
//' @param force     Skip trash bin, directly delete the iRODS object(s)
//'
// [[Rcpp::export]]
int irm( std::string rods_path="", bool recursive=false, bool verbose=false, bool force=false )
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
    // The rmUtil(...) function expects positional arguments parser initializing it beforehand
    memset( &myRodsArgs, 0, sizeof( rodsArguments_t ) );

    if ( rods_path.empty() )
    {
        ::Rf_error( "[irm] no rods_path input\n" );
        return( 2 );
    }
    if(verbose)
    	myRodsArgs.verbose = 1;
    if(recursive)
    	myRodsArgs.recursive = 1;
    if(force)
    	myRodsArgs.force = 1;

    status = getRodsEnv( &myEnv );

    if ( status < 0 ) {
    	::Rf_error( "[irm] getRodsEnv error %d. ", status );
        return( 1 );
    }

    conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName, myEnv.rodsZone, 1, &errMsg );

    if ( conn == NULL ) {
        exit( 2 );
    }

    status = clientLogin( conn );
    if ( status != 0 ) {
        rcDisconnect( conn );
        exit( 7 );
    }

    // Prepare the path for rmUtil(...)
    status = 0;
    rodsPath_t r_path;
    memset( &r_path, 0, sizeof( rodsPath_t ) );

    status = addSrcInPath((rodsPathInp_t*)(&rodsPathInp), rods_path.c_str());
    status = parseRodsPath( &(rodsPathInp.srcPath[0]), &myEnv );
    if( status < 0 ) {
    	::Rf_error("[irm] Error: %d : invalid iRODS src_path %s!\n", status, rodsPathInp.srcPath[0]);
    	return status;
    }

    status = r_rmUtil( conn, &myRodsArgs, &rodsPathInp );

    printErrorStack( conn->rError );
    rcDisconnect( conn );

    if ( ( USER_SOCK_CONNECT_ERR - 1000 ) < status && status <= USER_SOCK_CONNECT_ERR ) {
        ::Rf_error( "Remote resource may be unavailable %d.\n", status );
    }

    if ( status < 0 ) {
        return( 3 );
    }
    else {
        return( 0 );
    }

}

