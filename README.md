# gmat-dslwp-source

This is a modified version of the GMAT-R2019aBeta1 source that implements
delta-range measurements for simulation and estimation of DSLWP-B VLBI
observations.

The source tree contains a modified version of the GMAT EstimationPlugin that
defines these new measurement types.

A modified Makefile is provided so that the EstimationPlugin can be built
independently of the rest of GMAT's source tree, provided that a binary build of
GMAT-R2019aBeta1 is available.

To build, the GMAT-R2019aBeta1 binary build should be placed in a folder called
`gmat-dslwp-binary` alongside this source folder.

First, the Python3 script `configure.py` inide `GMAT-R2019aBeta1/depends` must
be run to download and configure Xerces sources, which is necessary to build the
EstimationPlugin. Errors regarding wxWidgets and CSPICE when running
`configure.py` can be ignored safely.

Then it is necessary to `cd` into
`GMAT-R2019aBeta1/plugins/EstimationPlugin/src/base` and run `make`. This should
place the compiled `libGmatEstimation.so` into
`GMAT-R2019aBeta1/plugins/EstimationPlugin/lib`.

To install the library it should be placed into the GMAT-R2019aBeta1 binary
build, replacing the library already present in `GMAT/R2019aBeta1/lib`.
