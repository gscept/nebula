# Locate the FBX SDK
#
# Defines the following variables:
#
#    FBX_FOUND - Found the FBX SDK
#    FBX_VERSION - Version number
#    FBX_INCLUDE_DIRS - Include directories
#    FBX_LIBRARIES - The libraries to link to
#    FBX_DLL - Shared fbxsdk library
#
# Accepts the following variables as input:
#
#    FBX_VERSION - as a CMake variable, e.g. 2017.0.1
#    FBX_ROOT - (as a CMake or environment variable)
#               The root directory of the FBX SDK install

# adapted from https://github.com/ufz-vislab/VtkFbxConverter/blob/master/FindFBX.cmake
# which uses the MIT license (https://github.com/ufz-vislab/VtkFbxConverter/blob/master/LICENSE.txt)

if (WIN32)
    string(REGEX REPLACE "\\\\" "/" WIN_PROGRAM_FILES_X64_DIRECTORY $ENV{ProgramW6432})
    set(FBX_WIN_LOCATION_ROOT "${WIN_PROGRAM_FILES_X64_DIRECTORY}/Autodesk/FBX/FBX\ SDK")
    file(GLOB FBX_VERSIONS_AVAILABLE LIST_DIRECTORIES true "${FBX_WIN_LOCATION_ROOT}/*")
    list(SORT FBX_VERSIONS_AVAILABLE)
    list(REVERSE FBX_VERSIONS_AVAILABLE)
    list(GET FBX_VERSIONS_AVAILABLE 0 FBX_VERSION)
    message(STATUS ${FBX_VERSIONS_AVAILABLE})

    get_filename_component(FBX_VERSION ${FBX_VERSION} NAME)
endif()

if (NOT FBX_VERSION)
  set(FBX_VERSION 2020.3.4)
endif()

string(REGEX REPLACE "^([0-9]+).*$" "\\1" FBX_VERSION_MAJOR "${FBX_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*$" "\\1" FBX_VERSION_MINOR  "${FBX_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" FBX_VERSION_PATCH "${FBX_VERSION}")

if (${FBX_VERSION_MAJOR} LESS 2020 OR ${FBX_VERSION_MINOR} LESS 3)
    MESSAGE(ERROR "Could not find compatible FBX version. Requires versions 2020.3+.")
endif()

set(FBX_MAC_LOCATIONS "/Applications/Autodesk/FBX\ SDK/${FBX_VERSION}")
set(FBX_LINUX_LOCATIONS "/opt/fbx/lib/gcc/x64/release/")

set(FBX_WIN_LOCATIONS "${FBX_WIN_LOCATION_ROOT}/${FBX_VERSION}")

set(FBX_SEARCH_LOCATIONS $ENV{FBX_ROOT} ${FBX_ROOT} ${FBX_MAC_LOCATIONS} ${FBX_WIN_LOCATIONS} ${FBX_LINUX_LOCATIONS})

function(_fbx_append_debugs _endvar _library)
  if (${_library} AND ${_library}_DEBUG)
    set(_output optimized ${${_library}} debug ${${_library}_DEBUG})
  else()
    set(_output ${${_library}})
  endif()

  set(${_endvar} ${_output} PARENT_SCOPE)
endfunction()

if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  set(fbx_compiler clang)
elseif (${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
  set(fbx_compiler gcc4)
endif()

function(_fbx_find_library _name _lib _suffix)
    if (MSVC_VERSION GREATER_EQUAL 1930)
        set(VS_PREFIX vs2022)
    elseif (MSVC_VERSION GREATER_EQUAL 1920)
        set(VS_PREFIX vs2019)
    elseif (MSVC_VERSION GREATER_EQUAL 1910)
        set(VS_PREFIX vs2017)
    elseif (MSVC_VERSION GREATER_EQUAL 1900)
        set(VS_PREFIX vs2015)
    endif()

    find_library(${_name}
        NAMES ${_lib}
        HINTS ${FBX_SEARCH_LOCATIONS}
        PATH_SUFFIXES lib/${fbx_compiler}/${_suffix} lib/${fbx_compiler}/ub/${_suffix} lib/${VS_PREFIX}/x64/${_suffix}
    )

    mark_as_advanced(${_name})
endfunction()

function(_fbx_find_dll _name _lib _suffix)
    if (MSVC_VERSION GREATER_EQUAL 1930)
        set(VS_PREFIX vs2022)
    elseif (MSVC_VERSION GREATER_EQUAL 1920)
        set(VS_PREFIX vs2019)
    elseif (MSVC_VERSION GREATER_EQUAL 1910)
        set(VS_PREFIX vs2017)
    elseif (MSVC_VERSION GREATER_EQUAL 1900)
        set(VS_PREFIX vs2015)
    endif()

	find_file(${_name}
		NAMES ${_lib}.dll
		HINTS ${FBX_SEARCH_LOCATIONS}
		PATH_SUFFIXES lib/${fbx_compiler}/${_suffix} lib/${fbx_compiler}/ub/${_suffix} lib/${VS_PREFIX}/x64/${_suffix}
	)
	mark_as_advanced(${_name})
endfunction()



find_path(FBX_INCLUDE_DIR fbxsdk.h
  PATHS ${FBX_SEARCH_LOCATIONS}
  PATH_SUFFIXES include
)
mark_as_advanced(FBX_INCLUDE_DIR)

if (WIN32)
  _fbx_find_library(FBX_LIBRARY libfbxsdk release)
  _fbx_find_library(FBX_LIBRARY_DEBUG libfbxsdk debug)
  _fbx_find_dll(FBX_DLL libfbxsdk release)
  _fbx_find_dll(FBX_DLL_DEBUG libfbxsdk debug)
elseif (APPLE)
  find_library(CARBON NAMES Carbon)
  find_library(SYSTEM_CONFIGURATION NAMES SystemConfiguration)
  _fbx_find_library(FBX_LIBRARY libfbxsdk.a release)
  _fbx_find_library(FBX_LIBRARY_DEBUG libfbxsdk.a debug)
else ()
  _fbx_find_library(FBX_LIBRARY libfbxsdk.a release)
endif()

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(FBX DEFAULT_MSG FBX_LIBRARY FBX_INCLUDE_DIR)

if (FBX_FOUND)
  MESSAGE("-- Using FBX version: ${FBX_VERSION}")
  set(FBX_INCLUDE_DIRS ${FBX_INCLUDE_DIR})
  _fbx_append_debugs(FBX_LIBRARIES FBX_LIBRARY)
  add_definitions(-DFBXSDK_NEW_API)

  if (APPLE)
    set(FBX_LIBRARIES ${FBX_LIBRARIES} ${CARBON} ${SYSTEM_CONFIGURATION})
  endif()
endif()
