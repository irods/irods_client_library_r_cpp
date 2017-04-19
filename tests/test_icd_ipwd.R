library(rirods)
source("check_functions.R")

# Absolute path
collection = "/tempZone/UNIT_TESTING/Data"
cd_collection = icd(collection)
pwd_collection = ipwd()

counters <- check(name = "icd() return type",
      condition = (class(cd_collection) == "character"),
      counter = counters,
      message = paste("Returned object of incorrect class. Received \"", class(cd_collection), "\", expecting \"character\"")
      )
counters <- check(name = "icd() - Absolute path",
      condition = (cd_collection == collection),
      counter = counters,
      message = paste("Has not returned the correct directory. Returned: \"", cd_collection, "\", expecting: \"", collection, "\"")
      )
counters <- check(name = "ipwd() - Absolute path",
      condition = (collection == pwd_collection),
      counter = counters,
      message = paste("Has not returned the correct directory. Returned: \"", pwd_collection, "\", expecting: \"", collection, "\"")
      )

# Relative path
cd_collection = icd("Proteomics")
pwd_collection = ipwd()
collection = paste(collection, "Proteomics", sep = "/")

counters <- check(name = "icd() - Relative path",
      condition = (cd_collection == collection),
      counter = counters,
      message = paste("Has not returned the correct directory. Returned: \"", cd_collection, "\", expecting: \"", collection, "\"")
      )
counters <- check(name = "ipwd() - Relative path",
      condition = (collection == pwd_collection),
      counter = counters,
      message = paste("Has not returned the correct directory. Returned: \"", pwd_collection, "\", expecting \"", collection, "\"")
      )

# Up directory
cd_collection = icd("..")
pwd_collection = ipwd()
collection = "/tempZone/UNIT_TESTING/Data"

counters <- check(name = "icd(\"..\") - Up directory",
                  condition = (cd_collection == collection),
                  counter = counters,
                  message = paste("Has not returned the correct directory. Returned: \"", cd_collection, "\", expecting: \"", collection, "\"")
)
counters <- check(name = "ipwd() - Up directory",
                  condition = (collection == pwd_collection),
                  counter = counters,
                  message = paste("Has not returned the correct directory. Returned: \"", pwd_collection, "\", expecting \"", collection, "\"")
)

# Home directory
cd_collection = icd("~")
pwd_collection = ipwd()
collection = ienv()$irods_home

counters <- check(name = "icd() - Tilde",
      condition = (cd_collection == collection),
      counter = counters,
      message = paste("Has not returned the correct directory. Returned: \"", cd_collection, "\", expecting: \"", collection, "\"")
)
counters <- check(name = "ipwd() - Tilde",
      condition = (collection == pwd_collection),
      counter = counters,
      message = paste("Has not returned the correct directory. Returned: \"", pwd_collection, "\", expecting \"", collection, "\"")
)

# Non-existent collection
counters <- check_exception(name = "icd() - Exception on non-existent directory",
                expression = "icd('/Nozone/a/b/c')",
                counter = counters,
                message = "Has not raised an error when called with non-existent collection."
                )

report_tests_and_exit(counters)
