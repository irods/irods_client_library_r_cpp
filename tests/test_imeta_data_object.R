library(rirods)
source("check_functions.R")

############################ PLACE TESTS BELOW ############################

# Helper function to sort rows and ensure identical factor mapping
make_metadf_comparable <- function(df){
  df <- df[with(df, order(Attribute)),]
  rownames(df) <- seq(length=nrow(df))
  return(df)
}

# DATA OBJECT METADATA
avus <- NULL
expected_avus <- data.frame(Attribute=c("Project","File type","File format"), Value=c("UNIT_TESTING","Raw data file","Comma-separated value format"), Unit=c("","",""), stringsAsFactors = FALSE)

# imeta_ls
counters <- check_no_exception(name = "imeta_ls(<data object>) no exceptionn",
                               expression = "avus <- imeta_ls('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts')",
                               counter = counters,
                               message = "This should not raise an exception"
)

counters <- check(name = "imeta_ls(<data object>) return type",
                  condition = (class(avus)=="data.frame"),
                  counter = counters,
                  message = paste("Should return a data.frame, not a", class(avus))
)

# sort rows for comparison
expected_avus <- make_metadf_comparable(expected_avus)
avus <- make_metadf_comparable(avus)

counters <- check(name = "imeta_ls(<data object>) output",
                  condition = (identical(avus, expected_avus)),
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)

# imeta_ls(<non-existant>)
counters <- check_exception(name = "imeta_ls(<data object>) no exception",
                            expression = "avus <- imeta_ls('d', '/tempZone/UNIT_TESTING/NonExistantDataObject')",
                            counter = counters,
                            message = "This should raise an exception"
)

# imeta_add(<data.frame>)
avu_addition = data.frame(Attribute=c("T","t"), Value=c("A","a"), Unit=c("N","n"), stringsAsFactors = FALSE)
expected_avus_add <- rbind(expected_avus, avu_addition)

counters <- check_no_exception(name = "imeta_add(<collection>, <data.frame>) after add - no exception",
                               expression = "imeta_add('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts', avu_addition)",
                               counter = counters,
                               message = "This should not raise any exceptions"
)

counters <- check_no_exception(name = "imeta_ls(<data object>, <data.frame>) after add no exception",
                               expression = "avus <- imeta_ls('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts')",
                               counter = counters,
                               message = "This should not raise an exception"
)

# sort rows for comparison
expected_avus_add <- make_metadf_comparable(expected_avus_add)
avus <- make_metadf_comparable(avus)

counters <- check(name = "imeta_ls(<data object>, <data.frame>) after add - output",
                  condition = (identical(avus, expected_avus_add)),
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)

# imeta_rm(<data.frame>)
counters <- check_no_exception(name = "imeta_rm(<data object>, <data.frame>) after add - no exception",
                               expression = "imeta_rm('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts', avu_addition)",
                               counter = counters,
                               message = "This should not raise any exceptions"
)

counters <- check_no_exception(name = "imeta_ls(<data object>, <data.frame>) after rm no exception",
                               expression = "avus <- imeta_ls('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts')",
                               counter = counters,
                               message = "This should not raise an exception"
)

# sort rows for comparison
expected_avus_add <- make_metadf_comparable(expected_avus_add)
avus <- make_metadf_comparable(avus)

counters <- check(name = "imeta_ls(<data object>, <data.frame>) after rm - output",
                  condition = (identical(avus, expected_avus)),
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)

# imeta_add(<string>)
avu_addition = "T;L;I;"
expected_avus_add <- rbind(expected_avus, data.frame(Attribute="T", Value="L", Unit="I"), stringsAsFactors = FALSE)

counters <- check_no_exception(name = "imeta_add(<data object>, <string>) after add - no exception",
                               expression = "imeta_add('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts', avu_addition)",
                               counter = counters,
                               message = "This should not raise any exceptions"
)

counters <- check_no_exception(name = "imeta_ls(<data object>, <string>) after add no exception",
                               expression = "avus <- imeta_ls('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts')",
                               counter = counters,
                               message = "This should not raise an exception"
)

# sort rows for comparison
expected_avus_add <- make_metadf_comparable(expected_avus_add)
avus <- make_metadf_comparable(avus)

counters <- check(name = "imeta_ls(<data object>, <string>) after add - output",
                  condition = (identical(avus, expected_avus_add)),
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)

# imeta_add(<data object>, <existing string>)
counters <- check_exception(name = "imeta_add(<data object>, <existing string>) - exception",
                            expression = "imeta_add('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts', avu_addition)",
                            counter = counters,
                            message = "This should raise an exception"
)

# imeta_rm(<string>)
counters <- check_no_exception(name = "imeta_rm(<data object>, <string>) after add - no exception",
                               expression = "imeta_rm('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts', avu_addition)",
                               counter = counters,
                               message = "This should not raise any exceptions"
)

counters <- check_no_exception(name = "imeta_ls(<data object>, <string>) after rm no exception",
                               expression = "avus <- imeta_ls('d', '/tempZone/UNIT_TESTING/Analysis/testauthor/Transcriptomics/AllSamples.counts')",
                               counter = counters,
                               message = "This should not raise an exception"
)

# sort rows for comparison
expected_avus_add <- make_metadf_comparable(expected_avus_add)
avus <- make_metadf_comparable(avus)

counters <- check(name = "imeta_ls(<data object>, <string>) after rm - output",
                  condition = (identical(avus, expected_avus)),
                  counter = counters,
                  message = "Output does not correspond to expectation from fixtures"
)

############################ PLACE TESTS ABOVE ############################

report_tests_and_exit(counters)

