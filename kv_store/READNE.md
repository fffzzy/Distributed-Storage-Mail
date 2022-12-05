# README



## gRPC

#### Useful Links

* [gRPC quick start](https://grpc.io/docs/languages/cpp/quickstart/)
  * Need to install gRPC manually.
  * Add following **Include Path** to **VSCode** to pass the source file check

![image-20221204211104173](snaps/VSCodeC++PluginIncludePath.png)

* [Protocol Buffer Basics C++](https://developers.google.com/protocol-buffers/docs/cpptutorial#compiling-your-protocol-buffers)
  * Includes the usage of protobuf

#### Protocol Buffer

All protobuf files should be included in **```protos```** directory.



#### Compile

Should use **CMake** command to compile the **gRPC**, since make is deprecated. 

```shell
# Remember to set installation dir, you can choose another dir
export MY_INSTALL_DIR=/home/cis5050/.local

mkdir -p cmake/build
pushd cmake/build # similar to 'cd cmake/build', but can use 'popd' to go back.
cmake -DCMAKE_PREFIX_PATH=$MY_INSTALL_DIR ../..

make -j 4
```