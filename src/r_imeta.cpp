/*
 * r_imeta.cpp
 *
 *  Created on: Jan 20, 2016
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

#define MAX_SQL 300
#define BIG_STR 3000

char cwd[BIG_STR];

int debug = 0;
int testMode = 0; /* some particular internal tests */
int longMode = 0; /* more detailed listing */
int upperCaseFlag = 0;

char zoneArgument[MAX_NAME_LEN + 2] = "";

rcComm_t *Conn;
rodsEnv myEnv;

int lastCommandStatus = 0;
int printCount = 0;

/*
 print the results of a general query.
 */
void printGenQueryResults( char* cmd, rcComm_t *Conn, int status, genQueryOut_t *genQueryOut, char *descriptions[] )
{
    int i, j;
    char localTime[TIME_LEN];
    lastCommandStatus = status;
    if ( status == CAT_NO_ROWS_FOUND ) {
        lastCommandStatus = 0;
    }
    if ( status != 0 && status != CAT_NO_ROWS_FOUND ) {
        Rf_error( "[imeta::%s] rcGenQuery: %d\n", cmd, status );
    }
    else {
        if ( status == CAT_NO_ROWS_FOUND ) {
            if ( printCount == 0 ) {
                Rf_error( "[imeta::%s] No rows found\n", cmd );
            }
        }
        else {
            for ( i = 0; i < genQueryOut->rowCnt; i++ ) {
                if ( i > 0 ) {
                    Rprintf( "----\n" );
                }
                for ( j = 0; j < genQueryOut->attriCnt; j++ ) {
                    char *tResult;
                    tResult = genQueryOut->sqlResult[j].value;
                    tResult += i * genQueryOut->sqlResult[j].len;
                    if ( *descriptions[j] != '\0' ) {
                        if ( strstr( descriptions[j], "time" ) != 0 ) {
                            getLocalTimeFromRodsTime( tResult, localTime );
                            Rprintf( "%s: %s\n", descriptions[j],
                                    localTime );
                        }
                        else {
                            Rprintf( "%s: %s\n", descriptions[j], tResult );
                            printCount++;
                        }
                    }
                }
            }
        }
    }
}

/*
 Via a general query and show the AVUs for a dataobject.
 */
int showDataObj( char* cmd, char *name, char *attrName, int wild )
{
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

    printf( "[imeta::%s] AVUs defined for dataObj %s:\n", cmd, name );
    printCount = 0;
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
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            lastCommandStatus = status;
            Rf_error( "[imeta::%s] Dataobject %s does not exist\n", cmd, fullName );
            Rf_error( "[imeta::%s] or, if 'strict' access control is enabled, you may not have access.\n", cmd );
            return 0;
        }
        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }
    else {
        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }

        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    return 0;
}

/*
Via a general query, show the AVUs for a collection
*/
int showColl( char* cmd, char *name, char *attrName, int wild )
{
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];

    char fullName[MAX_NAME_LEN];
    int  status;
    char *columnNames[] = {(char*)"attribute", (char*)"value", (char*)"units"};

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    Rprintf( "[imeta::%s] AVUs defined for collection %s:\n", cmd, name );
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
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            lastCommandStatus = status;
            Rf_error( "[imeta::%s] Collection %s does not exist.\n", cmd, fullName );
            return 0;
        }
    }

    printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }

        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    return 0;
}

/*
Via a general query, show the AVUs for a resource
*/
int showResc( char* cmd, char *name, char *attrName, int wild )
{
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

    Rprintf( "[imeta::%s] AVUs defined for resource %s:\n", cmd, name );

    printCount = 0;
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
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            lastCommandStatus = status;
            Rf_error( "[imeta::%s] Resource %s does not exist.\n", cmd, name );
            return 0;
        }
    }

    printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }
        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    return 0;
}

/*
Via a general query, show the AVUs for a user
*/
int showUser( char* cmd, char *name, char *attrName, int wild )
{
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
        Rf_error( "[imeta::%s] sizeof( userZone ) %s\n", cmd, myEnv.rodsZone );
    }

    genQueryInp_t genQueryInp;
    memset( &genQueryInp, 0, sizeof( genQueryInp ) );
    if ( upperCaseFlag ) {
        genQueryInp.options = UPPER_CASE_WHERE;
    }

    Rprintf( "[imeta::%s] AVUs defined for user %s#%s:\n", cmd, userName, userZone );

    printCount = 0;
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
            return 0;
        }
        if ( status == CAT_NO_ROWS_FOUND ) {
            lastCommandStatus = status;
            Rf_error( "[imeta::%s] User %s does not exist.\n", cmd, name );
            return 0;
        }
    }

    printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }
        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    return 0;
}

/*
Do a query on AVUs for dataobjs and show the results
attribute op value [AND attribute op value] [REPEAT]
 */
int queryDataObj( char* cmd, char *cmdToken[] )
{
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

    printCount = 0;
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
        return -2;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }
        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    return 0;
}

/*
Do a query on AVUs for collections and show the results
 */
int queryCollection( char* cmd, char *cmdToken[] )
{
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

    printCount = 0;
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
        return -2;
    }

    genQueryInp.maxRows = 10;
    genQueryInp.continueInx = 0;
    genQueryInp.condInput.len = 0;

    if ( zoneArgument[0] != '\0' ) {
        addKeyVal( &genQueryInp.condInput, ZONE_KW, zoneArgument );
    }

    status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );

    printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }
        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    return 0;
}


/*
Do a query on AVUs for resources and show the results
 */
int queryResc( char* cmd, char *attribute, char *op, char *value )
{
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

    printCount = 0;
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

    printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }
        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    return 0;
}

/*
Do a query on AVUs for users and show the results
 */
int queryUser( char* cmd, char *attribute, char *op, char *value )
{
    genQueryOut_t *genQueryOut;
    int i1a[10];
    int i1b[10];
    int i2a[10];
    char *condVal[10];
    int status;
    char *columnNames[] = {(char*)"user", (char*)"zone"};

    printCount = 0;
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

    printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );

    while ( status == 0 && genQueryOut->continueInx > 0 ) {
        genQueryInp.continueInx = genQueryOut->continueInx;
        status = rcGenQuery( Conn, &genQueryInp, &genQueryOut );
        if ( genQueryOut->rowCnt > 0 ) {
            Rprintf( "----\n" );
        }
        printGenQueryResults( cmd, Conn, status, genQueryOut, columnNames );
    }

    return 0;
}


/*
 Modify (copy) AVUs
 */
int modCopyAVUMetadata( char *arg0, char *arg1, char *arg2, char *arg3,
                        char *arg4, char *arg5, char *arg6, char *arg7 )
{
    modAVUMetadataInp_t modAVUMetadataInp;
    int status;
    char fullName1[MAX_NAME_LEN];
    char fullName2[MAX_NAME_LEN];

    if ( strcmp( arg1, "-R" ) == 0 || strcmp( arg1, "-r" ) == 0 ||
            strcmp( arg1, "-u" ) == 0 ) {
        snprintf( fullName1, sizeof( fullName1 ), "%s", arg3 );
    }
    else if ( strlen( arg3 ) > 0 ) {
        if ( *arg3 == '/' ) {
            snprintf( fullName1, sizeof( fullName1 ), "%s", arg3 );
        }
        else {
            snprintf( fullName1, sizeof( fullName1 ), "%s/%s", cwd, arg3 );
        }
    }
    else {
        snprintf( fullName1, sizeof( fullName1 ), "%s", cwd );
    }

    if ( strcmp( arg2, "-R" ) == 0 || strcmp( arg2, "-r" ) == 0 ||
            strcmp( arg2, "-u" ) == 0 ) {
        snprintf( fullName2, sizeof( fullName2 ), "%s", arg4 );
    }
    else if ( strlen( arg4 ) > 0 ) {
        if ( *arg4 == '/' ) {
            snprintf( fullName2, sizeof( fullName2 ), "%s", arg4 );
        }
        else {
            snprintf( fullName2, sizeof( fullName2 ), "%s/%s", cwd, arg4 );
        }
    }
    else {
        snprintf( fullName2, sizeof( fullName2 ), "%s", cwd );
    }

    modAVUMetadataInp.arg0 = arg0;
    modAVUMetadataInp.arg1 = arg1;
    modAVUMetadataInp.arg2 = arg2;
    modAVUMetadataInp.arg3 = fullName1;
    modAVUMetadataInp.arg4 = fullName2;
    modAVUMetadataInp.arg5 = arg5;
    modAVUMetadataInp.arg6 = arg6;
    modAVUMetadataInp.arg7 = arg7;
    modAVUMetadataInp.arg8 = (char*)"";
    modAVUMetadataInp.arg9 = (char*)"";

    status = rcModAVUMetadata( Conn, &modAVUMetadataInp );
    lastCommandStatus = status;

    if ( status < 0 ) {
        if ( Conn->rError ) {
            rError_t *Err;
            rErrMsg_t *ErrMsg;
            int i, len;
            Err = Conn->rError;
            len = Err->len;
            for ( i = 0; i < len; i++ ) {
                ErrMsg = Err->errMsg[i];
                Rf_error( "[imeta::%s] Level %d: %s", i, arg0, ErrMsg->msg );
            }
        }
        char *mySubName = NULL;
        const char *myName = rodsErrorName( status, &mySubName );
        Rf_error( "[imeta::%s] rcModAVUMetadata failed with error %d %s %s", arg0, status, myName, mySubName );
        //free( mySubName );
    }

    if ( status == CAT_UNKNOWN_FILE ) {
        char tempName[MAX_NAME_LEN] = "/";
        int len;
        int isRemote = 0;
        strncat( tempName, myEnv.rodsZone, MAX_NAME_LEN - strlen( tempName ) );
        len = strlen( tempName );
        if ( strncmp( tempName, fullName1, len ) != 0 ) {
        	Rf_error( "[imeta::%s] Cannot copy metadata from a remote zone.\n", arg0 );
            isRemote = 1;
        }
        if ( strncmp( tempName, fullName2, len ) != 0 ) {
        	Rf_error( "[imeta::%s] Cannot copy metadata to a remote zone.\n", arg0 );
            isRemote = 1;
        }
        if ( isRemote ) {
        	Rf_error( "[imeta::%s] Copying of metadata is done via SQL within each ICAT\n", arg0 );
        	Rf_error( "[imeta::%s] for efficiency.  Copying metadata between zones is\n", arg0 );
        	Rf_error( "[imeta::%s] not implemented.\n", arg0 );
        }
    }
    return status;
}

/*
 Modify (add or remove) AVUs
 */
int
modAVUMetadata( char *arg0, char *arg1, char *arg2, char *arg3,
                char *arg4, char *arg5, char *arg6, char *arg7, char *arg8 )
{
    modAVUMetadataInp_t modAVUMetadataInp;
    int status;
    char fullName[MAX_NAME_LEN];

    if ( strcmp( arg1, "-R" ) == 0 || strcmp( arg1, "-r" ) == 0 ||
            strcmp( arg1, "-u" ) == 0 ) {
        snprintf( fullName, sizeof( fullName ), "%s", arg2 );
    }
    else if ( strlen( arg2 ) > 0 ) {
        if ( *arg2 == '/' ) {
            snprintf( fullName, sizeof( fullName ), "%s", arg2 );
        }
        else {
            snprintf( fullName, sizeof( fullName ), "%s/%s", cwd, arg2 );
        }
    }
    else {
        snprintf( fullName, sizeof( fullName ), "%s", cwd );
    }

    modAVUMetadataInp.arg0 = arg0;
    modAVUMetadataInp.arg1 = arg1;
    modAVUMetadataInp.arg2 = fullName;
    modAVUMetadataInp.arg3 = arg3;
    modAVUMetadataInp.arg4 = arg4;
    modAVUMetadataInp.arg5 = arg5;
    modAVUMetadataInp.arg6 = arg6;
    modAVUMetadataInp.arg7 = arg7;
    modAVUMetadataInp.arg8 = arg8;
    modAVUMetadataInp.arg9 = (char*)"";

    status = rcModAVUMetadata( Conn, &modAVUMetadataInp );
    lastCommandStatus = status;

    if ( status < 0 ) {
        if ( Conn->rError ) {
            rError_t *Err;
            rErrMsg_t *ErrMsg;
            int i, len;
            Err = Conn->rError;
            len = Err->len;
            for ( i = 0; i < len; i++ ) {
                ErrMsg = Err->errMsg[i];
                Rf_error( "[imeta::%s] Level %d: %s", arg0, i, ErrMsg->msg );
            }
        }
        char *mySubName = NULL;
        const char *myName = rodsErrorName( status, &mySubName );
        Rf_error("[imeta::%s] rcModAVUMetadata failed with error %d: %s %s", arg0, status, myName, mySubName);
        //free( mySubName );
    }
    return status;
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

/* handle a command,
   return code is 0 if the command was (at least partially) valid,
   -1 for quitting,
   -2 for if invalid
   -3 if empty.
 */
int doCommand(
				const std::string& command,                     // add*, ls*, rm*, cp, qu
				const std::string& object_type,                 // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& src_name,                    // iRODS object name - should be always present
				const std::string& dst_object_type="",          // Destination object type (cp only)
				const std::string& dst_name ="",                // iRODS object name(cp only)
				const i_meta::AVU& avu      =i_meta::empty_avu, // Default is empty, but it is always supplied by imeta function
				const i_meta::AVU& new_avu  =i_meta::empty_avu, // Default is empty, but it is supplied by imeta function
				const std::string& query    =""                 // free style string for the moment (qu only)
			)
{
    if(
    	command.empty()     ||    // No command, no type, no work done
    	object_type.empty()
	  )
    {
    	Rf_error("[imeta::???] Command and object type is required!\n");
    	return(-3);
    }

    // Prefix object_type with "-" as it is sent to iRODS server for processing
    std::string normalized_object_type = std::string("-")+object_type;
    std::string normalized_dst_object_type = std::string("-")+dst_object_type;

    if ( command == "add" || command == "adda" || command == "addw")
    {
        int myStat = modAVUMetadata (
									(char*)command.c_str(),
									(char*)normalized_object_type.c_str(),
									(char*)src_name.c_str(),
									(char*)avu.attribute.c_str(),
									(char*)avu.value.c_str(),
									(char*)avu.unit.c_str(),
									(char*)"",
									(char*)"",
									(char*)""
									);
        if ( myStat > 0 ) {
            Rprintf( "[imeta::%s] AVU added to %d data-objects\n", command.c_str(), myStat );
        }
        return 0;
    }

    if ( command == "rm" || command == "rmw") // This is a test feature || command == "rmi")
    {
		int myStat = modAVUMetadata (
									(char*)command.c_str(),
									(char*)normalized_object_type.c_str(),
									(char*)src_name.c_str(),
									(char*)avu.attribute.c_str(),
									(char*)avu.value.c_str(),
									(char*)avu.unit.c_str(),
									(char*)"",
									(char*)"",
									(char*)""
									);
        if ( myStat > 0 ) {
            Rprintf( "[imeta::%s] AVU removed from %d data-objects\n", command.c_str(), myStat );
        }
        return 0;
    }
    if ( command == "mod" )
    {
    	// Normalize new AVU data
    	std::string n_avu_a = "";
    	std::string n_avu_v = "";
    	std::string n_avu_u = "";

    	if(! new_avu.attribute.empty() )
    		n_avu_a = "n:"+new_avu.attribute;
    	if(! new_avu.value.empty() )
    		n_avu_v = "v:"+new_avu.value;
    	if(! new_avu.unit.empty() )
    		n_avu_u = "u:"+new_avu.unit;

        int myStat = modAVUMetadata (
									(char*)command.c_str(),
									(char*)normalized_object_type.c_str(),
									(char*)src_name.c_str(),
									(char*)avu.attribute.c_str(),
									(char*)avu.value.c_str(),
									(char*)avu.unit.c_str(),
									(char*)n_avu_a.c_str(),
									(char*)n_avu_v.c_str(),
									(char*)n_avu_u.c_str()
									);
        if ( myStat > 0 ) {
            Rprintf( "[imeta::%s] AVU modified for %d data-object %s\n", command.c_str(), myStat, src_name.c_str() );
        }
        return 0;
    }
    if ( command == "set" )
    {
    	// Normalize new AVU data
    	std::string n_avu_a = "";
    	std::string n_avu_v = "";
    	std::string n_avu_u = "";

    	if(! new_avu.attribute.empty() )
    		n_avu_a = new_avu.attribute;
    	if(! new_avu.value.empty() )
    		n_avu_v = new_avu.value;
    	if(! new_avu.unit.empty() )
    		n_avu_u = new_avu.unit;

    	int myStat = modAVUMetadata (
									(char*)command.c_str(),
									(char*)normalized_object_type.c_str(),
									(char*)src_name.c_str(),
									(char*)n_avu_a.c_str(),
									(char*)n_avu_v.c_str(),
									(char*)n_avu_u.c_str(),
									(char*)"",
									(char*)"",
									(char*)""
									);
        if ( myStat > 0 ) {
            Rprintf( "[imeta::%s] AVU set for %d data-object %s\n", command.c_str(), myStat, src_name.c_str() );
        }
        return 0;
    }

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
            showDataObj( (char*)command.c_str(), (char*)src_name.c_str(), (char*)avu.attribute.c_str(), wild );
            return 0;
        }
        if ( object_type == "c" || object_type == "C")
		{
            showColl( (char*)command.c_str(), (char*)src_name.c_str(), (char*)avu.attribute.c_str(), wild );
            return 0;
        }
        if ( object_type == "r" || object_type == "R")
		{
            showResc( (char*)command.c_str(), (char*)src_name.c_str(), (char*)avu.attribute.c_str(), wild );
            return 0;
        }
        if ( command == "u" )
        {
            showUser( (char*)command.c_str(), (char*)src_name.c_str(), (char*)avu.attribute.c_str(), wild );
            return 0;
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
					return 0;
				}
				Rf_error( "[imeta::%s] %s\n", command.c_str(), msgs[i] );
			}
    		return(-2);
    	}

    	// Tokenize query using space character
    	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
   	    boost::char_separator<char> sep("\t\v\r\n", "", boost::keep_empty_tokens);
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

        int status;
        if ( object_type == "d" )
        {
            status = queryDataObj( (char*)command.c_str(), cmdToken );
            if ( status < 0 ) {
                return -2;
            }
            return 0;
        }
        if ( object_type == "C" )
        {
            status = queryCollection( (char*)command.c_str(), cmdToken );
            if ( status < 0 ) {
                return -2;
            }
            return 0;
        }
        if ( object_type == "R" || object_type == "r" )
        {
            if ( *cmdToken[5] != '\0' ) {
                Rf_error( "[imeta::%s] Unrecognized input\n", command.c_str() );
                return -2;
            }
            queryResc( (char*)command.c_str(), cmdToken[2], cmdToken[3], cmdToken[4] );
            return 0;
        }
        if ( object_type == "u" )
        {
            queryUser( (char*)command.c_str(), cmdToken[2], cmdToken[3], cmdToken[4] );
            return 0;
        }
    }

    if ( command == "cp" )
    {
        modCopyAVUMetadata (
							(char*)command.c_str(),
							(char*)normalized_object_type.c_str(),
							(char*)normalized_dst_object_type.c_str(),
							(char*)src_name.c_str(),
							(char*)dst_name.c_str(),
							(char*)"",
							(char*)"",
							(char*)""
							);
//        modCopyAVUMetadata( command, cmdToken[1], cmdToken[2],
//                            cmdToken[3], cmdToken[4], cmdToken[5],
//                            cmdToken[6], cmdToken[7] );
        return 0;
    }

    if ( ! command.empty() )
    {
        Rf_error( "[imeta] Unrecognized subcommand '%s', try 'imeta help'\n", command.c_str() );
        return -2;
    }

    return -3;
}

//' imeta
//' Retrieves the file from iRODS
//' @param command         add, ls, rm, cp, qu
//' @param object_type     iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param dst_object_type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param dst_name        iRODS object name(cp only)
//' @param avu             format "Attribute;Value;Unit"
//' @param new_avu         format "Attribute;Value;Unit"
//' @param query           See, icommand help 'imeta help qu' for details, help is printed if empty
//'
//                             //[[Rcpp::export]]
int imeta
		(
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
    int status;
    rErrMsg_t errMsg;

    rodsArguments_t myRodsArgs;

    // We have to reset all to 0s because we do not use
    // the "parse_program_options(...)" function
    // The mvUtil(...) function expects positional arguments parser initializing it beforehand
    memset( &myRodsArgs, 0, sizeof( rodsArguments_t ) );

    status = getRodsEnv( &myEnv );
    if ( status < 0 ) {
        Rf_error( "[imeta] getRodsEnv error. status = %d", status);
        return( 1 );
    }

    strncpy( cwd, myEnv.rodsCwd, BIG_STR );
    if ( strlen( cwd ) == 0 ) {
        strcpy( cwd, "/" );
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
        return( 2 );
    }

    status = clientLogin( Conn );
    if ( status != 0 ) {
        if ( !debug ) {
            return( 3 );
        }
    }

    i_meta::AVU     c_avu = i_meta::empty_avu;
    i_meta::AVU new_c_avu = i_meta::empty_avu;

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

    status = doCommand(	command,
						object_type, src_name,
						dst_object_type, dst_name,
						c_avu, new_c_avu,
						query );

    if( status < 0 ) {
    	Rf_error("[imeta::%s] Execution error %d\n", command.c_str(), status);
    }

    printErrorStack( Conn->rError );

    rcDisconnect( Conn );

    if ( lastCommandStatus != 0 ) {
        return( 4 );
    }

    return( 0 );
}

//' imeta_add
//' Adds a new metadata AVU to the selected iRODS object of the given type
//' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param name iRODS object name - should be always present
//' @param avu  format "Attribute;Value;Unit"
//'
int imeta_add (
				const std::string& type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& name,     // iRODS object name - should be always present
				const std::string& avu       // Format "Attribute;Value;Unit"
			  )
{
	int status = 0;

	status = imeta("add", type, name, ""/*dst_type*/, ""/*dst_name*/, avu/*avu*/, ""/*new_avu*/,""/*query*/);

	return status;
}

//' Add new metadata
//'
//' Adds a new metadata AVU to the selected iRODS object of the given type
//'
//' @param type iRODS files (`dD`), collections (`cC`), resources (`rR`), users (`u`)
//' @param name iRODS object name - should be always present, one can use wildcards %, _ on object name
//' @param avu  format "Attribute;Value;Unit"
//'
int imeta_addw (
				const std::string& type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& name,     // iRODS object name - should be always present
				const std::string& avu       // Format "Attribute;Value;Unit"
			  )
{
	int status = 0;

	status = imeta("addw", type, name, ""/*dst_type*/, ""/*dst_name*/, ""/*avu*/, avu/*new_avu*/,""/*query*/);

	return status;
}

//' imeta_rm
//' Removes the metadata AVU from the selected iRODS object of the given type
//' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param name iRODS object name - should be always present
//' @param avu  format "Attribute;Value;Unit", unit must be present if exists (iRODS constraint)
//'
int imeta_rm  (
				const std::string& type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& name,     // iRODS object name - should be always present
				const std::string& avu       // Format "Attribute;Value;Unit"
			  )
{
	int status = 0;

	status = imeta("rm", type, name, ""/*dst_type*/, ""/*dst_name*/, avu/*avu*/, ""/*new_avu*/,""/*query*/);

	return status;
}

//' imeta_rmw
//' Removes the metadata AVU from the selected iRODS object of the given type
//' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param name iRODS object name - should be always present
//' @param avu  format "Attribute;Value;Unit", one can use wildcards %, _ on attribute, value, unit
//'
// [[Rcpp::export]]
int imeta_rmw (
				const std::string& type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& name,     // iRODS object name - should be always present
				const std::string& avu       // Format "Attribute;Value;Unit" - one can use wildcards %, _
			  )
{
	int status = 0;

	status = imeta("rmw", type, name, ""/*dst_type*/, ""/*dst_name*/, avu/*avu*/, ""/*new_avu*/,""/*query*/);

	return status;
}

//' imeta_cp
//' Copies the metadata AVUs from the selected iRODS object of the given type to another object
//' @param src_type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param dst_type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param src_name iRODS source object name - should be always present
//' @param dst_name iRODS destination object name - should be always present
//'
// [[Rcpp::export]]
int imeta_cp (
				const std::string& src_type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& dst_type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& src_name,     // iRODS source object name - should be always present
				const std::string& dst_name      // iRODS destination object name - should be always present
			  )
{
	int status = 0;

	status = imeta("cp", src_type, src_name, dst_type/*dst_type*/, dst_name/*dst_name*/, ""/*avu*/, ""/*new_avu*/,""/*query*/);

	return status;
}

//' imeta_mod
//' Modifies the metadata AVUs of the selected iRODS object of the given type
//' @param src_type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param src_name iRODS source object name - should be always present
//' @param old_avu  format "Attribute;Value;Unit", e.g. "Attr1;Value1;" or "Attr1;Value1;Unit1"
//' @param new_avu  format "Attribute;Value;Unit", e.g. "AttrNew;12;" or "AttrNew;12;mm"
//'
// [[Rcpp::export]]
int imeta_mod (
				const std::string& src_type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& src_name,     // iRODS source object name - should be always present
				const std::string& old_avu,      // iRODS original metadata
				const std::string& new_avu       // iRODS new metadata
			  )
{
	int status = 0;

	status = imeta("mod", src_type, src_name, ""/*dst_type*/, ""/*dst_name*/, old_avu/*avu*/, new_avu/*new_avu*/,""/*query*/);

	return status;
}

//' imeta_set
//' Set the newValue (and newUnit) of an AVU of a dataobj (d), collection(C),
//' resource(R) or user(u).
//'
//' 'imeta_set' modifies an AVU if it exists, or creates one if it does not.
//' If the AttName does not exist, or is used by multiple objects, the AVU for
//' this object is added.
//'
//' If the AttName is used only by this one object, the AVU (row) is modified
//' with the new values, reducing the database overhead (unused rows).
//'
//' imeta_set( type="d", name="file1", avu="distance;12")
//'
//' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
//' @param name iRODS object name - should be always present
//' @param avu  format "Attribute;Value;Unit"
//'
// [[Rcpp::export]]
int imeta_set (
				const std::string& type,     // iRODS files/dD, collections/cC, resources/rR, users/uU
				const std::string& name,     // iRODS object name - should be always present
				const std::string& avu       // Format "Attribute;Value;Unit"
			  )
{
	int status = 0;

	status = imeta("set", type, name, ""/*dst_type*/, ""/*dst_name*/, ""/*avu*/, avu/*new_avu*/, ""/*query*/);

	return status;
}

