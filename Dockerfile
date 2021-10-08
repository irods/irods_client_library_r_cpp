# docker build -f Dockerfile -t myimages/rirods-builder .

FROM ubuntu:18.04

# initial requirements
RUN apt-get update && \
    apt-get install -y sudo wget nano gnupg make

# install iRODS repository
RUN wget -qO - https://packages.irods.org/irods-signing-key.asc | sudo apt-key add - && \
    echo "deb [arch=amd64] https://packages.irods.org/apt/ bionic  main" | sudo tee /etc/apt/sources.list.d/renci-irods.list && \
    apt-get update

# install build prerequisites
RUN DEBIAN_FRONTEND="noninteractive" apt-get install -y r-base-core irods-dev irods-runtime irods-externals-clang6.0.0
RUN apt-get install -y libkrb5-dev libjansson-dev

# prepare R environment
RUN mkdir /root/.R && \
    echo "CC=/opt/irods-externals/clang6.0-0/bin/clang" >> /root/.R/Makevars && \
    echo "CXX=/opt/irods-externals/clang6.0-0/bin/clang++" >> /root/.R/Makevars

# install R prerequisites
COPY ./ /rirods-source
RUN R CMD BATCH /rirods-source/install-Rcpp.R

# build rirods
#RUN make r_cmd_build

# leave this container running for development purposes
CMD tail -f /dev/null
