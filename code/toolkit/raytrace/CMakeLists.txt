#-------------------------------------------------------------------------------
# raytrace
#-------------------------------------------------------------------------------
INCLUDE_DIRECTORIES(				${Nebula3_SOURCE_DIR}/code/application
									${Nebula3_SOURCE_DIR}/code/addons									
									${Nebula3_SOURCE_DIR}/code/extlibs
									${Nebula3_SOURCE_DIR}/code/foundation
									${Nebula3_SOURCE_DIR}/code/render
									${Nebula3_SOURCE_DIR}/code/tools		
									${Nebula3_SOURCE_DIR}/code/toolkit
					)


FILE(GLOB SRC "*.cc")
FILE(GLOB HEAD "*.h")
LIST(APPEND SRCS ${SRC} ${HEAD})
SOURCE_GROUP("raytrace" FILES ${SRCS})

set(PCH_files	
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCS} ${PCH_files})

ADD_EXECUTABLE(raytrace ${ALLSRCS})
SET_TARGET_PROPERTIES(raytrace PROPERTIES FOLDER "N3SDK/Toolkit")

N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(raytrace)
N_SET_EXE_SUBSYSTEM(raytrace Console)
IF(N_USE_PRECOMPILED_HEADERS)
	N_SET_PCH(raytrace stdneb.h stdneb.cc ../)
ENDIF()
ADD_DEPENDENCIES(raytrace
	foundation
	toolkitutil
)
TARGET_LINK_LIBRARIES(raytrace 
	foundation
	toolkitutil
)
