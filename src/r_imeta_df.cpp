/*
 * r_imeta_df.cpp
 *
 *  Created on: Jan 26, 2016
 *      Author: radovan.chytracek@rd.nestle.com
 */

#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/

/*
  This is an interface to the Attribute-Value-Units type of metadata.
*/

#include "rods.h"
#include "rodsClient.h"

#include <boost/tokenizer.hpp>

#include <sstream>
#include <string>
#include <vector>

// We need to resolve linker error due to colliding symbols after splitting r_imeta.cpp
namespace df
{
#define MAX_SQL 300
#define BIG_STR 3000

int testMode = 0; /* some particular internal tests */
int longMode = 0; /* more detailed listing */
int upperCaseFlag = 0;

char cwd[BIG_STR];

char zoneArgument[MAX_NAME_LEN + 2] = "";


typedef std::vector<std::string> MetaColumn;

struct _MetaData  {
	MetaColumn data_objects;
	MetaColumn collections;
	MetaColumn attributes;
	MetaColumn values;
	MetaColumn units;
	MetaColumn times;

	Rcpp::DataFrame to_df() const {
		if(has_both())
		{
			return Rcpp::DataFrame::create (
					Rcpp::Named("Collection") = Rcpp::wrap(collections),
					Rcpp::Named("File") = Rcpp::wrap(data_objects),
					Rcpp::Named("stringsAsFactors")=false
					);
		}
		else if(has_collections())
		{
			return Rcpp::DataFrame::create (
					Rcpp::Named("Collection") = Rcpp::wrap(collections),
					Rcpp::Named("stringsAsFactors")=false
					);
		}
		else if(has_files())
		{
			return Rcpp::DataFrame::create (
					Rcpp::Named("File") = Rcpp::wrap(data_objects),
					Rcpp::Named("stringsAsFactors")=false
					);
		}
		else if(has_time())
		{
			return Rcpp::DataFrame::create (
					Rcpp::Named("Attribute") = Rcpp::wrap(attributes),
					Rcpp::Named("Value")     = Rcpp::wrap(values),
					Rcpp::Named("Unit")      = Rcpp::wrap(units),
					Rcpp::Named("Time")      = Rcpp::wrap(times),
					Rcpp::Named("stringsAsFactors")=false
					);
		}
		else if(has_attributes())
		{
			return Rcpp::DataFrame::create (
					Rcpp::Named("Attribute") = Rcpp::wrap(attributes),
					Rcpp::Named("Value")     = Rcpp::wrap(values),
					Rcpp::Named("Unit")      = Rcpp::wrap(units),
					Rcpp::Named("stringsAsFactors")=false
					);
		}
		else
			return Rcpp::DataFrame::create();
	}

	const struct _MetaData& operator +(const struct _MetaData& right);

	size_t size() const {
		if(this->has_time())
			return times.size();
		else if(this->has_files())
			return data_objects.size();
		else if(this->has_collections())
			return collections.size();
		else
			return attributes.size();
	}

	bool empty() const {
		return (
				attributes.empty() &&
				values.empty() &&
				units.empty() &&
				data_objects.empty() &&
				collections.empty() &&
				times.empty()
				);
	}

	bool has_attributes() const {
		return(!attributes.empty());
	}

	bool has_time() const {
		return(!times.empty());
	}

	bool has_files() const {
		return(!data_objects.empty());
	}

	bool has_collections() const {
		return(!collections.empty());
	}

	bool has_both() const {
		return(this->has_files() && this->has_collections());
	}

	void print(std::string cmd) const
	{
		size_t allm = this->size();
		for(size_t i=0; i<allm; i++)
		{
			if(this->has_time())
				Rprintf("[imeta::%s]T attribute: %s value: %s unit: %s time: %s\n",
						cmd.c_str(), attributes[i].c_str(), values[i].c_str(), units[i].c_str(), times[i].c_str());
			else if(this->has_both())
				Rprintf("[imeta::%s]B collection: %s data object: %s\n",
						cmd.c_str(), collections[i].c_str(), data_objects[i].c_str());
			else if(this->has_collections())
				Rprintf("[imeta::%s]C collection: %s\n",
						cmd.c_str(), collections[i].c_str());
			else if(this->has_files())
				Rprintf("[imeta::%s]F data object: %s\n",
						cmd.c_str(), data_objects[i].c_str());
			else
				Rprintf("[imeta::%s]A attribute: %s value: %s unit: %s\n",
						cmd.c_str(), attributes[i].c_str(), values[i].c_str(), units[i].c_str());
		}
	}

};

typedef struct _MetaData MetaData;

// Concatenates two vectors into the left vector which "absorbes" all the items from the right vector
// The right vector does not change
void cat_vectors(MetaColumn& left, const MetaColumn& right)
{
	left.resize(left.size()+right.size());
	left.insert(left.end(), right.begin(), right.end());
};

const MetaData& MetaData::operator +(const MetaData& right)
{
	// Prevent duplication
	if(this == &right)
		return *this;

	cat_vectors(this->data_objects, right.data_objects);
	cat_vectors(this->collections,  right.collections);
	cat_vectors(this->attributes,   right.attributes);
	cat_vectors(this->values,       right.values);
	cat_vectors(this->units,        right.units);
	cat_vectors(this->times,        right.times);

	return *this;
}

/*
 print the results of a general query.
 */
MetaData printGenQueryResults( 	rcComm_t *Conn,
								char* cmd, int status, int printCount,
								genQueryOut_t *genQueryOut, char *descriptions[] )
{
    int i, j;
    char localTime[TIME_LEN];

    MetaData result;

    if ( status == CAT_NO_ROWS_FOUND ) {
        return(result);
    }
    if ( status != 0 && status != CAT_NO_ROWS_FOUND ) {
        Rf_error( "[imeta::%s] rcGenQuery: %d\n", cmd, status );
        return(result);
    }
    else {
        if ( status == CAT_NO_ROWS_FOUND ) {
            if ( printCount == 0 ) {
                Rf_error( "[imeta::%s] No rows found\n", cmd );
                return(result);
            }
        }
        else {
            for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
                if ( i > 0 ) {
                    ;//Rprintf( "[imeta::%s] ----\n", cmd );
                }
                for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[j].value;
                    tResult += i * genQueryOut->sqlResult[j].len;
                    if ( *descriptions[j] != '\0' ) {
                        if ( strstr( descriptions[j], "time" ) != 0 ) {
                            getLocalTimeFromRodsTime( tResult, localTime );
                            //Rprintf( "[imeta::%s] %s: %s\n", cmd, descriptions[j], localTime );
                            // TODO : What imeta command returns the time information?
                            // Perform pivot from rows to columns
                            std::string column_value = localTime;
                            result.times.push_back(localTime);
                        }
                        else {
                            //Rprintf( "[imeta::%s] %s: %s\n", cmd, descriptions[j], tResult );
                            // Perform pivot from rows to columns
                            std::string column_name  = descriptions[j];
                            std::string column_value = tResult;
                            if(column_name == "attribute")
                            {
                            	result.attributes.push_back(column_value);
                            }
                            else if(column_name == "value")
                            {
                            	result.values.push_back(column_value);
                            }
                            else if(column_name == "units")
                            {
                            	result.units.push_back(column_value);
                            }
                            else if(column_name == "dataObj")
                            {
                            	result.data_objects.push_back(column_value);
                            }
                            else if(column_name == "collection")
                            {
                            	result.collections.push_back(column_value);
                            }
                            else
                            {
                            	Rf_error("[imeta:%s] Unknown metadata column data %s=%s\n",
                            			 cmd, column_name.c_str(), column_value.c_str());
                            }
                            printCount++;
                        }
                    }
                }
            }
            //result.print(cmd);
            return(result); // TODO : Empty result for now, to be fixed!
        }
    }
    return(result);
}

/*
 Via a general query and show the AVUs for a dataobject.
 */
df::MetaData showDataObj( rcComm_t *Conn, rodsEnv Env, char* cmd, char *name, char *attrName, int wild )
{
	df::MetaData result;

    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    char fullName[MAX_NAME_LEN];
    char myDirName[MAX_NAME_LEN];
    char myFileName[MAX_NAME_LEN];
    int status;
    /* "id" only used in testMode, in longMode id is reset to be 'time set' :*/
    char *columnNames[] = {(char*)"attribute", (char*)"value", (char*)"units", (char*)"id"};

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    //Rprintf( "[imeta::%s] AVUs defined for dataObj %s:\n", cmd, name );
    int printCount = 0;
    i1a[0] = COL_META_DATA_ATTR_NAME;
    i1b[0] = 0;
    i1a[1] = COL_META_DATA_ATTR_VALUE;
    i1b[1] = 0;
    i1a[2] = COL_META_DATA_ATTR_UNITS;
    i1b[2] = 0;
    if ( testMode ) {
        i1a[3] = COL_META_DATA_ATTR_ID;
        i1b[3] = 0;
    }
    if ( longMode ) {
        i1a[3] = COL_META_DATA_MODIFY_TIME;
        i1b[3] = 0;
        columnNames[3] = (char*)"time set";
    }
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 3;
    if ( testMode ) {
        genQueryInp.selectInp.len = 4;
    }
    if ( longMode ) {
        genQueryInp.selectInp.len = 4;
    }

    i2a[0] = COL_COLL_NAME;
    std::string v1;
    v1 = "='";
    v1 += cwd;
    v1 += "'";

    i2a[1] = COL_DATA_NAME;
    std::string v2;
    v2 = "='";
    v2 += name;
    v2 += "'";

    if ( *name == '/' ) {
        snprintf( fullName, sizeof( fullName ), "%s", name );
    }
    else {
        snprintf( fullName, sizeof( fullName ), "%s/%s", cwd, name );
    }

    if ( strstr( name, "/" ) != NULL ) {
        /* reset v1 and v2 for when full path or relative path entered */
        if ( int status = splitPathByKey( fullName, myDirName, MAX_NAME_LEN, myFileName, MAX_NAME_LEN, '/' ) )
        {
            Rf_error( "[imeta::%s] splitPathByKey failed in showDataObj with status %d\n", cmd, status );
        }

        v1 = "='";
        v1 += myDirName;
        v1 += "'";

        v2 = "='";
        v2 += myFileName;
        v2 += "'";
    }

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    std::string v3;
    if ( attrName != NULL && *attrName != '\0' ) {
        i2a[2] = COL_META_DATA_ATTR_NAME;
        if ( wild ) {
            v3  = "like '";
            v3 += attrName;
            v3 += "'";
        }
        else {
            v3  = "='";
            v3 += attrName;
            v3 += "'";
        }
        condVal[2] = const_cast<char*>( v3.c_str() );
        genQueryInp.sqlCondInp.len++;
    }

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_D_DATA_PATH;
        genQueryInp.selectInp.len = 1;
        genQueryInp.sqlCondInp.len = 2;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            Rprintf( "None\n" );
            return result;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            //lastCommandStatus = status;
            Rf_error( "[imeta::%s] Dataobject %s does not exist\n", cmd, fullName );
            Rf_error( "[imeta::%s] or, if 'strict' access control is enabled, you may not have access.\n", cmd );
            return result;
        }
        result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }
    else {
        result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            ;//Rprintf( "----\n" );
        }

        result = result + printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }

    return result;
}

/*
Via a general query, show the AVUs for a collection
*/
df::MetaData showColl( rcComm_t *Conn, rodsEnv Env, char* cmd, char *name, char *attrName, int wild )
{
	df::MetaData result;

    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    char fullName[MAX_NAME_LEN];
    int  status, printCount;
    char *columnNames[] = {(char*)"attribute", (char*)"value", (char*)"units"};

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    //s] AVUs defined for collection %s:\n", cmd, name );
    printCount = 0;
    i1a[0] = COL_META_COLL_ATTR_NAME;
    i1b[0] = 0; /* currently unused */
    i1a[1] = COL_META_COLL_ATTR_VALUE;
    i1b[1] = 0;
    i1a[2] = COL_META_COLL_ATTR_UNITS;
    i1b[2] = 0;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 3;

    strncpy( fullName, cwd, MAX_NAME_LEN );
    if ( strlen( name ) > 0 ) {
        if ( *name == '/' ) {
            strncpy( fullName, name, MAX_NAME_LEN );
        }
        else {
            rstrcat( fullName, "/", MAX_NAME_LEN );
            rstrcat( fullName, name, MAX_NAME_LEN );
        }
    }

    // JMC cppcheck - dangerous use of strcpy : need a explicit null term
    // NOTE :: adding len of name + 1 for added / + len of cwd + 1 for null term
    if ( ( strlen( name ) + 1 + strlen( cwd ) + 1 ) < LONG_NAME_LEN ) {
        fullName[ strlen( name ) + 1 + strlen( cwd ) + 1 ] = '\0';
    }
    else {
    	std::string error_name = cwd;
    	error_name += "/";
    	error_name += name;
    	size_t error_length = strlen( name ) + 1 + strlen( cwd ) + 1;
        Rf_error( 	"[imeta::%s] showColl :: error - fullName could not be explicitly null terminated: %s, size: %d\n",
        		    cmd,
        			error_name.c_str(),
					error_length);
    }

    i2a[0] = COL_COLL_NAME;
    std::string v1;
    v1 =  "='";
    v1 += fullName;
    v1 += "'";

    condVal[0] = const_cast<char*>( v1.c_str() );

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    std::string v2;
    if ( attrName != NULL && *attrName != '\0' ) {
        i2a[1] = COL_META_COLL_ATTR_NAME;
        if ( wild ) {
            v2 =  "like '";
            v2 += attrName;
            v2 += "'";
        }
        else {
            v2 =  "= '";
            v2 += attrName;
            v2 += "'";
        }
        condVal[1] = const_cast<char*>( v2.c_str() );
        genQueryInp.sqlCondInp.len++;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_COLL_COMMENTS;
        genQueryInp.selectInp.len = 1;
        genQueryInp.sqlCondInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            Rprintf( "None\n" );
            return result;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            //lastCommandStatus = status;
            Rf_error( "[imeta::%s] Collection %s does not exist.\n", cmd, fullName );
            return result;
        }
    }

    result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            ;//Rprintf( "----\n" );
        }

        result = result + printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }

    return result;
}

/*
Via a general query, show the AVUs for a resource
*/
df::MetaData showResc( rcComm_t *Conn, rodsEnv Env, char* cmd, char *name, char *attrName, int wild )
{
	df::MetaData result;

    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    int  status;
    char *columnNames[] = {(char*)"attribute", (char*)"value", (char*)"units"};

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    //Rprintf( "[imeta::%s] AVUs defined for resource %s:\n", cmd, name );

    int printCount = 0;
    i1a[0] = COL_META_RESC_ATTR_NAME;
    i1b[0] = 0; /* currently unused */
    i1a[1] = COL_META_RESC_ATTR_VALUE;
    i1b[1] = 0;
    i1a[2] = COL_META_RESC_ATTR_UNITS;
    i1b[2] = 0;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 3;

    i2a[0] = COL_R_RESC_NAME;
    std::string v1;
    v1 =  "='";
    v1 += name;
    v1 += "'";
    condVal[0] = const_cast<char*>( v1.c_str() );

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 1;

    std::string v2;
    if ( attrName != NULL && *attrName != '\0' ) {
        i2a[1] = COL_META_RESC_ATTR_NAME;
        if ( wild ) {
            v2 =  "like '";
            v2 += attrName;
            v2 += "'";
        }
        else {
            v2 =  "= '";
            v2 += attrName;
            v2 += "'";
        }
        condVal[1] = const_cast<char*>( v2.c_str() );
        genQueryInp.sqlCondInp.len++;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_R_RESC_INFO;
        genQueryInp.selectInp.len = 1;
        genQueryInp.sqlCondInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            Rprintf( "None\n" );
            return result;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            //lastCommandStatus = status;
            Rf_error( "[imeta::%s] Resource %s does not exist.\n", cmd, name );
            return result;
        }
    }

    result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }
        result = result + printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }

    return result;
}

/*
Via a general query, show the AVUs for a user
*/
df::MetaData showUser( rcComm_t *Conn, rodsEnv Env, char* cmd, char *name, char *attrName, int wild )
{
	df::MetaData result;

    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];
    int status;
    char *columnNames[] = {(char*)"attribute", (char*)"value", (char*)"units"};

    char userName[NAME_LEN];
    char userZone[NAME_LEN];

    status = parseUserName( name, userName, userZone );
    if ( status ) {
        Rf_error( "[imeta::%s] sizeof( userZone ) %s\n", cmd, Env.rodsZone );
    }

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    //Rprintf( "[imeta::%s] AVUs defined for user %s#%s:\n", cmd, userName, userZone );

    int printCount = 0;
    i1a[0] = COL_META_USER_ATTR_NAME;
    i1b[0] = 0; /* currently unused */
    i1a[1] = COL_META_USER_ATTR_VALUE;
    i1b[1] = 0;
    i1a[2] = COL_META_USER_ATTR_UNITS;
    i1b[2] = 0;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 3;

    i2a[0] = COL_USER_NAME;
    std::string v1;
    v1 =  "='";
    v1 += userName;
    v1 += "'";

    i2a[1] = COL_USER_ZONE;
    std::string v2;
    v2 =  "='";
    v2 += userZone;
    v2 += "'";

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    std::string v3;
    if ( attrName != NULL && *attrName != '\0' ) {
        i2a[2] = COL_META_USER_ATTR_NAME;
        if ( wild ) {
            v3 =  "like '";
            v3 += attrName;
            v3 += "'";
        }
        else {
            v3 =  "= '";
            v3 += attrName;
            v3 += "'";
        }
        condVal[2] = const_cast<char*>( v3.c_str() );
        genQueryInp.sqlCondInp.len++;
    }

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
    if ( status == CAT_NO_ROWS_FOUND ) {
        i1a[0] = COL_USER_COMMENT;
        genQueryInp.selectInp.len = 1;
        genQueryInp.sqlCondInp.len = 1;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( status == 0 ) {
            Rprintf( "None\n" );
            return result;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            //lastCommandStatus = status;
            Rf_error( "[imeta::%s] User %s does not exist.\n", cmd, name );
            return result;
        }
    }

    result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            ;//Rprintf( "----\n" );
        }
        result = result + printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }

    return result;
}

/*
Do a query on AVUs for dataobjs and show the results
attribute op value [AND attribute op value] [REPEAT]
 */
df::MetaData queryDataObj( rcComm_t *Conn, rodsEnv Env, char* cmd, char *cmdToken[] )
{
	df::MetaData result;

    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20];
    int i2a[20];
    char *condVal[20];

    int status;
    char *columnNames[] = {(char*)"collection", (char*)"dataObj"};
    int cmdIx;
    int condIx;

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    int printCount = 0;
    i1a[0] = COL_COLL_NAME;
    i1b[0] = 0; /* (unused) */
    i1a[1] = COL_DATA_NAME;
    i1b[1] = 0;
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 2;

    i2a[0] = COL_META_DATA_ATTR_NAME;
    std::string v1;
    v1 =  "='";
    v1 += cmdToken[2];
    v1 += "'";

    i2a[1] = COL_META_DATA_ATTR_VALUE;
    std::string v2;
    v2 =  cmdToken[3];
    v2 += " '";
    v2 += cmdToken[4];
    v2 += "'";

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    if ( strcmp( cmdToken[5], "or" ) == 0 ) {
        std::stringstream s;
        s << "|| " << cmdToken[6] << " '" << cmdToken[7] << "'";
        v2 += s.str();
    }

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    cmdIx = 5;
    condIx = 2;
    std::vector<std::string> vstr( condIx );
    while ( strcmp( cmdToken[cmdIx], "and" ) == 0 ) {
        i2a[condIx] = COL_META_DATA_ATTR_NAME;
        cmdIx++;
        std::stringstream s1;
        s1 << "='" << cmdToken[cmdIx] << "'";
        vstr.push_back( s1.str() );
        condVal[condIx] = const_cast<char*>( vstr.back().c_str() );
        condIx++;

        i2a[condIx] = COL_META_DATA_ATTR_VALUE;
        std::stringstream s2;
        s2 << cmdToken[cmdIx + 1] << " '" << cmdToken[cmdIx + 2] << "'";
        vstr.push_back( s2.str() );
        cmdIx += 3;
        condVal[condIx] = const_cast<char*>( vstr.back().c_str() );
        condIx++;
        genQueryInp.sqlCondInp.len += 2;
    }

    if ( *cmdToken[cmdIx] != '\0' ) {
        Rf_error( "[imeta::%s] Unrecognized input\n", cmd );
        return result; //-2;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            ;//Rprintf( "----\n" );
        }
        result = result + printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }

    return result; //0
}

/*
Do a query on AVUs for collections and show the results
 */
df::MetaData queryCollection( rcComm_t *Conn, rodsEnv Env, char* cmd, char *cmdToken[] )
{
	df::MetaData result;

    genQueryOut_t *genQueryOut;
    int i1a[20];
    int i1b[20];
    int i2a[20];
    char *condVal[20];

    int status;
    char *columnNames[] = {(char*)"collection"};
    int cmdIx;
    int condIx;

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    int printCount = 0;
    i1a[0] = COL_COLL_NAME;
    i1b[0] = 0; /* (unused) */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_META_COLL_ATTR_NAME;
    std::string v1;
    v1 =  "='";
    v1 += cmdToken[2];
    v1 += "'";

    i2a[1] = COL_META_COLL_ATTR_VALUE;
    std::string v2;
    v2 =  cmdToken[3];
    v2 += " '";
    v2 += cmdToken[4];
    v2 += "'";

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    if ( strcmp( cmdToken[5], "or" ) == 0 ) {
        std::stringstream s;
        s << "|| " << cmdToken[6] << " '" << cmdToken[7] << "'";
        v2 += s.str();
    }

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    cmdIx = 5;
    condIx = 2;
    std::vector<std::string> vstr( condIx );
    while ( strcmp( cmdToken[cmdIx], "and" ) == 0 ) {
        i2a[condIx] = COL_META_COLL_ATTR_NAME;
        cmdIx++;
        std::stringstream s1;
        s1 << "='" << cmdToken[cmdIx] << "'";
        vstr.push_back( s1.str() );
        condVal[condIx] = const_cast<char*>( vstr.back().c_str() );
        condIx++;

        i2a[condIx] = COL_META_COLL_ATTR_VALUE;
        std::stringstream s2;
        s2 << cmdToken[cmdIx + 1] << " '" << cmdToken[cmdIx + 2] << "'";
        vstr.push_back( s2.str() );
        cmdIx += 3;
        condVal[condIx] = const_cast<char*>( vstr.back().c_str() );
        condIx++;
        genQueryInp.sqlCondInp.len += 2;
    }

    if ( *cmdToken[cmdIx] != '\0' ) {
        Rf_error( "[imeta::%s] Unrecognized input\n", cmd );
        return result; //-2;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            ;//Rprintf( "----\n" );
        }
        result = result + printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }

    return result; //0;
}


/*
Do a query on AVUs for resources and show the results
 */
df::MetaData queryResc( rcComm_t *Conn, rodsEnv Env, char* cmd, char *attribute, char *op, char *value )
{
	df::MetaData result;

    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    int status;
    char *columnNames[] = {(char*)"resource"};

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    int printCount = 0;
    i1a[0] = COL_R_RESC_NAME;
    i1b[0] = 0; /* (unused) */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 1;

    i2a[0] = COL_META_RESC_ATTR_NAME;
    std::string v1;
    v1 =  "='";
    v1 += attribute;
    v1 += "'";

    i2a[1] = COL_META_RESC_ATTR_VALUE;
    std::string v2;
    v2 =  op;
    v2 += " '";
    v2 += value;
    v2 += "'";

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            ;//Rprintf( "----\n" );
        }
        result = result + printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );
    }

    return result; //0;
}

/*
Do a query on AVUs for users and show the results
 */
df::MetaData queryUser( rcComm_t *Conn, rodsEnv Env, char* cmd, char *attribute, char *op, char *value )
{
	df::MetaData result;

    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];
    int status;
    char *columnNames[] = {(char*)"user", (char*)"zone"};

    int printCount = 0;
    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    i1a[0] = COL_USER_NAME;
    i1b[0] = 0; /* (unused) */
    i1a[1] = COL_USER_ZONE;
    i1b[1] = 0; /* (unused) */
    genQueryInp.selectInp.inx = i1a;
    genQueryInp.selectInp.value = i1b;
    genQueryInp.selectInp.len = 2;

    i2a[0] = COL_META_USER_ATTR_NAME;
    std::string v1;
    v1 =  "='";
    v1 += attribute;
    v1 += "'";

    i2a[1] = COL_META_USER_ATTR_VALUE;
    std::string v2;
    v2 =  op;
    v2 += " '";
    v2 += value;
    v2 += "'";

    condVal[0] = const_cast<char*>( v1.c_str() );
    condVal[1] = const_cast<char*>( v2.c_str() );

    genQueryInp.sqlCondInp.inx = i2a;
    genQueryInp.sqlCondInp.value = condVal;
    genQueryInp.sqlCondInp.len = 2;

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    result = printGenQueryResults( Conn, cmd, status, printCount, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            ;//Rprintf( "----\n" );
        }
        result = result + printGenQueryResults( Conn, cmd, status, printCount,  genQueryOut, columnNames );
    }

    return result; //0;
}

namespace i_meta
{
	struct _avu_def
	{
		std::string attribute;
		std::string value;
		std::string unit;

		bool operator!=(const i_meta::_avu_def& r ) const
		{
			return ( this->attribute != r.attribute || this->value != r.value || this->unit != r.unit);
		}

		bool operator==(const i_meta::_avu_def& r ) const
		{
			return(! this->operator!=(r));
		}
	};

	typedef struct i_meta::_avu_def AVU;

	i_meta::AVU empty_avu = {"","",""};

	typedef std::vector<i_meta::AVU> AVUs;

	AVUs empty_avus;
}

}

/* handle a command,
   return code is 0 if the command was (at least partially) valid,
   -1 for quitting,
   -2 for if invalid
   -3 if empty.
 */
df::MetaData doCommand_df(
				rcComm_t *Conn,
				rodsEnv Env,
				int& finalStatus,                                   // Because we need to return MetaData we pass in status var
				const std::string& command,                         // add*, ls*, rm*, cp, qu
				const std::string& object_type,                     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& src_name,                        // iRODS object name - should be always present
				const std::string& dst_object_type="",              // Destination object type (cp only)
				const std::string& dst_name ="",                    // iRODS object name(cp only)
				const df::i_meta::AVU& avu      =df::i_meta::empty_avu, // Default is empty, but it is always supplied by imeta function
				const df::i_meta::AVU& new_avu  =df::i_meta::empty_avu, // Default is empty, but it is supplied by imeta function
				const std::string& query    =""                     // free style string for the moment (qu only)
			)
{
	df::MetaData metas;

    if(
    	command.empty()     ||    // No command, no type, no work done
    	object_type.empty()
	  )
    {
    	Rf_error("[imeta::???] Command and object type is required!\n");
    	finalStatus = -3;
    	return metas;
    }

    // Prefix object_type with "-" as it is sent to iRODS server for processing
    std::string normalized_object_type = std::string("-")+object_type;
    std::string normalized_dst_object_type = std::string("-")+dst_object_type;

    int wild = 0;
    int doLs = 0;

    if ( command == "lsw" )
    {
        doLs = 1;
        wild = 1;
    }
    if ( command == "ls" )
    {
        doLs = 1;
        wild = 0;
    }

	if ( doLs )
    {
        if ( object_type == "d" || object_type == "D")
		{
        	metas = df::showDataObj( Conn, Env, (char*)command.c_str(), (char*)src_name.c_str(), (char*)avu.attribute.c_str(), wild );
        	//metas.print(command);
        	finalStatus = 0;
            return metas;
        }
        if ( object_type == "c" || object_type == "C")
		{
        	metas = df::showColl( Conn, Env, (char*)command.c_str(), (char*)src_name.c_str(), (char*)avu.attribute.c_str(), wild );
        	//metas.print(command);
        	finalStatus = 0;
            return metas;
        }
        if ( object_type == "r" || object_type == "R")
		{
        	metas = df::showResc( Conn, Env, (char*)command.c_str(), (char*)src_name.c_str(), (char*)avu.attribute.c_str(), wild );
        	//metas.print(command);
        	finalStatus = 0;
            return metas;
        }
        if ( command == "u" )
        {
        	metas = df::showUser( Conn, Env, (char*)command.c_str(), (char*)src_name.c_str(), (char*)avu.attribute.c_str(), wild );
        	//metas.print(command);
        	finalStatus = 0;
            return metas;
        }
    }

    if ( command == "qu" )
    {
    	if(query.empty())
    	{
    		Rf_error("[imeta::%s] Query string is required!\n", command.c_str());
			char *msgs[] = {
				(char*)" qu -d|C|R|u AttName Op AttVal [...] (Query objects with matching AVUs)",
				(char*)"Query across AVUs for the specified type of item",
				(char*)"Example: qu -d distance '<=' 12",
				(char*)" ",
				(char*)"When querying dataObjects (-d) or collections (-C) additional conditions",
				(char*)"(AttName Op AttVal) may be given separated by 'and', for example:",
				(char*)" qu -d a = b and c '<' 10",
				(char*)"Or a single 'or' can be given for the same AttName, for example",
				(char*)" qu -d r '<' 5 or '>' 7",
				(char*)" ",
				(char*)"You can also query in numeric mode (instead of as strings) by adding 'n'",
				(char*)"in front of the test condition, for example:",
				(char*)" qu -d r 'n<' 123",
				(char*)"which causes it to cast the AVU column to numeric (decimal) in the SQL.",
				(char*)"In numeric mode, if any of the named AVU values are non-numeric, a SQL",
				(char*)"error will occur but this avoids problems when comparing numeric strings",
				(char*)"of different lengths.",
				(char*)" ",
				(char*)"Other examples:",
				(char*)" qu -d a like b%",
				(char*)"returns data-objects with attribute 'a' with a value that starts with 'b'.",
				(char*)" qu -d a like %",
				(char*)"returns data-objects with attribute 'a' defined (with any value).",
				(char*)""
			};
			for ( int i = 0;; i++ ) {
				if ( strlen( msgs[i] ) == 0 ) {
		        	finalStatus = 0;
					return metas;
				}
				Rf_error( "[imeta::%s] %s\n", command.c_str(), msgs[i] );
			}
        	finalStatus = -2;
    		return metas;
    	}

    	// Tokenize query using space character
    	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
   	    boost::char_separator<char> sep(" \t\v\r\n", "", boost::keep_empty_tokens);
   	    tokenizer tokens(query, sep);

   	    std::string cmdTokens[40]; // This can possibly got rid off as the tokenizer now keeps empty tokens
   	    char* cmdToken[40];
   	    for ( int i = 0; i<40; i++)
   	    	cmdToken[i] = (char*)"";

   	    cmdToken[0] = (char*)command.c_str();
   	    cmdToken[1] = (char*)normalized_object_type.c_str();
   	    int token = 2;
		for (tokenizer::iterator tok_iter = tokens.begin();tok_iter != tokens.end(); ++tok_iter)
		{
			if(token >= 40)
			{
				break;
			}

			cmdTokens[token] = *tok_iter;
			cmdToken[token]       = (char*)cmdTokens[token].c_str();
			++token;
		}

        int status = 0;
        if ( object_type == "d" )
        {
            // TODO : FIXME pass down the status to be colected by query call status =
        	metas = df::queryDataObj( Conn, Env, (char*)command.c_str(), cmdToken );
            if ( status < 0 ) {
            	finalStatus = -2;
            	return metas;
            }
        	//metas.print(command);
        	finalStatus = 0;
            return metas;
        }
        if ( object_type == "C" )
        {
            // TODO : FIXME pass down the status to be colected by query call status =
        	metas = df::queryCollection( Conn, Env, (char*)command.c_str(), cmdToken );
            if ( status < 0 ) {
            	finalStatus = -2;
                return metas;
            }
        	//metas.print(command);
            return metas;
        }
        if ( object_type == "R" || object_type == "r" )
        {
            if ( *cmdToken[5] != '\0' ) {
                Rf_error( "[imeta::%s] Unrecognized input\n", command.c_str() );
            	finalStatus = -2;
                return metas;
            }
            metas = df::queryResc( Conn, Env, (char*)command.c_str(), cmdToken[2], cmdToken[3], cmdToken[4] );
        	//metas.print(command);
        	finalStatus = 0;
            return metas;
        }
        if ( object_type == "u" )
        {
        	metas = df::queryUser( Conn, Env, (char*)command.c_str(), cmdToken[2], cmdToken[3], cmdToken[4] );
        	//metas.print(command);
        	finalStatus = 0;
            return metas;
        }
    }

    if ( ! command.empty() )
    {
        Rf_error( "[imeta] Unrecognized subcommand '%s', try 'imeta help'\n", command.c_str() );
    	finalStatus = -2;
        return metas;
    }

	finalStatus = -3;
    return metas;
}

//' imeta_list
//' Data frame veriant for inspecting iRODS metadata
//' @param command         add, ls, rm, cp, qu
//' @param object_type     iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param dst_object_type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param dst_name        iRODS object name(cp only)
//' @param avu             format "Attribute;Value;Unit"
//' @param new_avu         format "Attribute;Value;Unit"
//' @param query           See, icommand help 'imeta help qu' for details, help is printed if empty
//'
//                                     // [[Rcpp::export]]
df::MetaData imeta_list (
						int& status,
						const std::string& command,             // add, ls, rm, cp, qu, upper
						const std::string& object_type,         // iRODS files/dD, collections/cC, resources/rR, users/uU
						const std::string& src_name,            // iRODS object name - should be always present
						const std::string& dst_object_type="",  // Destination object type (cp only)
						const std::string& dst_name ="",        // iRODS object name(cp only)
						const std::string& avu      ="", 		// Default is empty, format "Attribute;Value;Unit"
						const std::string& new_avu  ="", 		// Default is empty, format "Attribute;Value;Unit"
						const std::string& query    =""         // free style string for the moment (qu only)
						)
{
	df::MetaData result;

    rErrMsg_t errMsg;
    rcComm_t *Conn;
    rodsEnv myEnv;
    int debug = 0;

    rodsArguments_t myRodsArgs;

    // We have to reset all to 0s because we do not use
    // the "parse_program_options(...)" function
    // The mvUtil(...) function expects positional arguments parser initializing it beforehand
    memset( &myRodsArgs, 0, sizeof( rodsArguments_t ) );

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        Rf_error( "[imeta] getRodsEnv error. status = %d", status);
        status = 1;
        return result;
    }

    strncpy( df::cwd, myEnv.rodsCwd, BIG_STR );
    if ( strlen( df::cwd ) == 0 ) {
        strcpy( df::cwd, "/" );
    }

    Conn = rcConnect( myEnv.rodsHost, myEnv.rodsPort, myEnv.rodsUserName, myEnv.rodsZone, 0, &errMsg );

    if ( Conn == NULL ) {
        char *mySubName = NULL;
        const char *myName = rodsErrorName( errMsg.status, &mySubName );
        Rf_error( "[imeta] rcConnect failure %s (%s) (%d) %s",
                 myName,
                 mySubName,
                 errMsg.status,
                 errMsg.msg );
        //free( mySubName );
        status = 2;
        return result;
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        if ( !debug ) {
        	status = 3;
            return result;
        }
    }

    df::i_meta::AVU     c_avu = df::i_meta::empty_avu;
    df::i_meta::AVU new_c_avu = df::i_meta::empty_avu;

    // Convert AVUs to structures
    if(!avu.empty())
    {
    	// Tokenize query using ';' character
    	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
   	    boost::char_separator<char> sep(";\t\v\r\n", "", boost::keep_empty_tokens);
   	    tokenizer tokens(avu, sep);
   	    std::string cmdToken[3] = {"", "", ""};
   	    int token = 0;
		for (tokenizer::iterator tok_iter = tokens.begin();tok_iter != tokens.end(), token<3; ++tok_iter)
		{
			//Rprintf("%s: Current avu token: %s\n", command.c_str(), (*tok_iter).c_str());
			if(! (*tok_iter).empty() )
				cmdToken[token]       = *tok_iter;
			++token;
		}
		c_avu.attribute = cmdToken[0];
		c_avu.value     = cmdToken[1];
		c_avu.unit      = cmdToken[2];
    }
    if(!new_avu.empty())
    {
    	// Tokenize query using ';' character
    	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
   	    boost::char_separator<char> sep(";\t\v\r\n", "", boost::keep_empty_tokens);
   	    tokenizer tokens(new_avu, sep);
   	    std::string cmdToken[3] = {"", "", ""};
   	    int token = 0;
		for (tokenizer::iterator tok_iter = tokens.begin();tok_iter != tokens.end(), token<3; ++tok_iter)
		{
			//Rprintf("%s: Current new_avu token: %s\n", command.c_str(), (*tok_iter).c_str());
			if(! (*tok_iter).empty() )
				cmdToken[token]       = *tok_iter;
			++token;
		}
		new_c_avu.attribute = cmdToken[0];
		new_c_avu.value     = cmdToken[1];
		new_c_avu.unit      = cmdToken[2];
    }

    // TODO : MOdify to retrieve a DataFrame object!
    result = doCommand_df(	Conn,
    						myEnv,
							status,
    						command,
							object_type, src_name,
							dst_object_type, dst_name,
							c_avu, new_c_avu,
							query );

    if( status < 0 ) {
    	Rf_error("[imeta::%s] Execution error %d\n", command.c_str(), status);
    }

    printErrorStack( Conn->rError );

    rcDisconnect( Conn );

    return( result ); // TODO : Return DataFrame? Now we return MetaData and let the stub functions convert to to DataFrame
}

//' imeta_qu
//' Query iRODS objects (files, collections) using their AVUs
//'
//' imeta_qu( type="d", query="distance '<=' 12")
//'
//' When querying dataObjects (d) or collections (C) additional conditions
//' (AttName Op AttVal) may be given separated by 'and', for example:
//'
//' imeta_qu( type="d", query="a = b and c '<' 10")
//'
//' Or a single 'or' can be given for the same AttName, for example
//'
//' imeta_qu( type="d", query="r '<' 5 or '>' 7")
//'
//' You can also query in numeric mode (instead of as strings) by adding 'n'
//' in front of the test condition, for example:
//'
//' imeta_qu( type="d", query="r 'n<' 123")
//'
//' which causes it to cast the AVU column to numeric (decimal) in the SQL.
//' In numeric mode, if any of the named AVU values are non-numeric, a SQL
//' error will occur but this avoids problems when comparing numeric strings
//' of different lengths.
//'
//' Other examples:
//'
//' imeta_qu( type="d", query="a like b%")
//'
//' returns data-objects with attribute 'a' with a value that starts with 'b'.
//'
//' imeta_qu( type="d", query="a like %")
//'
//' returns data-objects with attribute 'a' defined (with any value).
//'
//' @param src_type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param query    Query string expression on AVUs
//'
// [[Rcpp::export]]
Rcpp::DataFrame imeta_qu (
				const std::string& src_type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& query         // query string
			  )
{
	int status = 0;
	df::MetaData metas;

	metas = imeta_list(	status,
						"qu",
						src_type, ""/*src_name*/, ""/*dst_type*/, ""/*dst_name*/, ""/*avu*/, ""/*new_avu*/, query/*query*/);
	//metas.print("qu");
	return metas.to_df();
}

//' imeta_ls
//' imeta_ls( type=[d|C|R|u], name="object_name"[, attr="AttName"])
//'
//' List defined AVUs for the specified item
//'
//' imeta_ls( type="d", name="file1")
//'
//' If the optional AttName is included, it is the attribute name
//' you wish to list and only those will be listed.
//'
//' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param name iRODS object name - should be always present
//' @param attribute limit listing to values for a single attribute name defined by attr, blank for all
//'
// [[Rcpp::export]]
Rcpp::DataFrame imeta_ls  (
				const std::string& type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& name,     // iRODS object name - should be always present
				const std::string& attribute=""    // Format "Attribute;Value;Unit" (optional)
			  )
{
	int status = 0;
	df::MetaData metas;

	metas = imeta_list(	status,
						"ls",
						type, name, ""/*dst_type*/, ""/*dst_name*/, attribute/*atr*/, ""/*new_avu*/, ""/*query*/);
	//metas.print("ls");
	return metas.to_df();
}

//' imeta_lsw
//' imeta_lsw( type=[d|C|R|u], name="object_name"[, avu="AttName"])
//'
//' List defined AVUs for the specified item
//'
//' imeta_lsw( type="d", name="file1")
//'
//' If the optional AttName is included, it is the attribute name
//' you wish to list, doing so using wildcard matching.
//'
//' imeta_lsw(type="d", name="file1", avu="attr%")
//'
//' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param name iRODS object name - should be always present
//' @param avu  format "Attribute", using wildcards '%' and '_'
//'
// [[Rcpp::export]]
Rcpp::DataFrame imeta_lsw (
				const std::string& type,  // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& name,  // iRODS object name - should be always present
				const std::string& avu    // Format "Attribute"
			  )
{
	int status = 0;
	df::MetaData metas;

	metas = imeta_list(	status,
						"lsw",
						type, name, ""/*dst_type*/, ""/*dst_name*/, avu/*avu*/, ""/*new_avu*/, ""/*query*/);

	//metas.print("lsw");
	return metas.to_df();
}
