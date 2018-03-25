#-------------------------------------------------------------------------------
# Setup
#-------------------------------------------------------------------------------
INCLUDE_DIRECTORIES(				${Nebula3_SOURCE_DIR}/code/foundation
									${Nebula3_SOURCE_DIR}/code/tools/setup
									${Nebula3_SOURCE_DIR}/code/extlibs
									${Nebula3_SOURCE_DIR}/code/toolkit
									${Nebula3_SOURCE_DIR}/code/toolkit/toolkitutil
									${Nebula3_SOURCE_DIR}/code
					)


FILE(GLOB SRC "*.cc")
FILE(GLOB HEAD "*.h")
LIST(APPEND SRCS ${SRC} ${HEAD})
SOURCE_GROUP("setup" FILES ${SRCS})

set(PCH_files	
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCS} ${PCH_files})

ADD_EXECUTABLE(setup ${ALLSRCS})
SET_TARGET_PROPERTIES(setup PROPERTIES FOLDER "N3SDK/Toolkit")

N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(setup)
N_SET_EXE_SUBSYSTEM(setup Console)
IF(N_USE_PRECOMPILED_HEADERS)
	N_SET_PCH(setup stdneb.h stdneb.cc ../)
ENDIF()


ADD_DEPENDENCIES(setup foundation toolkitutil tinyxml zlib base64)
TARGET_LINK_LIBRARIES(setup foundation toolkitutil tinyxml zlib base64 StackWalker
					 ${N_LIBS}
					 )