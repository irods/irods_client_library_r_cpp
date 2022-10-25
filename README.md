Note: This repository has been deprecated and archived.

Please find a new pure-R implementation at:

https://github.com/irods/irods_client_library_rirods

---

# rirods2015 C++ R-Package

`rirods2015` is an integration solution whose aim is to allow users within R to work efficiently and comfortably with iRODS data objects, following the R paradigms they are familiar with. Standard R objects are accepted and returned. Furthermore, the design minimizes dependency on tools beyond iRODS itself in order to both limit complexity of deployment and to benefit from the performance optimizations present in the core iRODS.

The `rirods2015` package has been engineered to have semantics equivalent to the iCommands and can easily be used as a basis for further customization.

It was initially developed at the Nestle Institute of Health Sciences by Radovan Chytracek, Richard Coté, and Bernhard Sonderegger; based heavily on iCommands code.

`rirods2015` was demontrated to work with iRODS 4.1.x.


## Installation
### Prerequisites are:
`irods-dev irods-runtime gcc gcc-c++`
### Installing the R package
`install.packages('./rirods_1.0.tar.gz', repos = NULL, type="source")`
