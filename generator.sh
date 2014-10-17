#!/bin/bash


function setup
{
	# Define here the installation path for your platform
	echo "============================= Platform identification ============================="
	case "$HOSTNAME" in
		Arcteryx) echo "$HOSTNAME is registered. Compiling with specific parameters."
			install_path="$PWD/install"
			options="-Ddebug=ON -Doptimize=OFF -Dtransport_mpi=ON"
			echo -n "MPI library [openmpi]/mpich : "
			read a
			case "$a" in
				mpich) echo "Compiling with using MPICH transport link"
					mpi_path_overide="/home/amxx/Work/Argonne/Libs/mpich-3.1.3/install/"
					;;
				*) echo "Compiling with using OPENMPI transport link"
					;;
			esac
			;;
		*) echo "Your platform is not defined in $0."
			install_path="$PWD/install"
			options="-Ddebug=OFF -Doptimize=OFF -Dtransport_mpi=ON"
			echo -n "Path MPI library [default]/path : "
			read mpi_path_overide
			;;
	esac
	# Setting links to executables
	if [ -n "$mpi_path_overide" ];
	then
		export PATH=$mpi_path_overide/bin:$PATH
		export LD_LIBRARY_PATH=$mpi_path_overide/lib:$LD_LIBRARY_PATH
	fi
	mpi_c=`which mpicxx`
	mpi_r=`which mpiexec`
	# Summary
	echo "============================== Configuration summary =============================="
	echo "MPI compiler : $mpi_c"
	echo "MPI runner : $mpi_r"
	echo "Options $options"
	echo "==================================================================================="
}

function build
{
	setup &&
	mkdir -p build &&
	cd build &&
	cmake -DCMAKE_CXX_COMPILER=$mpi_c						\
				-DCMAKE_INSTALL_PREFIX:PATH=$install_path			\
				$options																			\
				.. &&
	# make install &&
	make &&
	cd ..
}

function launch
{
	echo "================================ RUNNING EXAMPLES ================================="
	echo "command : $mpi_r -n 10 build/examples/direct"
	$mpi_r -n 10 build/examples/direct
}

function remove
{
	rm -rfv $1
}

function help
{
	echo "Project generator"
	echo "Options are:"
	echo " -h, --help         show brief help"
	echo " -b, --build        build the project"
	echo " -c, --clean        clean the project"
	echo " --clear            clear the project (remove binaries)"
}

############################################
#                   MAIN                   #
############################################

if test $# -gt 0; then
	case "$1" in
		-h|--help)
			help
			exit 0
			;;
		
		-b|--build)
			build
			exit 0
			;;
			
		-c|--clean)
			remove "build"
			exit 0
			;;
					
		*)
			echo "invalid option -- '$1'"
			help
			exit 0
			;;
	esac
fi

build && launch
#help
