# This is preventive action to make sure the build can go
R_BUILD_LOCK_DIR = ${HOME}/R/x86_64-redhat-linux-gnu-library/3.2/00LOCK-rirods

r_build_unlock:
	@echo "Removing lock: ${R_BUILD_LOCK_DIR}"
	@rm -rf ${R_BUILD_LOCK_DIR}

rcpp_attributes:
	@echo "Working dir: "`pwd`
	@echo `Rscript -e 'library(Rcpp); Rcpp::compileAttributes()'`
	
r_cmd_build: rcpp_attributes r_build_unlock
	@echo "Working dir: "`pwd`
	R CMD build .
	
r_cmd_pkg: rcpp_attributes r_build_unlock
	@echo "Working dir: "`pwd`
	R CMD INSTALL --build .
	
r_cmd_clean: rcpp_attributes r_build_unlock
	@echo "Working dir: "`pwd`
	R CMD INSTALL --clean .
	
r_cmd_install: rcpp_attributes r_build_unlock
	@echo "Working dir: "`pwd`
	R CMD INSTALL .

inst/doc/rirods__Getting_R_to_talk_to_iRODS.html: vignettes/rirods__Getting_R_to_talk_to_iRODS.Rmd
	@echo `Rscript -e 'devtools::build_vignettes()'`
	#echo 'devtools::build_vignettes()' | R --no-save --quiet

vignette: inst/doc/rirods__Getting_R_to_talk_to_iRODS.html

all: vignette r_cmd_build r_cmd_pkg

clean:
	rm -f *.zip
	rm -f *.tar.gz
	rm -f src/*.o
	rm -f src/*.so
