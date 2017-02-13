#include <Rcpp.h>
using namespace Rcpp;

// ienv
std::map<std::string, std::string> ienv();
RcppExport SEXP rirods_ienv() {
  BEGIN_RCPP
  Rcpp::RObject __result;
  Rcpp::RNGScope __rngScope;
  __result = Rcpp::wrap(ienv());
  return __result;
  END_RCPP
}

// ils
Rcpp::DataFrame ils(std::vector< std::string > args);
RcppExport SEXP rirods_ils(SEXP argsSEXP) {
  BEGIN_RCPP
  Rcpp::RObject __result;
  Rcpp::RNGScope __rngScope;
  Rcpp::traits::input_parameter< std::vector< std::string > >::type args(argsSEXP);
  __result = Rcpp::wrap(ils(args));
  return __result;
  END_RCPP
}
// isearch
Rcpp::DataFrame isearch(Rcpp::DataFrame constraints);
RcppExport SEXP rirods_isearch(SEXP constraintsSEXP) {
BEGIN_RCPP
    Rcpp::RObject __result;
    Rcpp::RNGScope __rngScope;
    Rcpp::traits::input_parameter< Rcpp::DataFrame >::type constraints(constraintsSEXP);
    __result = Rcpp::wrap(isearch(constraints));
    return __result;
END_RCPP
}

// iput
std::string iput(std::string src_path, std::string dest_path, std::string data_type, bool force, bool calculate_checksum, bool checksum, bool progress, bool verbose, std::string metadata, std::string acl);
RcppExport SEXP rirods_iput(SEXP src_pathSEXP, SEXP dest_pathSEXP, SEXP data_typeSEXP, SEXP forceSEXP, SEXP calculate_checksumSEXP, SEXP checksumSEXP, SEXP progressSEXP, SEXP verboseSEXP, SEXP metadataSEXP, SEXP aclSEXP) {
  BEGIN_RCPP
  Rcpp::RObject __result;
  Rcpp::RNGScope __rngScope;
  Rcpp::traits::input_parameter< std::string >::type src_path(src_pathSEXP);
  Rcpp::traits::input_parameter< std::string >::type dest_path(dest_pathSEXP);
  Rcpp::traits::input_parameter< std::string >::type data_type(data_typeSEXP);
  Rcpp::traits::input_parameter< bool >::type force(forceSEXP);
  Rcpp::traits::input_parameter< bool >::type calculate_checksum(calculate_checksumSEXP);
  Rcpp::traits::input_parameter< bool >::type checksum(checksumSEXP);
  Rcpp::traits::input_parameter< bool >::type progress(progressSEXP);
  Rcpp::traits::input_parameter< bool >::type verbose(verboseSEXP);
  Rcpp::traits::input_parameter< std::string >::type metadata(metadataSEXP);
  Rcpp::traits::input_parameter< std::string >::type acl(aclSEXP);
  __result = Rcpp::wrap(iput(src_path, dest_path, data_type, force, calculate_checksum, checksum, progress, verbose, metadata, acl));
  return __result;
  END_RCPP
}
// iget
std::string iget(std::string src_path, std::string dest_path, bool force, bool checksum, bool progress, bool verbose);
RcppExport SEXP rirods_iget(SEXP src_pathSEXP, SEXP dest_pathSEXP, SEXP forceSEXP, SEXP checksumSEXP, SEXP progressSEXP, SEXP verboseSEXP) {
  BEGIN_RCPP
  Rcpp::RObject __result;
  Rcpp::RNGScope __rngScope;
  Rcpp::traits::input_parameter< std::string >::type src_path(src_pathSEXP);
  Rcpp::traits::input_parameter< std::string >::type dest_path(dest_pathSEXP);
  Rcpp::traits::input_parameter< bool >::type force(forceSEXP);
  Rcpp::traits::input_parameter< bool >::type checksum(checksumSEXP);
  Rcpp::traits::input_parameter< bool >::type progress(progressSEXP);
  Rcpp::traits::input_parameter< bool >::type verbose(verboseSEXP);
  __result = Rcpp::wrap(iget(src_path, dest_path, force, checksum, progress, verbose));
  return __result;
  END_RCPP
}
// imeta_add
int imeta_add(const std::string& type, const std::string& name, const std::string& avu);
RcppExport SEXP rirods_imeta_add(SEXP typeSEXP, SEXP nameSEXP, SEXP avuSEXP) {
  BEGIN_RCPP
  Rcpp::RObject __result;
  Rcpp::RNGScope __rngScope;
  Rcpp::traits::input_parameter< const std::string& >::type type(typeSEXP);
  Rcpp::traits::input_parameter< const std::string& >::type name(nameSEXP);
  Rcpp::traits::input_parameter< const std::string& >::type avu(avuSEXP);
  __result = Rcpp::wrap(imeta_add(type, name, avu));
  return __result;
  END_RCPP
}
// imeta_addw
int imeta_addw(const std::string& type, const std::string& name, const std::string& avu);
RcppExport SEXP rirods_imeta_addw(SEXP typeSEXP, SEXP nameSEXP, SEXP avuSEXP) {
  BEGIN_RCPP
  Rcpp::RObject __result;
  Rcpp::RNGScope __rngScope;
  Rcpp::traits::input_parameter< const std::string& >::type type(typeSEXP);
  Rcpp::traits::input_parameter< const std::string& >::type name(nameSEXP);
  Rcpp::traits::input_parameter< const std::string& >::type avu(avuSEXP);
  __result = Rcpp::wrap(imeta_addw(type, name, avu));
  return __result;
  END_RCPP
}
// imeta_rm
int imeta_rm(const std::string& type, const std::string& name, const std::string& avu);
RcppExport SEXP rirods_imeta_rm(SEXP typeSEXP, SEXP nameSEXP, SEXP avuSEXP) {
  BEGIN_RCPP
  Rcpp::RObject __result;
  Rcpp::RNGScope __rngScope;
  Rcpp::traits::input_parameter< const std::string& >::type type(typeSEXP);
  Rcpp::traits::input_parameter< const std::string& >::type name(nameSEXP);
  Rcpp::traits::input_parameter< const std::string& >::type avu(avuSEXP);
  __result = Rcpp::wrap(imeta_rm(type, name, avu));
  return __result;
  END_RCPP
}
