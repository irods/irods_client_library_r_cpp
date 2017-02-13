counters <- list(failure = 0, success = 0, failure_names = c())

check <- function(name, condition, counter, message){
  if (condition){
    cat(".")
    counter$success <- counter$success + 1
  } else{
    print(message)
    counter$failure <- counter$failure + 1
    counter$failure_names <- append(counter$failure_names, paste(name, message, sep = ": "))
    cat("F")
  }
  return(counter)
}

check_exception <- function(name, expression, counter, message){
  tmpcounter <- counter$success
  myEnv <- sys.frame(sys.nframe())

  tryCatch(
    eval.parent(parse(text = expression)),
    error = function(w){
      eval(substitute(counter$success <- counter$success + 1),myEnv)
      cat(".")
    }
  )
  if (counter$success == tmpcounter){
    counter$failure <- counter$failure + 1
    counter$failure_names <- append(counter$failure_names, paste(name, message, sep = ": "))
    cat("F")
  }
  return(counter)
}

check_no_exception <- function(name, expression, counter, message){
  tmpcounter <- counter$failure
  myEnv <- sys.frame(sys.nframe())

  tryCatch(
    eval.parent(parse(text = expression)),
    error = function(w){
      eval(substitute(counter$failure <- counter$failure + 1),myEnv)
      counter$failure_names <- append(counter$failure_names, paste(name, message, sep = ": "))
      cat("F")
    }
  )
  if (counter$failure == tmpcounter){
    counter$success <- counter$success + 1
    cat(".")
  }
  return(counter)
}

report_tests <- function(counter){
  writeLines(paste0("\n============================\n CHECKS RUN: ",
                    counter$success + counter$failure,
                    "\n    SUCCESS: ",
                    counter$success,
                    "\n    FAILURE: ",
                    counter$failure,
                    "\n               ",
                    paste(counter$failure_names, collapse = "\n               "),
                    "\n============================\n")
  )
}

report_tests_and_exit <- function(counter){
  report_tests(counter)
  q(save = "no", status = counter$failure)
}

add_failing_test <- function(counter){
  counters <- check(name = "Failing test",
                    condition = (FALSE),
                    counter = counters,
                    message = "Included to turn on output for debugging"
  )
}
