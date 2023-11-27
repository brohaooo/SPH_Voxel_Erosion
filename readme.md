# SPH Voxel Erosion

> final project for CGS59200


## Demo

[demo video download](https://drive.google.com/file/d/1K0T10wbgq3JublAsWS5QQ3W3ZFLo6Ow-/view?usp=drive_link)

## Build Instructions

This project uses CMake to build and is configured to support MSVC and other compilers under Windows10/11.

To build this project with a small test scene, run the following commands:

```shell
# MSVC
cmake -S . -Bbuild -DCMAKE_BUILD_TYPE=Release

#  then, open .sln file under ./build directory, and build the project
#  or, you can build using this command
cmake --build build --config Release --target Voxel_Fluid_Erosion -j 10

# Ninja
cmake -S . -Bbuild-ninja -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja --target Voxel_Fluid_Erosion -j 10

```


## Simulation Instructions

We use compile definitions to modify parameters.

To run our demo scene, use `SPH_PARTICLE_NUM=35000` and `VOXEL_FIELD_SIZE=64`.

```shell
cmake -S . -Bbuild -DCMAKE_BUILD_TYPE=Release -DSPH_PARTICLE_NUM=35000 -DVOXEL_FIELD_SIZE=64 -DOFFLINE_RENDERING=OFF
cmake --build build --config Release --target Voxel_Fluid_Erosion -j 10
```

If you want to use the offline rendering, configure it with `OFFLINE_RENDERING` option.

And you need to make a directory named `out` under `bin/<Debug/Release>`.

```shell
cmake -S . -Bbuild -DCMAKE_BUILD_TYPE=Release -DSPH_PARTICLE_NUM=35000 -DVOXEL_FIELD_SIZE=64 -DOFFLINE_RENDERING=ON
mkdir bin/Debug/out
cmake --build build --config Release --target Voxel_Fluid_Erosion -j 10
```
