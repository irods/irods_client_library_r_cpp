library(rirods)
source("check_functions.R")

############################ PLACE TESTS BELOW ############################

# Prepare local filesystem
myDir <- tempdir()
myFile <- tempfile()
getDir <-paste0(myDir, '/getdir')
system(paste('mkdir', getDir))
write.table(file = myFile, data.frame(a=c("b","c","d"), A=c("B","C","D")))
df <- read.table(myFile)
getdf <- NULL
returnval <- NULL

# iput(<collection destination>)
expectedIrodsPath <- paste0('/tempZone/UNIT_TESTING/EmptyCollection/',basename(myFile))
counters <- check_no_exception(name = "iput(<collection destination>) - no exception",
                               expression = 'returnval <- iput(src_path = myFile, dest_path = "/tempZone/UNIT_TESTING/EmptyCollection")',
                               counter = counters,
                               message = "Should not raise exception"
)

counters <- check(name = "iput(<collection destination>) - return value",
                  condition = (returnval == expectedIrodsPath),
                  counter = counters,
                  message = "Path not returned"
)

counters <- check_no_exception(name = "iput(<collection destination>) - check file exists in iRODS",
                               expression = paste0('ils("',expectedIrodsPath,'")'),
                               counter = counters,
                               message = "Exception may indicate file does not exist"
)

counters <- check_no_exception(name = "iget(<directory destination>) - get file",
                               expression = paste0('iget("/tempZone/UNIT_TESTING/EmptyCollection/',basename(myFile),'", "',getDir,'" )'),
                               counter = counters,
                               message = "Exception may indicate file does not exist in iRODS"
)

counters <- check_no_exception(name = "iget(<directory destination>) - check file exists locally and is readable",
                               expression = paste0('getdf <- read.table("',getDir, '/', basename(myFile),'")'),
                               counter = counters,
                               message = "Exception indicates file does not exist"
)

counters <- check(name = "iget(<directory destination>) - Validate file readable",
                  condition = identical(getdf, df),
                  counter = counters,
                  message = "This should never fail"
)


# iput(<data object destination>)
counters <- check_no_exception(name = "iput(<data object destination>) - no exception",
                               expression = 'iput(src_path = myFile, dest_path = "/tempZone/UNIT_TESTING/EmptyCollection/table")',
                               counter = counters,
                               message = "Should not raise exception"
)

counters <- check_no_exception(name = "iput(<data object destination>) - check file exists",
                               expression = 'ils("/tempZone/UNIT_TESTING/EmptyCollection/table")',
                               counter = counters,
                               message = "Exception indicates file does not exist"
)



# iput(<existing destination>) - overwrite no force
counters <- check_exception(name = "iput(<existing destination>) - overwrite no force",
                            expression = 'iput(src_path = myFile, dest_path = "/tempZone/UNIT_TESTING/EmptyCollection")',
                            counter = counters,
                            message = "Should not raise exception"
)

# iput(<existing destination>) - overwrite force
counters <- check_no_exception(name = "iput(<existing destination>) - overwrite force",
                               expression = 'iput(src_path = myFile, dest_path = "/tempZone/UNIT_TESTING/EmptyCollection", force = TRUE)',
                               counter = counters,
                               message = "Should not raise exception"
)

# iput(<missing destination>)
counters <- check_exception(name = "iput(<missing destination>)",
                            expression = 'iput(src_path = myFile, dest_path = "/tempZone/UNIT_TESTING/NoCollection/noFile")',
                            counter = counters,
                            message = "Should raise exception"
)

# iput(<missing source>)
counters <- check_exception(name = "iput(<missing source>) - no exception",
                            expression = paste0('iput(src_path = ',tmpfile(),', dest_path = "/tempZone/UNIT_TESTING/EmptyCollection")'),
                            counter = counters,
                            message = "Should raise exception"
)



# iget


#counters <- add_failing_test(counters)


############################ PLACE TESTS ABOVE ############################

report_tests_and_exit(counters)

