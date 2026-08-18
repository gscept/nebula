# FindDotnet
# ----------
# Results are reported in the following variables:
#
#   DOTNET_FOUND               - True if dotnet executable is found
#   DOTNET_EXE                 - Dotnet executable
#   DOTNET_PATH                - Path to dotnet root
#   DOTNET_VERSION             - Dotnet version as reported by dotnet executable
#   DOTNET_APPHOST_PATH        - Path to the native runtime dir of the AppHost pack
#   DOTNET_NETHOST_LIBRARIES   - Path to the nethost static lib (nethost.lib / libnethost.a)
#   DOTNET_NETHOST_DLL         - Path to the nethost runtime lib (nethost.dll / libnethost.so)
#   DOTNET_NETHOST_INCLUDE_DIR - Path to include dir for nethost
#

cmake_minimum_required(VERSION 3.5.0)

IF(NOT NEBULA_DOTNET_MAJOR_VERSION)
    SET(NEBULA_DOTNET_MAJOR_VERSION 8)
ENDIF()

FIND_PROGRAM(DOTNET_EXE dotnet)
SET(DOTNET_MODULE_DIR ${CMAKE_CURRENT_LIST_DIR})

IF(NOT DOTNET_EXE)
    SET(DOTNET_FOUND FALSE)
    IF(Dotnet_FIND_REQUIRED)
        MESSAGE(SEND_ERROR "Command 'dotnet' is not found.")
    ENDIF()
    RETURN()
ENDIF()

EXECUTE_PROCESS(
    COMMAND ${DOTNET_EXE} --version
    OUTPUT_VARIABLE DOTNET_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

MESSAGE("-- Found .NET toolchain: ${DOTNET_EXE} (version ${DOTNET_VERSION})")

EXECUTE_PROCESS(
    COMMAND ${DOTNET_EXE} --list-sdks
    OUTPUT_VARIABLE DOTNET_SDK_LIST
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
STRING(REGEX MATCHALL "${NEBULA_DOTNET_MAJOR_VERSION}\\.[0-9]+\\.[0-9]+" DOTNET_SDK_VERSIONS "${DOTNET_SDK_LIST}")
LIST(SORT DOTNET_SDK_VERSIONS ORDER DESCENDING COMPARE NATURAL)

IF(NEBULA_DOTNET_SDK_VERSION)
    LIST(FIND DOTNET_SDK_VERSIONS ${NEBULA_DOTNET_SDK_VERSION} DOTNET_SDK_VERSION_INDEX)
    IF(DOTNET_SDK_VERSION_INDEX EQUAL -1)
        SET(DOTNET_FOUND FALSE)
        MESSAGE(SEND_ERROR "-- Requested .NET SDK ${NEBULA_DOTNET_SDK_VERSION} is not installed.")
        RETURN()
    ENDIF()
    SET(DOTNET_SDK_VERSION ${NEBULA_DOTNET_SDK_VERSION})
ELSEIF(DOTNET_SDK_VERSIONS)
    LIST(GET DOTNET_SDK_VERSIONS 0 DOTNET_SDK_VERSION)
ELSE()
    SET(DOTNET_FOUND FALSE)
    MESSAGE(SEND_ERROR "-- No .NET ${NEBULA_DOTNET_MAJOR_VERSION}.x SDK is installed.")
    RETURN()
ENDIF()
MESSAGE("-- Using .NET SDK ${DOTNET_SDK_VERSION} and target framework net${NEBULA_DOTNET_MAJOR_VERSION}.0")

# On Linux the dotnet executable is often reachable through a symlink
# (e.g. /usr/bin/dotnet -> ../share/dotnet/dotnet). Resolve the real path
# first so the dotnet root (and hence the 'packs' directory) is found.
get_filename_component(DOTNET_EXE_REAL ${DOTNET_EXE} REALPATH)
get_filename_component(DOTNET_PATH ${DOTNET_EXE_REAL} DIRECTORY)

# Map the current platform to the dotnet runtime id used by the AppHost
# pack and its native runtime folder.
IF(WIN32)
    SET(DOTNET_RID win-x64)
ELSEIF(LINUX)
    SET(DOTNET_RID linux-x64)
ELSEIF(APPLE)
    SET(DOTNET_RID osx-x64)
ELSE()
    SET(DOTNET_RID unknown)
ENDIF()

SET(APPHOST_ROOT ${DOTNET_PATH}/packs/Microsoft.NETCore.App.Host.${DOTNET_RID})

SET(DOTNET_APPHOST_PATH NOT_FOUND)
SET(DOTNET_NETHOST_LIBRARIES)
SET(DOTNET_NETHOST_DLL)
SET(DOTNET_NETHOST_INCLUDE_DIR)

IF(EXISTS ${APPHOST_ROOT})
    file(GLOB apphost_versions LIST_DIRECTORIES true ${APPHOST_ROOT}/${NEBULA_DOTNET_MAJOR_VERSION}.*)
    list(SORT apphost_versions ORDER DESCENDING COMPARE NATURAL)
    if(apphost_versions)
        list(GET apphost_versions 0 APPHOST_LATEST_VER)
        get_filename_component(APPHOST_LATEST_VER ${APPHOST_LATEST_VER} NAME)
    endif()

    if(NOT APPHOST_LATEST_VER OR NOT EXISTS ${APPHOST_ROOT}/${APPHOST_LATEST_VER})
        SET(DOTNET_FOUND FALSE)
        MESSAGE(SEND_ERROR "-- Could not find an AppHost version under ${APPHOST_ROOT}")
        RETURN()
    endif()

    MESSAGE("-- Found AppHost. Using latest version (${APPHOST_LATEST_VER}).")
    SET(DOTNET_APPHOST_PATH ${APPHOST_ROOT}/${APPHOST_LATEST_VER}/runtimes/${DOTNET_RID}/native)

    IF(WIN32)
        SET(DOTNET_NETHOST_LIBRARIES ${DOTNET_APPHOST_PATH}/nethost.lib)
        SET(DOTNET_NETHOST_DLL ${DOTNET_APPHOST_PATH}/nethost.dll)
    ELSE()
        SET(DOTNET_NETHOST_LIBRARIES ${DOTNET_APPHOST_PATH}/libnethost.a)
        SET(DOTNET_NETHOST_DLL ${DOTNET_APPHOST_PATH}/libnethost.so)
    ENDIF()
    SET(DOTNET_NETHOST_INCLUDE_DIR ${DOTNET_APPHOST_PATH})
ELSE()
    SET(DOTNET_FOUND FALSE)
    MESSAGE(SEND_ERROR "-- Could not find AppHost package at ${APPHOST_ROOT}")
    RETURN()
ENDIF()

MESSAGE("-- Using nethost library : ${DOTNET_NETHOST_LIBRARIES}")
MESSAGE("-- Using nethost runtime: ${DOTNET_NETHOST_DLL}")
MESSAGE("-- Using nethost include: ${DOTNET_NETHOST_INCLUDE_DIR}")
SET(DOTNET_FOUND TRUE)
