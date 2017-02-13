#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*
 * isearch - Customized gen query for File ID card
*/

#include "rodsClient.h"
#include "rodsPath.h"
#include "rodsType.h"
//#include "r_lsUtil.h"

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "ls_query.h"


//' isearch
//' Lists collections and data objects in iRODS
//' @param args "isearch" icommand params
//'
Rcpp::DataFrame isearch(Rcpp::DataFrame constraints){
    if ( constraints.nrows()==0){
        ::Rf_error( "Error: at least one attribute value (filter) must be given");
    }

    CharacterVector constraintAttributes  = constraints["Attribute"];
    CharacterVector constraintValues      = constraints["Value"];


    int status;
    rodsEnv env;
    rErrMsg_t errMsg;
    rcComm_t *conn;

    status = getRodsEnv( &env );
    if ( status < 0 ) {
        ::Rf_error( "ils: getRodsEnv error %d", status );
    }

    conn = rcConnect(
               env.rodsHost,
               env.rodsPort,
               env.rodsUserName,
               env.rodsZone,
               0, &errMsg );

    if ( conn == NULL ) {
        ::Rf_error( "ils: Connection error %d", status );
    }

    if ( strcmp( env.rodsUserName, PUBLIC_USER_NAME ) != 0 ) {
        status = clientLogin( conn );
        if ( status != 0 ) {
            rcDisconnect( conn );
            ::Rf_error( "ils: Conection error %d", status );
        }
    }

    LsOutput lsOutput(&env, conn);

    char condStr[MAX_NAME_LEN];

    genQueryInp_t genQueryInp;
    genQueryOut_t* genQueryOutP = 0;

    memset(&genQueryInp, 0, sizeof( genQueryInp_t ) );
    genQueryInp.maxRows = MAX_SQL_ROWS_RIRODS;

    for (int i = 0; i<constraints.nrows(); ++i){
        std::ostringstream attributes;
        std::ostringstream values;
        /*
        std::vector<std::string> value_list = Rcpp::as<std::vector<std::string> >(constraints["values"][i]);
        for(std::vector<std::string>::iterator itr=value_list.begin(); itr != value_list.end(); ++itr){
            if (itr!=value_list.begin()){
                values << ",";
            }
            values << "'" << *itr << "'";
        }
        */
        attributes << "='" << constraintAttributes[i] << "'";
        if (((char*)constraintValues[i])[0]!='\''){
          values    << "in('" << constraintValues[i] << "')";
        } else{
          values    << "in(" << constraintValues[i] << ')';
        }
        Rprintf("Constraint: \"WHERE META_DATA_ATTR_NAME %s AND META_DATA_ATTR_VALUE %s\"\n", attributes.str().c_str(), values.str().c_str());

        snprintf( condStr, MAX_NAME_LEN, "%s", attributes.str().c_str());
        addInxVal( &genQueryInp.sqlCondInp, COL_META_DATA_ATTR_NAME, condStr );
        snprintf( condStr, MAX_NAME_LEN, "%s", values.str().c_str() );
        addInxVal( &genQueryInp.sqlCondInp, COL_META_DATA_ATTR_VALUE, condStr );
    }

    lsOutput.set_query_output_for_data_objects(genQueryInp);

    status = rcGenQuery(conn, &genQueryInp, &genQueryOutP);
    if ( status < 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            ::Rf_error("rirods exception\n");
        }
    }

    for ( int i = 0; i < genQueryOutP->rowCnt; i++ ) {
        lsOutput.process_query_line_for_data_objects(genQueryOutP, i);
    }
    free (genQueryOutP);

    rcDisconnect( conn );
    return lsOutput.toDataFrame();
}


