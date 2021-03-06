Run LAMMPS from Python
======================

The LAMMPS distribution includes a python directory with all you need
to run LAMMPS from Python.  The python/lammps.py file wraps the LAMMPS
library interface, with one wrapper function per LAMMPS library
function.  This file makes it is possible to do the following either
from a Python script, or interactively from a Python prompt: create
one or more instances of LAMMPS, invoke LAMMPS commands or give it an
input script, run LAMMPS incrementally, extract LAMMPS results, an
modify internal LAMMPS variables.  From a Python script you can do
this in serial or parallel.  Running Python interactively in parallel
does not generally work, unless you have a version of Python that
extends Python to enable multiple instances of Python to read what you
type.

To do all of this, you must first build LAMMPS as a shared library,
then insure that your Python can find the python/lammps.py file and
the shared library.

Two advantages of using Python to run LAMMPS are how concise the
language is, and that it can be run interactively, enabling rapid
development and debugging.  If you use it to mostly invoke costly
operations within LAMMPS, such as running a simulation for a
reasonable number of timesteps, then the overhead cost of invoking
LAMMPS through Python will be negligible.

The Python wrapper for LAMMPS uses the "ctypes" package in Python,
which auto-generates the interface code needed between Python and a
set of C-style library functions.  Ctypes is part of standard Python
for versions 2.5 and later.  You can check which version of Python you
have by simply typing "python" at a shell prompt.


.. _lws: http://lammps.sandia.gov
.. _ld: Manual.html
.. _lc: Commands_all.html
