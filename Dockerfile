# docker build -f Dockerfile -t myimages/rirods-builder .

FROM ubuntu:18.04

# initial requirements
RUN apt-get update && \
    apt-get install -y sudo wget nano gnupg make

# install CRAN repository (https://cran.r-project.org/)
RUN wget -qO - https://cloud.r-project.org/bin/linux/ubuntu/marutter_pubkey.asc | sudo tee -a /etc/apt/trusted.gpg.d/cran_ubuntu_key.asc && \
    echo "deb https://cloud.r-project.org/bin/linux/ubuntu bionic-cran40/" | sudo tee /etc/apt/sources.list.d/renci-irods.list && \
    apt-get update

# install build R
RUN apt-get install -y --no-install-recommends r-base

# install iRODS repository
RUN wget -qO - https://packages.irods.org/irods-signing-key.asc | sudo apt-key add - && \
    echo "deb [arch=amd64] https://packages.irods.org/apt/ bionic  main" | sudo tee /etc/apt/sources.list.d/renci-irods.list && \
    apt-get update

# install build iRODS
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y irods-dev irods-runtime irods-externals-clang6.0.0
RUN apt-get install -y  libkrb5-dev irods-externals-jansson2.7-0

# Package development: devtools and rmarkdown
RUN apt-get update -qq
RUN apt-get install -y --no-install-recommends software-properties-common dirmngr
RUN apt-get install -y build-essential libcurl4-gnutls-dev libxml2-dev libssl-dev pandoc ghostscript qpdf libc6 libgcc1

# prepare R environment
RUN mkdir /root/.R && \
      echo "CC=/opt/irods-externals/clang6.0-0/bin/clang" >> /root/.R/Makevars && \
      echo "CXX=/opt/irods-externals/clang6.0-0/bin/clang++" >> /root/.R/Makevars

COPY ./ /rirods
RUN cd /rirods

# install R prerequisites (https://cran.r-project.org/)
RUN add-apt-repository ppa:c2d4u.team/c2d4u4.0+
#RUN apt-get install -y --no-install-recommends r-cran-devtools r-cran-rcpp
RUN R -e "install.packages(c('devtools', 'Rcpp','rmarkdown'))"

# use the package set dir to package
ENTRYPOINT ["R", "-e", "setwd('rirods')"]
