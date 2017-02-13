library(rirods)

library(rirods)
source("check_functions.R")

############################ PLACE TESTS BELOW ############################

# Remove data object (file)
target <- "/tempZone/UNIT_TESTING/Data/Transcriptomics/Sample1.fastq"
counters <- check_no_exception(name = "ils() prior to irm(<data object>)",
                               expression = paste0("ils(\"",target,"\")"),
                               counter = counters,
                               message = "File should exist"
)
counters <- check_no_exception(name = "irm(<data object>)",
                               expression = paste0("irm(\"",target,"\")"),
                               counter = counters,
                               message = "Removal should not raise errors"
)
counters <- check_exception(name = "ils() after irm(<data object>)",
                            expression = paste0("ils(\"",target,"\")"),
                            counter = counters,
                            message = "File should no longer exist"
)

# Remove collection (directory)
target <- "/tempZone/UNIT_TESTING/EmptyCollection"
counters <- check_no_exception(name = "ils() prior to irm(<empty collection>)",
                               expression = paste0("ils(\"",target,"\")"),
                               counter = counters,
                               message = "Collection should exist"
)
counters <- check_no_exception(name = "irm(<empty collection>)",
                               expression = paste0("irm(\"",target,"\", recursive = TRUE)"),
                               counter = counters,
                               message = "Removal should not raise errors"
)
counters <- check_exception(name = "ils() after irm(<empty collection>)",
                            expression = paste0("ils(\"",target,"\")"),
                            counter = counters,
                            message = "Collection should no longer exist"
)

# Remove collection with contents
target <- "/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics"
counters <- check_no_exception(name = "ils() prior to irm(<collection>)",
                               expression = paste0("ils(\"",target,"\")"),
                               counter = counters,
                               message = "File should exist"
)
counters <- check_no_exception(name = "irm(<collection>)",
                               expression = paste0("irm(\"",target,"\", recursive = TRUE)"),
                               counter = counters,
                               message = "Removal should not raise errors"
)
counters <- check_exception(name = "ils() after irm(<data object>)",
                            expression = paste0("ils(\"",target,"\")"),
                            counter = counters,
                            message = "File should no longer exist"
)


# Attempt to remove non-existant object
counters <- check_exception(name = "irm(<non-existant>)",
                            expression = "irm(\"/Nozone/a/b/c\")",
                            counter = counters,
                            message = "Should raise error")

############################ PLACE TESTS ABOVE ############################

report_tests_and_exit(counters)

