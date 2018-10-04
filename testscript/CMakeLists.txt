#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------
INCLUDE_DIRECTORIES(				${Nebula_SOURCE_DIR}/code/application
									${Nebula_SOURCE_DIR}/code/addons									
									${Nebula_SOURCE_DIR}/code/application/basegamefeature
									${Nebula_SOURCE_DIR}/code/application/corefeature
									${Nebula_SOURCE_DIR}/code/application/graphicsfeature
									${Nebula_SOURCE_DIR}/code/application/physicsfeature
									${Nebula_SOURCE_DIR}/code/extlibs									
									${Nebula_SOURCE_DIR}/code/foundation
									${Nebula_SOURCE_DIR}/code/render
									${Nebula_SOURCE_DIR}/code/sui
									${Nebula_SOURCE_DIR}/code/tools		
									${Nebula_SOURCE_DIR}/code/toolkit
									${Nebula_SOURCE_DIR}/code/tests/
									${Nebula_SOURCE_DIR}/code
					)


FILE(GLOB SRC "*.cc")
FILE(GLOB HEAD "*.h")
LIST(APPEND SRCS ${SRC} ${HEAD})
SOURCE_GROUP("testscript" FILES ${SRCS})

set(PCH_files	
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCS} ${PCH_files})

ADD_EXECUTABLE(testscript ${ALLSRCS})
SET_TARGET_PROPERTIES(testscript PROPERTIES FOLDER "N3SDK/Tests")

N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(testscript)
N_SET_EXE_SUBSYSTEM(testscript Windows)
IF(N_USE_PRECOMPILED_HEADERS)
	N_SET_PCH(testscript stdneb.h stdneb.cc ../)
ENDIF()
ADD_DEPENDENCIES(testscript
	foundation
	foundation_NIDLS
	scripting
	scripting_NIDLS
	zlib
	testbase
	lua
	tinyxml
	)
TARGET_LINK_LIBRARIES(testscript 
	foundation
	foundation_NIDLS
	zlib
	testbase
	tinyxml
	lua
	scripting
	scripting_NIDLS
	${N_LIBS}
)
