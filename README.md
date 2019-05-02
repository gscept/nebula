# Nebula engine

## Requirements
1. OS: Windows or Linux
2. Compiler with support for C++14.
3. GPU supporting Vulkan
4. [CMake 3.13+](https://cmake.org/download/)
5. [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
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
1. `./fips set config vulkan-win64-vstudio-debug` in your project directory

Remember to run `fips nebula` verb to set work and toolkit directory registry variables.

