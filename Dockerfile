# rocker
FROM rocker/rstudio:4.0.0-ubuntu18.04

# initial requirements
RUN apt-get update && \
    apt-get install -y sudo wget nano gnupg make

# install iRODS repository
RUN wget -qO - https://packages.irods.org/irods-signing-key.asc | sudo apt-key add - && \
    echo "deb [arch=amd64] https://packages.irods.org/apt/ bionic  main" | sudo tee /etc/apt/sources.list.d/renci-irods.list && \
    apt-get update

# install build iRODS
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y irods-dev irods-runtime irods-externals-clang6.0.0
RUN apt-get install -y  libkrb5-dev irods-externals-jansson2.7-0

# prepare R environment
RUN mkdir /root/.R && \
      echo "CC=/opt/irods-externals/clang6.0-0/bin/clang" >> /root/.R/Makevars && \
      echo "CXX=/opt/irods-externals/clang6.0-0/bin/clang++" >> /root/.R/Makevars

COPY ./ /home/rstudio/rirods
RUN cd /home/rstudio/rirods

# package development, tidy tools and rmarkdown
RUN /rocker_scripts/install_tidyverse.sh
RUN /rocker_scripts/install_pandoc.sh

# use the package set dir to package
CMD ["/init"]
