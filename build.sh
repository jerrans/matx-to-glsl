#!/bin/sh

mkdir -p ./build/MaterialX
cd build/MaterialX
cmake ../../MaterialX/
make -j40
make install
cd ../
cmake ../ -DCMAKE_MODULE_PATH=`pwd`/../MaterialX/cmake/modules -DMATERIALX_ROOT=`pwd`/MaterialX/installed
make -j40
