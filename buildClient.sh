set -x
cd rucbase_client
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j3