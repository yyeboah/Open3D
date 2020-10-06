#!/usr/bin/env bash
set -e

rm -rf $HOME/repo/Open3D/build
mkdir -p $HOME/repo/Open3D/build
pushd $HOME/repo/Open3D/build

eval "$(conda shell.bash hook)"
conda activate open3d3
which python

cmake -DBUILD_RPC_INTERFACE=OFF -DENABLE_HEADLESS_RENDERING=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_FILAMENT_FROM_SOURCE=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_SHARED_LIBS=OFF -DBUILD_GUI=ON -DBUILD_TENSORFLOW_OPS=OFF -DBUILD_PYTORCH_OPS=OFF -DBUILD_CUDA_MODULE=OFF -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DBUILD_UNIT_TESTS=ON -DTHIRD_PARTY_DOWNLOAD_DIR=$HOME/open3d_downloads -DCMAKE_INSTALL_PREFIX=~/open3d_install ..

make tests -j$(nproc)

./bin/tests --gtest_filter="-*Sum*"

make install-pip-package -j

python ../eigen_test.py
