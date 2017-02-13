#ifndef R_LS_QUERY_H__
#define R_LS_QUERY_H__

#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

#define MAX_SQL_ROWS_RIRODS 100000

/*
 * ils - The irods ls utility
*/

#include "rodsClient.h"
#include "rodsPath.h"
#include "rodsType.h"
//#include "r_lsUtil.h"


#include <vector>
#include <string>
#include <iostream>

class LsOutput{
    protected:
    rcComm_t* myConn;
    rodsEnv* myEnv;

    public:
    void set_query_output_for_data_objects(genQueryInp_t& genQueryInp);
    void set_query_output_for_collections(genQueryInp_t& genQueryInp);
    void process_query_line_for_data_objects(genQueryOut_t* genQueryOutp, int i);
    void process_query_line_for_collections(genQueryOut_t* genQueryOutp, int i);
    std::vector<std::string> rDataName;
    std::vector<std::string> rCollName;
    std::vector<std::string> rDataPath;
    std::vector<double> rDataSize; // CRAN (i.e. R) does not support long long so use double
    std::vector<char> rDataType;
    std::vector<std::string> rCreateTime;
    std::vector<std::string> rModifyTime;

    LsOutput(rodsEnv* env, rcComm_t* conn);

    void process_collection(char* path);
    void process_data_object(char* path);

    Rcpp::DataFrame toDataFrame();
};


#endif // R_LS_QUERY_H__
