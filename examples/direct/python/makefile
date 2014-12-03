#
# this makefile builds a .so dynamic shared object out of the python driver + decaf source
#
# the user needs to set the following options manually, and they need to match the cmake settings
# TODO: find a better solution
#
# set the transport layer here
#
CCFLAGS = -DTRANSPORT_MPI
#
# set the compiler here
#
CC = /Users/tpeterka/software/mpich-3.0.4-install/bin/mpicxx
#
# set path to python here
#
INCLUDE = -I/System/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7
LIBS = -L/System/Library/Frameworks/Python.framework/Versions/2.7/lib -lpython2.7
#
# no further edits needed below this point
#
INCLUDE += -I../../../include

CCFLAGS += -shared \
-pthread \
-fPIC \
-fwrapv \
-O2 \
-Wall \
-fno-strict-aliasing

OBJS = driver.cpp

.SUFFIXES: .pyx

.pyx.cpp: driver.pyx ../direct.cpp
	cython --cplus $<

default: driver.so

driver.so: $(OBJS)
	$(CC) -o driver.so $(OBJS) $(CCFLAGS) $(INCLUDE) $(LIBS)

clean:
	rm -f *.cpp *.o *.so
