library(rirods)
source("check_functions.R")

############################ PLACE TESTS BELOW ############################

#Simple search
listing <- ""
cons <- data.frame(Attribute=c("Project", "File format"), Value = c("UNIT_TESTING", "BAM"))
expected_listing <- data.frame(Data_name = c("Sample1.bam","Sample2.bam"),
                               Collection_name =
                                 c("/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics",
                                   "/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics"),
                               Data_path =
                                 c("/var/lib/irods/iRODS/Vault/UNIT_TESTING/Analysis/testauthor/Transcriptomics/Sample1.bam",
                                   "/var/lib/irods/iRODS/Vault/UNIT_TESTING/Analysis/testauthor/Transcriptomics/Sample2.bam"),
                               Data_size = c(63, 63),
                               Data_type = c("d","d"),
                               stringsAsFactors = FALSE)



counters <- check_no_exception(name = "isearch() no exception",
                               expression = "listing <- isearch(constraints = cons)",
                               counter = counters,
                               message = "Should not raise an error"
)

counters <- check(name = "isearch() return type",
                  condition = (class(listing) == "data.frame"),
                  counter = counters,
                  message = "Should return a data.frame"
)

counters <- check(name = "isearch() output column names",
                  condition = (identical(colnames(listing), c("Data_name", "Collection_name", "Data_path", "Data_size", "Data_type", "Create_time", "Modify_time"))),
                  counter = counters,
                  message = "Unexpected column names in output"
)

# sort rows for comparison
expected_listing <- expected_listing[with(expected_listing, order(Collection_name, Data_name)),]
rownames(expected_listing) <- seq(length=nrow(expected_listing))

listing <- listing[with(listing, order(Collection_name, Data_name)),]
rownames(listing) <- seq(length=nrow(listing))

counters <- check(name = "isearch() output",
                  condition = identical(listing[c("Data_name", "Collection_name", "Data_path", "Data_size", "Data_type")], expected_listing), #exclude timestamps from comparison
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)


# Search with multiple values for the same field
listing <- ""
cons <- data.frame(Attribute=c("Project", "File format"), Value = c("UNIT_TESTING", "Comma-separated value format','BAM"))
expected_listing <-  rbind(data.frame(Data_name = c("AllSamples.counts"),
                                      Collection_name = c("/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics"),
                                      Data_path =
                                        c("/var/lib/irods/iRODS/Vault/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts"),
                                      Data_size = c(63),
                                      Data_type = c("d"),
                                      stringsAsFactors = FALSE),
                           expected_listing
                           )

counters <- check_no_exception(name = "in() isearch no exception",
                               expression = "listing <- isearch(constraints = cons)",
                               counter = counters,
                               message = "Should not raise an error"
)

counters <- check(name = "in() isearch return type",
                  condition = (class(listing) == "data.frame"),
                  counter = counters,
                  message = "Should return a data.frame"
)

counters <- check(name = "in() isearch output column names",
                  condition = (identical(colnames(listing), c("Data_name", "Collection_name", "Data_path", "Data_size", "Data_type", "Create_time", "Modify_time"))),
                  counter = counters,
                  message = "Unexpected column names in output"
)

# sort rows for comparison
expected_listing <- expected_listing[with(expected_listing, order(Collection_name, Data_name)),]
rownames(expected_listing) <- seq(length=nrow(expected_listing))

listing <- listing[with(listing, order(Collection_name, Data_name)),]
rownames(listing) <- seq(length=nrow(listing))

counters <- check(name = "in() isearch output",
                  condition = identical(listing[c("Data_name", "Collection_name", "Data_path", "Data_size", "Data_type")], expected_listing), #exclude timestamps from comparison
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)




############################ PLACE TESTS ABOVE ############################

report_tests_and_exit(counters)
