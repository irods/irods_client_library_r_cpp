/*
 * r_ipwd.cpp
 *
 *  Created on: Dec 30, 2015
 *      Author: radovan.chytracek@rd.nestle.com
 */

#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
#include "rods.h"
#include "parseCommandLine.h"
#include "rcMisc.h"

//' ipwd
//' Returns the current working directory in iRODS or empty string if error
//'
// [[Rcpp::export]]
std::string ipwd()
{
    int status;
    rodsEnv myEnv;

    status = getRodsEnv( &myEnv );
    if ( status != 0 ) {
        Rf_error( "Failed with error %d\n", status );
        return "";
    }

//    Rprintf( "%s\n", myEnv.rodsCwd );

    return myEnv.rodsCwd;
}


