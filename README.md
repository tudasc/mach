MPI Assertion checking
=======

This clang Pass allows to check if the assertions defined in section 6.4.4 of the upcoming MPI standard[^fn1] hold for an application.

Building
-----------
Building the Pass with Cmake is quite straightforward:

''mkdir build; cd build; cmake ..; make -j 4''
You need LLVM/clang version 10.0.

Running
-----------
For running the pass, you need an MPI Implementation built with clang (Tested with mpich 3.3.2).
For convenience, you can use the 'run.sh' script in order to run the analysis.
The Analysis results are printed to the command line.

[^fn1]: https://www.mpi-forum.org/docs/drafts/mpi-2019-draft-report.pdf
