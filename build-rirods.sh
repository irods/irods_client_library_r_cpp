#!/bin/bash

# build image
sudo docker image build --tag rirods:build .

# local source and library paths
SRCPATH=$(pwd)
LIBPATH=$(R -e ".libPaths()[1]" | grep /)

# devtools document
sudo docker container run --rm --name rirods-document \
  -v $SRCPATH:/rirods \
  rirods:build \
  -e 'devtools::document()'

# devtools build
sudo docker container run --rm --name rirods-document \
  -v $(SRCPATH):/rirods \
  rirods:build \
  -e 'devtools::build()'

# clean src
rm -f src/*.o && rm -f src/*.so && rm -f src/*.dll

