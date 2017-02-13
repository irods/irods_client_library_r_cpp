library(rirods)
source("check_functions.R")

############################ PLACE TESTS BELOW ############################

counters <- check(name = "Passing test",
                  condition = (TRUE),
                  counter = counters,
                  message = "This should never fail"
)

counters <- check_no_exception(name = "Passing test for no exception",
                               expression = "TRUE",
                               counter = counters,
                               message = "This should never fail"
)
counters <- check_exception(name = "Passing test for exception",
                            expression = "stop()",
                            counter = counters,
                            message = "This should never fail"
)


############################ PLACE TESTS ABOVE ############################

report_tests_and_exit(counters)

