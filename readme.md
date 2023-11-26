# SPH Voxel Erosion

> final project for CGS59200

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

Or you can just open MSVC and it will help you do all the thing

## Simulation Instructions

If you want to use the offline rendering, configure it with `OFFLINE_RENDERING` option.

And you need to make a directory named `out` under `bin/<Debug/Release>`.

```powershell
cmake -S . -Bbuild -DCMAKE_BUILD_TYPE=<Release/Debug> -DOFFLINE_RENDERING=ON
mkdir bin/<Debug/Release>/out
```
