#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*
 * ils - The irods ls utility
*/

#include "rodsClient.h"
//#include "parseCommandLine.h"
#include "rodsPath.h"
#include "rodsType.h"
//#include "r_lsUtil.h"


//#include "irods_buffer_encryption.hpp"
//#include "irods_client_api_table.hpp"
//#include "irods_pack_table.hpp"

#include <vector>
#include <string>
#include <iostream>

#include "ls_query.h"

//' ils
//' Lists collections and data objects in iRODS
//' @param args "ils" one or several iRODS data-object (file) or collection (directory) paths
//' @return A data frame with information about collections and data objects under the supplied paths
Rcpp::DataFrame ils( std::vector< std::string > args )
{
    int status;
    rodsEnv env;
    rErrMsg_t errMsg;
    rcComm_t *conn;

    status = getRodsEnv( &env );
    if ( status < 0 ) {
        Rf_error( "ils: getRodsEnv error %d", status );
    }

    conn = rcConnect(
               env.rodsHost,
               env.rodsPort,
               env.rodsUserName,
               env.rodsZone,
               0, &errMsg );

    if ( conn == NULL ) {
        Rf_error( "ils: Cannotconnect" );
    }

    if ( strcmp( env.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            Rf_error( "ils: Conection error %d", status );
        }
    }

    LsOutput lsOutput(&env, conn);

    for (std::vector< std::string >::iterator itr = args.begin(); itr != args.end(); ++itr){
        // Process path
        rodsPath_t rodsPath;
        strcpy (rodsPath.inPath, itr->c_str());

        status = parseRodsPath(&rodsPath, &env);
        if ( status != 0 ) {
            Rf_error( "Path parsing error for \"%s\" (%i)\n", itr->c_str(), status);
            continue;
        }

        status = getRodsObjType(conn, &rodsPath);

        if ( status != EXIST_ST ) {
            Rf_error( "Irods object does not exist \"%s\" (%i)\n", itr->c_str(), status);
            continue;
        }

        switch(rodsPath.objType){
            case COLL_OBJ_T:
                lsOutput.process_collection(rodsPath.outPath);
                break;
            case DATA_OBJ_T:
                lsOutput.process_data_object(rodsPath.outPath);
                break;
            default:
                Rf_error("Unsupported objType for path \"%s\" (%i)\n", itr->c_str(), rodsPath.objType);
                continue;
        }
    }

    rcDisconnect( conn );
    return lsOutput.toDataFrame();
}
