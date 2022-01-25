#include <Rinterface.h>
#include <Rcpp.h>
#include <vector>
#include <string>
#include <sstream>
using namespace Rcpp;

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/*
 * ienv - The irods print environment utility
 */

#include "rodsClient.h"

//' r_ienv
//' Shows current iRODS environment settings
//'
std::map<std::string, std::string> ienv( ) {

  std::map<std::string, std::string> returnEnv;

  int status;
  rodsEnv myEnv;

//  Rprintf(  "Release Version = %s, API Version = %s\n",
//            RODS_REL_VERSION, RODS_API_VERSION );

  //  setenv( PRINT_RODS_ENV_STR, "1", 0 );
  status = getRodsEnv( &myEnv );

  if ( status < 0 ) {
    Rf_error("rirods exception (getRodsEnv error)");
  }

  returnEnv["Release Version"] = RODS_REL_VERSION;
  returnEnv["API Version"] = RODS_API_VERSION;
  returnEnv["irods_user_name"] = myEnv.rodsUserName;
  returnEnv["irods_host"] = myEnv.rodsHost;

  {
    std::ostringstream ostr;
    ostr << myEnv.rodsPort;
    returnEnv["irods_port"] = ostr.str();
  }

  returnEnv["irods_home"] = myEnv.rodsHome;
  returnEnv["irods_cwd"] = myEnv.rodsCwd;
  returnEnv["irods_authentication_scheme"] = myEnv.rodsAuthScheme;
  returnEnv["irods_default_resource"] = myEnv.rodsDefResource;
  returnEnv["irods_zone_name"] = myEnv.rodsZone;
  returnEnv["irods_client_server_policy"] = myEnv.rodsClientServerPolicy;
  returnEnv["irods_client_server_negotiation"] = myEnv.rodsClientServerNegotiation;

  {
  std::ostringstream ostr;
  ostr << myEnv.rodsEncryptionKeySize;
  returnEnv["irods_encryption_key_size"] = ostr.str();
  }
  {
  std::ostringstream ostr;
  ostr << myEnv.rodsEncryptionSaltSize;
  returnEnv["irods_encryption_salt_size"] = ostr.str();
  }
  {
  std::ostringstream ostr;
  ostr << myEnv.rodsEncryptionNumHashRounds;
  returnEnv["irods_encryption_num_hash_rounds"] = ostr.str();
  }

  returnEnv["irods_encryption_algorithm"] = myEnv.rodsEncryptionAlgorithm;

  return returnEnv;
}
