#include <Rinterface.h>
#include <Rcpp.h>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
// =-=-=-=-=-=-=-
// irods includes
#include "rods.h"
#include "parseCommandLine.h"
#include "rcMisc.h"
#include "rodsClient.h"

// =-=-=-=-=-=-=-
#include "irods_native_auth_object.hpp"
#include "irods_pam_auth_object.hpp"
#include "irods_gsi_object.hpp"
#include "irods_kvp_string_parser.hpp"
#include "irods_auth_constants.hpp"
//#include "irods_client_api_table.hpp"
//#include "irods_pack_table.hpp"
#include "irods_environment_properties.hpp"
#include "irods_kvp_string_parser.hpp"

#include "boost/lexical_cast.hpp"

#include "jansson.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

/* Uncomment the line below if you want TTL to be required for all
   users; i.e. all credentials will be time-limited.  This is only
   enforced on the client side so users can bypass this restriction by
   building their own iinit but it would strongly encourage the use of
   time-limited credentials. */
/* Uncomment the line below if you also want a default TTL if none
   is specified by the user. This TTL is specified in hours. */

#define TTYBUF_LEN 100
#define UPDATE_TEXT_LEN NAME_LEN*10

/*
 Attempt to make the ~/.irods directory in case it doesn't exist (may
 be needed to write the .irodsA file and perhaps the
 irods_environment.json file).
 */
int
mkrodsdir() {
    char dirName[NAME_LEN];
    int mode;
    char *getVar;
#ifdef windows_platform
    getVar = iRODSNt_gethome();
#else
    getVar = getenv( "HOME" );
#endif
    rstrcpy( dirName, getVar, NAME_LEN );
    rstrcat( dirName, "/.irods", NAME_LEN );
    mode = 0700;
#ifdef _WIN32
    iRODSNt_mkdir( dirName, mode );
#else
    int error_code = mkdir( dirName, mode );
    int errsv = errno;
    if ( error_code != 0 && errsv != EEXIST ) {
        rodsLog( LOG_NOTICE, "mkdir failed in mkrodsdir with error code %d", error_code );
    }
#endif
    return 0; /* no error messages as it normally fails */
}

void
printUpdateMsg() {
    Rprintf( "One or more fields in your iRODS environment file (irods_environment.json) are\n" );
    Rprintf( "missing; please enter them.\n" );
}

//' r_iinit
//' Performs the iRODS session login and obtains the session token
//' Unlike real 'iinit' command it does not perform user environment actions
//' @param p Secret password to be passed from R session
//'
// [[Rcpp::export]]
int iinit( std::string pass ) {

    rodsEnv my_env;
    rcComm_t *Conn = 0;
    rErrMsg_t errMsg;
    rodsArguments_t myRodsArgs;
    bool doingEnvFileUpdate = false;
    int status = 0;
    int i = 0;

    status = getRodsEnv( &my_env );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR, "main: getRodsEnv error. status = %d",
                 status );
        return 1;
    }

    int ttl = 0;

    char* password = (char*)pass.c_str();

    if ( myRodsArgs.longOption == True ) {
        /* just list the env */
        return 0;
    }

    json_t* json_env = json_object();
    if ( !json_env ) {
        Rprintf( "call to json_object() failed.\n" );
        return SYS_MALLOC_ERR;
    }

    /*
       Check on the key Environment values, prompt and save
       them if not already available.
     */
    if ( strlen( my_env.rodsHost ) == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        Rprintf( "Enter the host name (DNS) of the server to connect to: " );
        std::string response;
        getline( std::cin, response );
        snprintf(
            my_env.rodsHost,
            NAME_LEN,
            "%s",
            response.c_str() );
        json_object_set(
            json_env,
            "irods_host",
            json_string( my_env.rodsHost ) );
    }
    if ( my_env.rodsPort == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        Rprintf( "Enter the port number: " );
        std::string response;
        getline( std::cin, response );
        try {
            my_env.rodsPort = boost::lexical_cast< int >( response );
        }
        catch ( const boost::bad_lexical_cast& ) {
            my_env.rodsPort = 0;
        }

        json_object_set(
            json_env,
            "irods_port",
            json_integer( my_env.rodsPort ) );
    }
    if ( strlen( my_env.rodsUserName ) == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        Rprintf( "Enter your irods user name: " );
        std::string response;
        getline( std::cin, response );
        snprintf(
            my_env.rodsUserName,
            NAME_LEN,
            "%s",
            response.c_str() );
        json_object_set(
            json_env,
            "irods_user_name",
            json_string( my_env.rodsUserName ) );
    }
    if ( strlen( my_env.rodsZone ) == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        Rprintf( "Enter your irods zone: " );
        std::string response;
        getline( std::cin, response );
        snprintf(
            my_env.rodsZone,
            NAME_LEN,
            "%s",
            response.c_str() );
        json_object_set(
            json_env,
            "irods_zone_name",
            json_string( my_env.rodsZone ) );
    }
    if ( strlen( my_env.rodsAuthScheme ) == 0 ) {
        if ( !doingEnvFileUpdate ) {
            doingEnvFileUpdate = true;
            printUpdateMsg();
        }
        Rprintf( "Enter your irods authentication scheme: " );
        std::string response;
        getline( std::cin, response );
        snprintf(
            my_env.rodsAuthScheme,
            NAME_LEN,
            "%s",
            response.c_str() );
        json_object_set(
            json_env,
            irods::CFG_IRODS_AUTHENTICATION_SCHEME_KW.c_str(),
            json_string( my_env.rodsAuthScheme ) );
    }

    if ( doingEnvFileUpdate ) {
        Rprintf( "Those values will be added to your environment file (for use by\n" );
        Rprintf( "other iCommands) if the login succeeds.\n\n" );
    }

    /*
      Now, get the password
     */
    // =-=-=-=-=-=-=-
    // ensure scheme is lower case for comparison
    std::string lower_scheme = my_env.rodsAuthScheme;
    std::transform(
        lower_scheme.begin(),
        lower_scheme.end(),
        lower_scheme.begin(),
        ::tolower );

    int useGsi = 0;
    if ( irods::AUTH_GSI_SCHEME == lower_scheme ) {
        useGsi = 1;
    }

    if ( irods::AUTH_PAM_SCHEME == lower_scheme ) {
    }

    if ( strcmp( my_env.rodsUserName, ANONYMOUS_USER ) == 0 ) {
    }
    if ( useGsi == 1 ) {
        Rprintf( "Using GSI, attempting connection/authentication\n" );
    }

    i = obfSavePw( 0, 0, 0, password );

    if ( i != 0 ) {
        rodsLogError( LOG_ERROR, i, (char*)("Save Password failure") );
        return 1;
    }

    // =-=-=-=-=-=-=-
    // initialize pluggable api table
    //irods::api_entry_table&  api_tbl = irods::get_client_api_table();
    //irods::pack_entry_table& pk_tbl  = irods::get_pack_table();
    //init_api_table( api_tbl, pk_tbl );

    /* Connect... */
    Conn = rcConnect( my_env.rodsHost, my_env.rodsPort, my_env.rodsUserName,
                      my_env.rodsZone, 0, &errMsg );
    if ( Conn == NULL ) {
        rodsLog( LOG_ERROR,
                 "Saved password, but failed to connect to server %s",
                 my_env.rodsHost );
        return 2;
    }

    // =-=-=-=-=-=-=-
    // PAM auth gets special consideration, and also includes an
    // auth by the usual convention
    bool pam_flg = false;
    if ( irods::AUTH_PAM_SCHEME == lower_scheme ) {
        // =-=-=-=-=-=-=-
        // set a flag stating that we have done pam and the auth
        // scheme needs overridden
        pam_flg = true;

        // =-=-=-=-=-=-=-
        // build a context string which includes the ttl and password
        std::stringstream ttl_str;  ttl_str << ttl;
		irods::kvp_map_t ctx_map;
		ctx_map[ irods::AUTH_TTL_KEY ] = ttl_str.str();
		ctx_map[ irods::AUTH_PASSWORD_KEY ] = password;
		std::string ctx_str = irods::escaped_kvp_string(ctx_map);

        // =-=-=-=-=-=-=-
        // pass the context with the ttl as well as an override which
        // demands the pam authentication plugin
        status = clientLogin( Conn, ctx_str.c_str(), irods::AUTH_PAM_SCHEME.c_str() );
        if ( status != 0 ) {
            return 8;
        }

        // =-=-=-=-=-=-=-
        // if this succeeded, do the regular login below to check
        // that the generated password works properly.
    } // if pam

    // =-=-=-=-=-=-=-
    // since we might be using PAM
    // and check that the user/password is OK
    const char* auth_scheme = ( pam_flg ) ?
                              irods::AUTH_NATIVE_SCHEME.c_str() :
                              my_env.rodsAuthScheme;

    status = clientLogin( Conn, 0, auth_scheme );
    if ( status != 0 ) {
        rcDisconnect( Conn );
        return 7;
    }

    printErrorStack( Conn->rError );

    if ( ttl > 0 && !pam_flg ) {
        /* if doing non-PAM TTL, now get the
        short-term password (after initial login) */
        status = clientLoginTTL( Conn, ttl );
        if ( status != 0 ) {


            rcDisconnect( Conn );
            return 8;
        }
        /* And check that it works */
        status = clientLogin( Conn );
        if ( status != 0 ) {


            rcDisconnect( Conn );
            return 7;
        }
    }

    rcDisconnect( Conn );

    /* Save updates to irods_environment.json. */
    if ( doingEnvFileUpdate ) {
        std::string env_file, session_file;
        irods::error ret = irods::environment_properties::get_json_environment_file(
                               env_file,
                               session_file );
        if ( ret.ok() ) {
            json_error_t error;
            json_t *current_contents = json_load_file( env_file.c_str(), 0, &error );

            json_t *obj_to_dump = 0;
            if ( current_contents ) {
                int ret = json_object_update( current_contents, json_env );
                if ( ret == 0 ) {
                    obj_to_dump = current_contents;
                }
                else {
                    obj_to_dump = json_env;
                    Rcerr << "Failed to update " << env_file.c_str() << std::endl;
                    Rcerr << error.text << std::endl;
                    Rcerr << error.source << std::endl;
                    Rcerr << error.line << ":" << error.column << " " << error.position << std::endl;
                }
            }
            else {
                obj_to_dump = json_env;
            }

            char* tmp_buf = json_dumps( obj_to_dump, JSON_INDENT( 4 ) );
            std::ofstream f( env_file.c_str(), std::ios::out );
            if ( f.is_open() ) {
                f << tmp_buf << std::endl;
                f.close();
            }
            else {
                Rprintf(
                    "failed to open environment file [%s]\n",
                    env_file.c_str() );
            }

            if ( current_contents ) {
                json_decref( current_contents );
            }
        }
        else {
            Rprintf( "failed to get environment file - %ji\n", ( intmax_t )ret.code() );
        }
    } // if doingEnvFileUpdate

    return 0;
}
