#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rods.h"
#include "rcMisc.h"

//' r_iexit
//' Performs the iRODS session logout
//' @param full Optional parameter 'full' will terminate the iRODS session, default "FALSE"
//'
// [[Rcpp::export]]
void iexit( bool full=false ) {

    rodsArguments_t myRodsArgs;
    rodsEnv myEnv;
    char *envFile;
    int status = 0;

    status = getRodsEnv( &myEnv );

    envFile = getRodsEnvFileName();
    status = unlink( envFile );
    if ( myRodsArgs.verbose == True ) {
        printf( "Deleting (if it exists) session envFile:%s\n", envFile );
        printf( "unlink status = %d\n", status );
    }

    if ( full ) {
        obfRmPw( 1 );
    }

}
