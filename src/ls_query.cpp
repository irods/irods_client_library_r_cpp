#include "ls_query.h"

LsOutput::LsOutput(rodsEnv* env, rcComm_t* conn): myConn(conn), myEnv(env) {}

void LsOutput::set_query_output_for_data_objects(genQueryInp_t& genQueryInp){
    addInxIval(&genQueryInp.selectInp, COL_DATA_NAME, 1);
    addInxIval(&genQueryInp.selectInp, COL_COLL_NAME, 1);
    addInxIval(&genQueryInp.selectInp, COL_DATA_SIZE, 1);
    addInxIval(&genQueryInp.selectInp, COL_D_DATA_PATH, 1);
    addInxIval(&genQueryInp.selectInp, COL_D_CREATE_TIME, 1);
    addInxIval(&genQueryInp.selectInp, COL_D_MODIFY_TIME, 1);
}

void LsOutput::set_query_output_for_collections(genQueryInp_t& genQueryInp){
    addInxIval( &genQueryInp.selectInp, COL_COLL_NAME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_CREATE_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_MODIFY_TIME, 1 );
    addInxIval( &genQueryInp.selectInp, COL_COLL_TYPE, 1 );
//    addInxIval( &genQueryInp.selectInp, COL_COLL_INFO1, 1 );
//    addInxIval( &genQueryInp.selectInp, COL_COLL_INFO2, 1 );
}

void LsOutput::process_query_line_for_data_objects(genQueryOut_t* genQueryOutp, int i){
    sqlResult_t *dataName = 0;
    sqlResult_t *collName = 0;
    sqlResult_t *dataSize = 0;
    sqlResult_t *dataPath = 0;
    sqlResult_t *createTime = 0;
    sqlResult_t *modifyTime = 0;

    dataName   = getSqlResultByInx( genQueryOutp, COL_DATA_NAME );
    collName   = getSqlResultByInx( genQueryOutp, COL_COLL_NAME );
    dataSize   = getSqlResultByInx( genQueryOutp, COL_DATA_SIZE );
    dataPath   = getSqlResultByInx( genQueryOutp, COL_D_DATA_PATH );
    createTime = getSqlResultByInx( genQueryOutp, COL_D_CREATE_TIME );
    modifyTime = getSqlResultByInx( genQueryOutp, COL_D_MODIFY_TIME );

    //conversions
    std::stringstream sstr(&dataSize->value[dataSize->len * i]);
    double tmpSize;
    sstr >> tmpSize;

    //push into vectors
    rDataName.push_back(std::string(&dataName->value[dataName->len * i]));
    rCollName.push_back(std::string(&collName->value[collName->len * i]));
    rDataSize.push_back(tmpSize);
    rDataPath.push_back(std::string(&dataPath->value[dataPath->len * i]));
    rCreateTime.push_back(std::string(&createTime->value[createTime->len * i]));
    rModifyTime.push_back(std::string(&modifyTime->value[modifyTime->len * i]));
    rDataType.push_back('d');
}

void LsOutput::process_query_line_for_collections(genQueryOut_t* genQueryOutp, int i){
    sqlResult_t *collName = 0;
    sqlResult_t *createTime = 0;
    sqlResult_t *modifyTime = 0;

    collName = getSqlResultByInx( genQueryOutp, COL_COLL_NAME );
    createTime = getSqlResultByInx( genQueryOutp, COL_COLL_CREATE_TIME );
    modifyTime = getSqlResultByInx( genQueryOutp, COL_COLL_MODIFY_TIME );

    rDataName.push_back(std::string());
    rCollName.push_back(std::string(&collName->value[collName->len * i]));
    rDataSize.push_back(-1);
    rDataPath.push_back(std::string());
    rCreateTime.push_back(std::string(&createTime->value[createTime->len * i]));
    rModifyTime.push_back(std::string(&modifyTime->value[modifyTime->len * i]));
    rDataType.push_back('C');
}

void LsOutput::process_collection(char* path ){
    int status;
    char condStr[MAX_NAME_LEN];

    // code from lsUtil.cpp
    genQueryInp_t genQueryInp;
    genQueryOut_t* genQueryOutPtr = 0;

    // Query for subcollections
    memset(&genQueryInp, 0, sizeof( genQueryInp_t ) );
    genQueryInp.maxRows = MAX_SQL_ROWS_RIRODS;

    snprintf( condStr, MAX_NAME_LEN, "='%s'", path );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_PARENT_NAME, condStr );

    set_query_output_for_collections(genQueryInp);

    status = rcGenQuery(myConn, &genQueryInp, &genQueryOutPtr);
    if ( status < 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            ::Rf_error("rirods exception (rcGenQuery error for \"%s\")\n", path);
            return;
        }
    }

    if(genQueryOutPtr != 0) {
		int rowsCount = genQueryOutPtr->rowCnt;
		for ( int i = 0; i < rowsCount; i++ ) {
			process_query_line_for_collections(genQueryOutPtr, i);
		}

		free (genQueryOutPtr);
    }

    // Query for data objects
    genQueryOutPtr = 0;

    memset(&genQueryInp, 0, sizeof( genQueryInp_t ) );
    genQueryInp.maxRows = MAX_SQL_ROWS_RIRODS;

    snprintf( condStr, MAX_NAME_LEN, "='%s'", path );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, condStr );

    set_query_output_for_data_objects(genQueryInp);

    status = rcGenQuery(myConn, &genQueryInp, &genQueryOutPtr);
    if ( status < 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            ::Rf_error("rirods exception (rcGenQuery error for \"%s\")\n", path);
            return;
        }
    }

    if(genQueryOutPtr != 0) {

		for ( int i = 0; i < genQueryOutPtr->rowCnt; i++ ) {
			process_query_line_for_data_objects(genQueryOutPtr, i);
		}

		free (genQueryOutPtr);
    }
}

void LsOutput::process_data_object(char* path){
    int status;
    char dataObjName[MAX_NAME_LEN];
    char collectionName[MAX_NAME_LEN];
    char condStr[MAX_NAME_LEN];

    splitPathByKey( path, collectionName, MAX_NAME_LEN, dataObjName, MAX_NAME_LEN, '/' );

    // code from lsUtil.cpp
    genQueryInp_t genQueryInp;
    genQueryOut_t* genQueryOutPtr = 0;

    memset(&genQueryInp, 0, sizeof( genQueryInp_t ) );
    genQueryInp.maxRows = MAX_SQL_ROWS_RIRODS;

    snprintf( condStr, MAX_NAME_LEN, "='%s'", collectionName );
    addInxVal( &genQueryInp.sqlCondInp, COL_COLL_NAME, condStr );
    snprintf( condStr, MAX_NAME_LEN, "='%s'", dataObjName );
    addInxVal( &genQueryInp.sqlCondInp, COL_DATA_NAME, condStr );


    set_query_output_for_data_objects(genQueryInp);

    status = rcGenQuery(myConn, &genQueryInp, &genQueryOutPtr);
    if ( status < 0 ) {
        if ( status != CAT_NO_ROWS_FOUND ) {
            ::Rf_error("rirods exception (rcGenQuery error for \"%s\")\n", path);
            return;
        }
    }

    if(genQueryOutPtr != 0) {

		for ( int i = 0; i < genQueryOutPtr->rowCnt; i++ ) {
			process_query_line_for_data_objects(genQueryOutPtr, i);
		}

		free (genQueryOutPtr);
    }
}


Rcpp::DataFrame LsOutput::toDataFrame(){
    return Rcpp::DataFrame::create( Rcpp::Named("Data_name")=Rcpp::wrap(rDataName),
                                    Rcpp::Named("Collection_name")=Rcpp::wrap(rCollName),
                                    Rcpp::Named("Data_path")=Rcpp::wrap(rDataPath),
                                    Rcpp::Named("Data_size")=Rcpp::wrap(rDataSize),
                                    Rcpp::Named("Data_type")=Rcpp::wrap(rDataType),
                                    Rcpp::Named("Create_time")=Rcpp::wrap(rCreateTime),
                                    Rcpp::Named("Modify_time")=Rcpp::wrap(rModifyTime),
                                    Rcpp::Named("stringsAsFactors")=false
                                    );
}
