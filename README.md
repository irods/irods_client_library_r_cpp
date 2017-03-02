# rirods R-Package

`rirods` is an integration solution whose aim is to allow users within R to work efficiently and comfortably with iRODS data objects, following the R paradigms they are familiar with. Standard R objects are accepted and returned. Furthermore, the design minimizes dependency on tools beyond iRODS itself in order to both limit complexity of deployment and to benefit from the performance optimizations present in the core iRODS.

The `rirods` package has been engineered to have semantics equivalent to the iCommands and can easily be used as a basis for further customization.

It was initially developed at the Nestle Institute of Health Sciences by Radovan Chytracek, Richard Cote, and Bernhard Sonderegger; based heavily on iCommands code.

`rirods` is currently supported for use with iRODS 4.1.x.

Support for iRODS 4.2+ is forthcoming.

## Installation
### Prerequisites are:
`irods-dev irods-runtime gcc gcc-c++`
### Installing the R package
`install.packages('./rirods_1.0.tar.gz', repos = NULL, type="source")`
