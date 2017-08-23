#! /bin/bash

SOFTWARE_FOLDER=$HOME/software
SOFTWARE_INSTALL_FOLDER=$SOFTWARE_FOLDER/install
DECAF_FOLDER=$HOME/Decaf

# Dependencies installation

sudo apt-get install git
sudo apt-get install cmake #version 3.6
sudo apt-get install python-pip # version 2.7.13

# Minimum version: 1.8 for MPI 3.0
sudo apt-get install openmpi-common
sudo apt-get install openmpi-bin
sudo apt-get install libopenmpi-dev

mkdir $SOFTWARE_FOLDER
mkdir $SOFTWARE_INSTALL_FOLDER

export PATH=$SOFTWARE_INSTALL_FOLDER/include:$SOFTWARE_INSTALL_FOLDER/bin:$PATH
export LD_LIBRARY_PATH=$SOFTWARE_INSTALL_FOLDER/lib:$LD_LIBRARY_PATH

#Boost
cd $SOFTWARE_FOLDER
wget https://dl.bintray.com/boostorg/release/1.64.0/source/boost_1_64_0.tar.gz
tar -xf boost_1_64_0.tar.gz
cd boost_1_64_0
./bootstrap.sh --prefix=$SOFTWARE_INSTALL_FOLDER
./b2 install

#NetworkX
pip install networkx

#CCI
cd $SOFTWARE_FOLDER
wget http://cci-forum.com/wp-content/uploads/2017/05/cci-2.1.tar.gz
tar -xf cci-2.1.tar.gz
cd cci-2.1
./configure --prefix=$SOFTWARE_INSTALL_FOLDER
make -j4 install

# Decaf build
mkdir $DECAF_FOLDER
cd $DECAF_FOLDER
mkdir source build install
cd source
git clone https://bitbucket.org/tpeterka1/decaf.git .
cd ../build
cmake ../source/ -DCMAKE_INSTALL_PREFIX:PATH=$DECAF_FOLDER/install -Dtransport_cci=on
make -j4 install

export LD_LIBRARY_PATH=$DECAF_FOLDER/install/lib:$LD_LIBRARY_PATH
export DECAF_PREFIX=$DECAF_FOLDER/install/