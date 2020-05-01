# Copyright 2013-2020 Lawrence Livermore National Security, LLC and other
# Spack Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack import *


class Decaf(CMakePackage):
    """A dataflow system for the parallel communication of coupled tasks in an HPC workflow."""

    homepage = "https://tpeterka.github.io/decaf/"
    url      = "https://github.com/tpeterka/decaf.git"
    git      = "https://github.com/tpeterka/decaf.git"

    version('master', branch='master')

    depends_on('mpi')
    depends_on('boost')
    depends_on('python@3:',   type='run')
    depends_on('py-networkx', type='run')

    def cmake_args(self):
        args = ['-DCMAKE_C_COMPILER=%s' % self.spec['mpi'].mpicc,
                '-DCMAKE_CXX_COMPILER=%s' % self.spec['mpi'].mpicxx]
        return args
