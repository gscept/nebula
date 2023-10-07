# FindDotnet
# ----------
# Results are reported in the following variables:
#
#   DOTNET_FOUND               - True if dotnet executable is found
#   DOTNET_EXE                 - Dotnet executable
#   DOTNET_PATH                - Path to dotnet root
#   DOTNET_VERSION             - Dotnet version as reported by dotnet executable
#   DOTNET_APPHOST_PATH        - Path to Microsoft.NETCore.App.Host package
#   DOTNET_NETHOST_LIBRARIES   - Path to nethost.lib
#   DOTNET_NETHOST_DLL         - Path to nethost.dll
#   DOTNET_NETHOST_INCLUDE_DIR - Path to include dir for nethost
#

cmake_minimum_required(VERSION 3.5.0)

IF(DOTNET_FOUND)
    RETURN()
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

SET(DOTNET_FOUND TRUE)
SET(DOTNET_PATH NOT_FOUND)
get_filename_component(DOTNET_PATH ${DOTNET_EXE} DIRECTORY)

IF(WIN32)
    SET(APPHOST_ROOT ${DOTNET_PATH}/packs/Microsoft.NETCore.App.Host.win-x64)
ELSE()
    SET(APPHOST_ROOT NOT_FOUND)
ENDIF(WIN32)

SET(DOTNET_APPHOST_PATH NOT_FOUND)
SET(DOTNET_NETHOST_LIBRARIES)
SET(DOTNET_NETHOST_DLL)
SET(DOTNET_NETHOST_INCLUDES)

IF(EXISTS ${APPHOST_ROOT})
    file(GLOB apphost_versions LIST_DIRECTORIES true ${APPHOST_ROOT}/*)
    list(SORT apphost_versions)
    list(REVERSE apphost_versions)
    list(GET apphost_versions 0 APPHOST_LATEST_VER)
    get_filename_component(APPHOST_LATEST_VER ${APPHOST_LATEST_VER} NAME)
    if(EXISTS ${APPHOST_ROOT}/${APPHOST_LATEST_VER})
        MESSAGE("-- Found AppHost. Using latest version (${APPHOST_LATEST_VER}).")
    else()
        MESSAGE(ERROR "-- Could not find AppHost package!")
    endif()

    SET(DOTNET_APPHOST_PATH ${APPHOST_ROOT}/${APPHOST_LATEST_VER}/runtimes/win-x64/native)
    if(WIN32)
        SET(DOTNET_NETHOST_LIBRARIES ${DOTNET_APPHOST_PATH}/nethost.lib)
        SET(DOTNET_NETHOST_DLL ${DOTNET_APPHOST_PATH}/nethost.dll)
        SET(DOTNET_NETHOST_INCLUDE_DIR ${DOTNET_APPHOST_PATH})
    else()
        MESSAGE(ERROR "DotNET support for your platform is not yet supported!")
    endif()
ELSE()
    MESSAGE(ERROR "-- Could not find AppHost package!")
ENDIF()
