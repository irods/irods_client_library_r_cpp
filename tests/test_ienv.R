library(rirods)
source("check_functions.R")

############################ PLACE TESTS BELOW ############################

env <- ienv()

counters <- check(name = "ienv() return type",
                  condition = (class(env) == "list"),
                  counter = counters,
                  message = paste("ienv() has returned object of incorrect class. Received \"", class(env), "\", expecting \"character\"")
                  )

counters <- check(name = "ienv() value for zone name",
                  condition = (env$irods_zone_name == "tempZone"),
                  counter = counters,
                  paste("ienv() has returned incorrect zone name. Received \"", env["irods_zone_name"], "\", expecting \"tempZone\"")
)

############################ PLACE TESTS ABOVE ############################

report_tests_and_exit(counters)
