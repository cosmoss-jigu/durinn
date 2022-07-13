#!/bin/bash

CURR_DIR=$DURINN_HOME/benchmark/pmdk-1.8-deps
LIBPMEM_DIR=$DURINN_HOME/pmdk-1.8/src/libpmem
LIBPMEMOBJ_DIR=$DURINN_HOME/pmdk-1.8/src/libpmemobj

cd $LIBPMEM_DIR
make -j64
sudo make install prefix=/usr
rm -f $LIBPMEM_DIR/*.d

cd $LIBPMEMOBJ_DIR
make -j64
sudo make install prefix=/usr
rm -f $LIBPMEMOBJ_DIR/*.d

cd $CURR_DIR
cp $LIBPMEMOBJ_DIR/*.bc .
