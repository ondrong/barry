#!/bin/sh

export CC="ccache gcc"
export CXX="ccache g++"

#export CXXFLAGS="-Wall -Werror -pedantic -O0 -g -pg"
export CXXFLAGS="-Wall -Werror -pedantic -O0 -g"
#export CXX="g++-3.3"
export PKG_CONFIG_PATH=/home/cdfrey/Contract/netdirect/syncberry/cvs/barry1/rootdir/lib/pkgconfig

./configure \
	--prefix=/home/cdfrey/Contract/netdirect/syncberry/cvs/barry1/rootdir

