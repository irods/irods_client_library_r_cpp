.onLoad <- function (libpath, pkgname) {
    # We load rirods shared library with global symbol resolution enforced to make sure
    # that iRODS internal plugins can also resolve symbols from iRODS API library
    library.dynam("rirods", pkgname, libpath, local = FALSE, now = FALSE)
}

.onUnload <- function (libpath) {
  library.dynam.unload("rirods", libpath)
}

.metadata_df_to_str <- function (metadata_df){
  metadata_string = ""
  for (attr_idx in 1:nrow(metadata_df)) {
    metadata_string <- paste(metadata_string,metadata_df[attr_idx,"Attribute"],";",metadata_df[attr_idx,"Value"],";",metadata_df[attr_idx,"Unit"],";", sep="")
  }
  return(metadata_string)
}
