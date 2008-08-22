#!/bin/sh

CHOICE="system"

export CC="ccache gcc"
export CXX="ccache g++"

#export CXXFLAGS="-Wall -Werror -pedantic -O0 -g"
export CXXFLAGS="-Wall -Werror -O0 -g"
#export PKG_CONFIG_PATH=/home/cdfrey/Contract/netdirect/syncberry/cvs/barry1/rootdir/lib/pkgconfig:/home/cdfrey/software/opensync/0.22/rootdir/lib/pkgconfig

./configure \
	--prefix=/home/cdfrey/Contract/netdirect/syncberry/cvs/barry1/rootdir \
	--enable-gui \
	--enable-opensync-plugin

