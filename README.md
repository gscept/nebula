# Nebula engine

![Deferred Lighting using 3D clustering and GPU culling](url)
![Geometric decals, culled and calculated entirely on GPU](url)
![Volumetric fog lighting](url)
![Local fog volumes](url)

## Requirements
1. OS: Windows or Linux
2. Compiler with support for C++17.
3. GPU and drivers supporting Vulkan 1.2+
4. [CMake 3.13+](https://cmake.org/download/)
5. [Vulkan SDK 1.2+](https://vulkan.lunarg.com/sdk/home)
6. [Python 3.5+](https://www.python.org/downloads)
    * Python modules required:
        1. numpy
        2. jedi
        
        As root/admin: `python -m pip install numpy jedi`
    * Python requirements (Windows):
        1. Correct architecture (64-bit if you're building for 64-bit systems)
        2. Installed for all users
        3. Added to PATH
        4. Installed with debugging symbols and binaries

## Setup

#### Setup config and toolkit

1. `./fips set config vulkan-win64-vstudio-debug` in your project directory and `nebula-toolkit` directory
2. Build `nebula-toolkit` (external library) with the same config as your project (`$ fips build`).

#### Build project

In your project directory:

  1. `fips physx build`
  2. `fips anyfx setup`
  3. `fips build`
  4. `fips physx deploy`

#### Set environment variables

Remember to run `fips nebula` verb to set work and toolkit directory registry variables:

  * `fips nebula set work {PATH}`
  * `fips nebula set toolkit {PATH}`

## Features
Nebula is being developed continuously, which means that features keep getting added all the time. Currently, we support this:

* Completely data-driven design from bottom to top.
* Data structure suite, from containers to OS wrappers, everything is designed for performance and minimal call stacks.
* Multithreaded.
* Non-compromising SSE-accellerated and easy-to-use and understand maths library.
* Full python supported scripting layer.
* Advanced rendering framework and shaders.
* Test-benches and benchmarking.

#### Rendering
A lot of effort has been made to the Nebula rendering subsystem, where we currently support:

* Unified clustering system - fog volumes, decals and lights all go into the same structure.
* Screen-space reflections - working condition, but still work in progress.
* SSAO - Horizon-based ambient occlusion done in compute.
* Deferred lighting.
* Multi-threaded subpass recording.
* Shadow mapping for local lights and CSM for global/directional/sun light.
* Volumetric fog and lighting.
* Geometric decals. 
* CPU-GPU hybrid particle system.
* Skinning and animation.
* Scripted rendering path.
* Vulkan.
* Tonemapping.
* Asynchronous compute.
