#!/bin/bash

cd $DURINN_HOME/benchmark/pmdk-1.8-deps
./run.sh

cd $DURINN_HOME/tracer
make -j
