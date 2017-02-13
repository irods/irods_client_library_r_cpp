#' r_ienv
#' Shows current iRODS environment settings
#'
ienv <- function() {
  x <- .Call('rirods_ienv', PACKAGE = 'rirods')
  return(as.list(x))
}

#' ils
#' Lists collections and data objects in iRODS
#' @param args "ils" one or several iRODS data-object (file) or collection (directory) paths
#' @return A data frame with information about collections and data objects under the supplied paths
ils <- function(args="") {
  x <- .Call('rirods_ils', PACKAGE = 'rirods', args = args)
  x$Modify_time <- as.POSIXct(as.numeric(as.character(x$Modify_time)),origin="1970-01-01")
  x$Create_time <- as.POSIXct(as.numeric(as.character(x$Create_time)),origin="1970-01-01")
  return(x)
}

#' isearch
#' Lists collections and data objects in iRODS
#' @param args "isearch" icommand params
#'
isearch <- function(constraints) {
  x <- .Call('rirods_isearch', PACKAGE = 'rirods', constraints)
  x$Modify_time <- as.POSIXct(as.numeric(as.character(x$Modify_time)),origin="1970-01-01")
  x$Create_time <- as.POSIXct(as.numeric(as.character(x$Create_time)),origin="1970-01-01")
  return(x)
}

#' iput
#' Transfers the file to iRODS
#' @param src_path Local source file path
#' @param dest_path iRODS destination file path
#' @param data_type the data type string
#' @param force write data-object even it exists already; overwrite it
#' @param calculate_checksum calculate a checksum on the data server-side, and store it in the catalog
#' @param checksum verify checksum - calculate and verify the checksum on the data,
#'                 both client-side and server-side, and store it in the catalog
#' @param progress output the progress of the download
#' @param verbose verbose
#' @param metadata atomically assign metadata after a data object is registered in,
#'                 the catalog. Metadata should be a data frame of "Attribute", "Value", "Unit" OR a string of the
#'                 string of the form attr1;val1;unit1;attr2;val2;unit2;...
#' @param acl      atomically apply ACLs of the form",
#'                 'perm user_or_group;perm user_or_group;'",
#'                 where 'perm' is defined as null|read|write|own"
#'
iput <- function(src_path, dest_path, data_type = "", force = FALSE, calculate_checksum = FALSE, checksum = FALSE, progress = FALSE, verbose = FALSE, metadata = "", acl = "") {

    if(class(metadata)=="character"){
      .Call('rirods_iput', PACKAGE = 'rirods', src_path, dest_path, data_type, force, calculate_checksum, checksum, progress, verbose, metadata, acl)
    } else if (class(metadata)=="data.frame"){
      .Call('rirods_iput', PACKAGE = 'rirods', src_path, dest_path, data_type, force, calculate_checksum, checksum, progress, verbose, .metadata_df_to_str(metadata), acl)
    }
}

#' iget
#' Retrieves the file from iRODS
#' @param src_path iRODS source file path
#' @param dest_path local destination directory or file path for local copies of files
#' @param force force - write local files even it they exist already (overwrite them)
#' @param checksum verify the checksum
#' @param progress output the progress of the download
#' @param verbose verbose
#'
iget <- function(src_path, dest_path=tempdir(), force = FALSE, checksum = FALSE, progress = FALSE, verbose = FALSE) {
  .Call('rirods_iget', PACKAGE = 'rirods', src_path, dest_path, force, checksum, progress, verbose)
}

#' imeta_add
#' Adds a new metadata AVU to the selected iRODS object of the given type
#' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
#' @param name iRODS object name - should be always present
#' @param avu format is a data frame with "Attribute", "Value", ("Unit") OR a string of the format "Attribute;Value;Unit;..."
#'
imeta_add <- function(type, name, avu) {
  if(class(avu)=="character"){
    .Call('rirods_imeta_add', PACKAGE = 'rirods', type, name, avu)
  } else if (class(avu)=="data.frame"){
    for(i in 1:nrow(avu)) {
      .Call('rirods_imeta_add', PACKAGE = 'rirods', type, name, .metadata_df_to_str(avu[i,]))
    }
  }
}

#' imeta_addw
#' Adds a new metadata AVU to the selected iRODS object of the given type
#' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
#' @param name iRODS object name - should be always present, one can use wildcards %, _ on object name
#' @param avu format is a data frame with "Attribute", "Value", ("Unit") OR a string of the format "Attribute;Value;Unit"
#'
imeta_addw <- function(type, name, avu) {
  if(class(avu)=="character"){
    .Call('rirods_imeta_addw', PACKAGE = 'rirods', type, name, avu)
  } else if (class(avu)=="data.frame"){
    .Call('rirods_imeta_addw', PACKAGE = 'rirods', type, name, .metadata_df_to_str(avu))
  }
}

#' imeta_rm
#' Removes the metadata AVU from the selected iRODS object of the given type
#' @param type iRODS files[dD], collections[cC], resources[rR], users[u]
#' @param name iRODS object name - should be always present
#' @param avu  format "Attribute;Value;Unit", unit must be present if exists (iRODS constraint)
#'
imeta_rm <- function(type, name, avu) {
  if(class(avu)=="character"){
    .Call('rirods_imeta_rm', PACKAGE = 'rirods', type, name, avu)
  } else if (class(avu)=="data.frame"){
    for(i in 1:nrow(avu)) {
      .Call('rirods_imeta_rm', PACKAGE = 'rirods', type, name, .metadata_df_to_str(avu[i,]))
    }
  }
}
