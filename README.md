MPI Assertion checking
=======

This clang Pass allows to check if the assertions defined in section 6.4.4 of the upcoming MPI standard \[[MPI19](#ref-mpi2019)\ hold for an application.

Building
-----------
Building the Pass with Cmake is quite straightforward:

``mkdir build; cd build; cmake ..; make -j 4``
You need LLVM/clang version 10.0.

Running
-----------
For running the pass, you need an MPI Implementation built with clang (Tested with mpich 3.3.2).
For convenience, you can use the 'run.sh' script in order to run the analysis.
The Analysis results are printed to the command line.

References
-----------
<table style="border:0px">
<tr>
    <td valign="top"><a name="ref-MACH20"></a>[MACH20]</td>
    <td>Jammer, Tim and Iwainsky, Christian and Bischof, Christian:
       Automatic detection of MPI assertions 2020.</td>
</tr>
<tr>
    <td valign="top"><a name="ref-mpi2019"></a>[MPI19]</td>
    <td>Message Passing Interface Forum:
    <a href=https://www.mpi-forum.org/docs/drafts/mpi-2019-draft-report.pdf>
    MPI: A Message-Passing Interface Standard - 2019 Draft Specification, 2019</td>
</tr>
</table>
