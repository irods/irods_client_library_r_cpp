/*
 * r_icd.cpp
 *
 *  Created on: Jan 15, 2016
 *      Author: Radovan Chytracek
 */

#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include <fcntl.h>
#include <string>
#include "rods.h"
#include "rodsPath.h"
#include "miscUtil.h"
#include "rcMisc.h"
#include "genQuery.h"
#include "rodsClient.h"

//' icd
//' Changes the current working directory in iRODS or home if empty
//' @param rods_path The iRODS destination path to cd into
//'
// [[Rcpp::export]]
std::string icd( std::string rods_path="", bool verbose=false)
{
    int status, i, fd, len;
    rodsArguments_t myRodsArgs;
    rodsEnv myEnv;
    char *envFile;
    char buffer[MAX_NAME_LEN];
    rodsPath_t rodsPath;
    rcComm_t *Conn;
    rErrMsg_t errMsg;

    // We have to reset all to 0s because we do not use
    // the "parse_program_options(...)" function
    // The mvUtil(...) function expects positional arguments parser initializing it beforehand
    memset( &myRodsArgs, 0, sizeof( rodsArguments_t ) );

    if(verbose)
    	myRodsArgs.verbose = 1;

    status = getRodsEnv( &myEnv );
    envFile = getRodsEnvFileName();

    if(rods_path.empty())
    	rods_path = myEnv.rodsHome;

//    /* Just "icd", so cd to home, so just remove the session file */
//    /* (can do this for now, since session has only the cwd). */
//    if ( rods_path.empty() )
//    {
//        status = unlink( envFile );
//        if ( myRodsArgs.verbose == True )
//        {
//            Rprintf( "Deleting (if it exists) session envFile:%s\n", envFile );
//            Rprintf( "unlink status = %d\n", status );
//        }
//        return( 0 );
//    }

    memset( ( char* )&rodsPath, 0, sizeof( rodsPath ) );
    rstrcpy( rodsPath.inPath, rods_path.c_str(), MAX_NAME_LEN );
    parseRodsPath( &rodsPath, &myEnv );

    /* Connect and check that the path exists */
    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName, myEnv.rodsZone, 0, &errMsg );
    if ( Conn == NULL ) {
      Rf_error( "ils: Connection error");
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
      rcDisconnect( Conn );
      Rf_error( "ils: Connection error %d", status);
    }

    status = getRodsObjType( Conn, &rodsPath );
    printErrorStack( Conn->rError );
    rcDisconnect( Conn );

    if ( status < 0 ) {
      Rf_error( "[icd] Error %d getting type", status );
    }

    if ( rodsPath.objType != COLL_OBJ_T || rodsPath.objState != EXIST_ST ) {
      Rf_error( "[icd] No such directory (collection): %s\n", rodsPath.outPath );
    }


    /* open the sessionfile and write or update it */
    if ( ( fd = open( envFile, O_CREAT | O_RDWR | O_TRUNC, 0644 ) ) < 0 ) {
      Rf_error( "[icd] Unable to open envFile %s\n", envFile );
    }

    snprintf( buffer, sizeof( buffer ), "{\n\"irods_cwd\": \"%s\"\n}\n", rodsPath.outPath );
    len = strlen( buffer );
    i = write( fd, buffer, len );
    close( fd );
    if ( i != len ) {
      Rf_error( "[icd] Unable to write envFile %s\n", envFile );
    }

    // iRODS environment has been updated so we need to reload for the other functions called later
    _reloadRodsEnv( myEnv );

    return std::string(rodsPath.outPath);
}

/* Check to see if a collection exists */
int
checkColl( rcComm_t *Conn, char *path ) {
    genQueryInp_t genQueryInp;
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[2];
    char v1[MAX_NAME_LEN];
    int status;

    memset( &genQueryInp, 0, sizeof( genQueryInp_t ) );

    i1a[0] = COL_COLL_ID;
    i1b[0] = 0; /* currently unused */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_COLL_NAME;
    genQueryInp.sqlCondInp.inx = i2a;
    sprintf( v1, "='%s'", path );
    condVal[0] = v1;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    return status;
}



