library(rirods)
source("check_functions.R")

listing <- ils()

counters <- check(name = "ils() return type",
                  condition = (class(listing) == "data.frame"),
                  counter = counters,
                  message = "Should return a data.frame"
)

counters <- check(name = "ils() output column names",
                  condition = (identical(colnames(listing), c("Data_name", "Collection_name", "Data_path", "Data_size", "Data_type", "Create_time", "Modify_time"))),
                  counter = counters,
                  message = "Unexpected column names in output"
)


# ils(<Absolute path>)
listing <- ils("/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics")
expected_listing <- data.frame(Data_name = c("","AllSamples.counts","Sample1.bam","Sample2.bam"),
                               Collection_name =
                                 c("/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/indices",
                                   "/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics",
                                   "/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics",
                                   "/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics"),
                               Data_path =
                                 c("",
                                   "/var/lib/irods/iRODS/Vault/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts",
                                   "/var/lib/irods/iRODS/Vault/UNIT_TESTING/Analysis/testauthor/Transcriptomics/Sample1.bam",
                                   "/var/lib/irods/iRODS/Vault/UNIT_TESTING/Analysis/testauthor/Transcriptomics/Sample2.bam"),
                               Data_size =
                                 c(-1, 63, 63, 63),
                               Data_type =
                                 c("C","d","d","d"),
                               Create_time =
                                 listing$Create_time, # cannot predict timestamps
                               Modify_time =
                                 listing$Modify_time, # cannot predict timestamps
                               stringsAsFactors = FALSE
)

# sort rows for comparison
expected_listing <- expected_listing[with(expected_listing, order(Collection_name, Data_name)),]
rownames(expected_listing) <- seq(length=nrow(expected_listing))

listing <- listing[with(listing, order(Collection_name, Data_name)),]
rownames(listing) <- seq(length=nrow(listing))

counters <- check(name = "ils(<absolute path>)",
                  condition = (identical(listing, expected_listing)),
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)

counters <- check(name = "Correct class for ils() Create_time column",
                  condition = (class(listing$Create_time)==c("POSIXct", "POSIXt")),
                  counter = counters,
                  message = "Incorrect class for Create_time column"
)

counters <- check(name = "Correct class for ils() Modify_time column",
                  condition = (class(listing$Modify_time)==c("POSIXct", "POSIXt")),
                  counter = counters,
                  message = "Incorrect class for Modify_time column"
)

# relative path
icd("/tempZone/UNIT_TESTING/Analysis/testauthor")
listing <- ils("Transcriptomics")

# sort rows for comparison
expected_listing <- expected_listing[with(expected_listing, order(Collection_name, Data_name)),]
rownames(expected_listing) <- seq(length=nrow(expected_listing))

listing <- listing[with(listing, order(Collection_name, Data_name)),]
rownames(listing) <- seq(length=nrow(listing))

counters <- check(name = "ils(<relative path>)",
                  condition = (identical(listing, expected_listing)),
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)

# Currently no test for ils("~") as we have no fixtures there

# non-existant collection
counters <- check_exception(name = "ils(<non-existant>)",
                            expression = "ils(/tempZone/UNIT_TESTING/banana)",
                            counter = counters,
                            message = "Should raise error"
)

report_tests_and_exit(counters)
