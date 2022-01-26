#!/bin/bash

# build image
sudo docker image build --tag rirods:build .

# interactive R studio
sudo docker container run -d \
  -p 8787:8787 \
  -v $(pwd):/home/rstudio/rirods \
  -e ROOT=TRUE \
  -e PASSWORD=pass \
  --name rirods-document \
  rirods:build
