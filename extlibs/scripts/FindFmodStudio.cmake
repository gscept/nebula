# Locate fmod studio api files
# This module defines:
# FMOD_INCLUDE_DIR, where to find the headers
#
# FMOD_LIBRARIES
#



IF(WIN32)
# try registry
GET_FILENAME_COMPONENT( FMOD_API_PATH [HKEY_CURRENT_USER\\SOFTWARE\\FMOD\ Studio\ API\ Windows]  ABSOLUTE)
IF(CMAKE_CL_64)
	FIND_LIBRARY(FMOD_LOW_LIB "fmod64_vc" PATHS "${FMOD_API_PATH}/api/lowlevel/lib")	
	FIND_LIBRARY(FMOD_STUDIO_LIB "fmodstudio64_vc" PATHS "${FMOD_API_PATH}/api/studio/lib")
	FIND_LIBRARY(FMOD_LOW_LIB_DEBUG "fmodL64_vc" PATHS "${FMOD_API_PATH}/api/lowlevel/lib")
	FIND_LIBRARY(FMOD_STUDIO_LIB_DEBUG "fmodstudioL64_vc" PATHS "${FMOD_API_PATH}/api/studio/lib")	
ELSE()
    FIND_LIBRARY(FMOD_LOW_LIB "fmod_vc.lib" PATHS "${FMOD_API_PATH}/api/lowlevel/lib")	
	FIND_LIBRARY(FMOD_STUDIO_LIB "fmodstudio_vc" PATHS "${FMOD_API_PATH}/api/studio/lib")
	FIND_LIBRARY(FMOD_LOW_LIB_DEBUG "fmodL_vc" PATHS "${FMOD_API_PATH}/api/lowlevel/lib")
	FIND_LIBRARY(FMOD_STUDIO_LIB_DEBUG "fmodstudioL_vc" PATHS "${FMOD_API_PATH}/api/studio/lib")
ENDIF()	
	FIND_PATH(FMOD_LOW_INCLUDE_DIR "fmod.h" "${FMOD_API_PATH}/api/lowlevel/inc" )
	FIND_PATH(FMOD_STUDIO_INCLUDE_DIR "fmod_studio.h" "${FMOD_API_PATH}/api/studio/inc" )	
ELSE()
	find_path(FMOD_LOW_INCLUDE_DIR "fmod.h" "/opt/fmodstudioapi/api/lowlevel/inc")
	find_path(FMOD_STUDIO_INCLUDE_DIR "fmod_studio.h" "/opt/fmodstudioapi/api/studio/inc")
	find_library(FMOD_STUDIO_LIB "fmodstudio" PATHS "/opt/fmodstudioapi/api/studio/lib/x86_64")
	find_library(FMOD_STUDIO_LIB_DEBUG "fmodstudioL" PATHS "/opt/fmodstudioapi/api/studio/lib/x86_64")
	find_library(FMOD_LOW_LIB "fmod" PATHS "/opt/fmodstudioapi/api/lowlevel/lib/x86_64")
	find_library(FMOD_LOW_LIB_DEBUG "fmodL" PATHS "/opt/fmodstudioapi/api/lowlevel/lib/x86_64")
ENDIF()

IF(FMOD_LOW_LIB AND FMOD_STUDIO_LIB AND FMOD_LOW_LIB_DEBUG AND FMOD_STUDIO_LIB_DEBUG)
	SET(FMOD_LIBRARIES optimized ${FMOD_LOW_LIB} debug ${FMOD_LOW_LIB_DEBUG} optimized ${FMOD_STUDIO_LIB} debug ${FMOD_STUDIO_LIB_DEBUG})
ENDIF()

SET(FMOD_INCLUDE_DIRS ${FMOD_LOW_INCLUDE_DIR} ${FMOD_STUDIO_INCLUDE_DIR} )

FIND_PACKAGE(PackageHandleStandardArgs REQUIRED)

find_package_handle_standard_args(FmodStudio REQUIRED_VARS FMOD_INCLUDE_DIRS FMOD_LIBRARIES)

MARK_AS_ADVANCED(FMOD_STUDIO_LIB FMOD_LOW_LIB FMOD_LOW_LIB_DEBUG FMOD_STUDIO_LIB_DEBUG)