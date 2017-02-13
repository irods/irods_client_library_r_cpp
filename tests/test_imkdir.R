library(rirods)
source("check_functions.R")

############################ PLACE TESTS BELOW ############################

target <- "/tempZone/UNIT_TESTING/MadeCollection"

# Valid absolute path
output <- ""
counters <- check_no_exception(name = "imkdir(<valid absolute path>)",
                               expression = paste0("output <- imkdir(\"", target, "\")"),
                               counter = counters,
                               message = "Should not generate any errors"
)

counters <- check(name = "imkdir(<valid absolute path>) return value",
                  condition = (output == target),
                  counter = counters,
                  message = "Should return absolute path of created directory"
)

# Valid relative path
invisible(icd(target))
output <- ""
counters <- check_no_exception(name = "imkdir(<valid relative path>)",
                               expression = "output <- imkdir(\"subdir\")",
                               counter = counters,
                               message = "Should not generate any errors"
)

counters <- check(name = "imkdir(<valid relative path>) return value",
                  condition = (output == paste0(target, "/subdir")),
                  counter = counters,
                  message = "Should return absolute path of created directory"
)

# Absolute path with missing parent
target <- paste(target, "a", "b", sep = "/")
output <- ""
counters <- check_exception(name = "imkdir(<valid absolute path>)",
                               expression = paste0("output <- imkdir(\"", target, "\")"),
                               counter = counters,
                               message = "Should generate error as parent directories not created"
)

counters <- check_no_exception(name = "imkdir(<valid absolute path>, parents = TRUE)",
                            expression = paste0("output <- imkdir(\"", target, "\", parents = TRUE)"),
                            counter = counters,
                            message = "Should not generate error as parent directories are created"
)

counters <- check(name = "imkdir(<valid absolute path>) return value",
                  condition = (output == target),
                  counter = counters,
                  message = "Should return absolute path of created directory"
)

# Relative path with missing parent
icd(target)
output <- ""

counters <- check_exception(name = "imkdir(<valid relative path>)",
                            expression = "output <- imkdir(\"A/B\")",
                            counter = counters,
                            message = "Should generate error as parent directories not created"
)

counters <- check_no_exception(name = "imkdir(<valid relative path>, parents = TRUE)",
                               expression = "output <- imkdir(\"A/B\", parents = TRUE)",
                               counter = counters,
                               message = "Should not generate error as parent directories are created"
)

counters <- check(name = "imkdir(<valid relative path>) return value",
                  condition = (output == paste0(target, "/A/B")),
                  counter = counters,
                  message = "Should return absolute path of created directory"
)

############################ PLACE TESTS ABOVE ############################

report_tests_and_exit(counters)

