# SPH Voxel Erosion

final project for CGS59200

## Build Instructions

This project uses CMake to build and is configured to support MSVC and other compilers under Windows10/11.

To build this project, run the following commands:

```powershell
# MSVC
cmake -S . -Bbuild -DCMAKE_BUILD_TYPE=<Release/Debug>

#  then, open .sln file under ./build directory, and build the project
#  or, you can build using this command
cmake --build build --config <Release/Debug>

# Ninja
cmake -S . -Bbuild-ninja -GNinja -DCMAKE_BUILD_TYPE=<Release/Debug>
cmake --build build-ninja

```
