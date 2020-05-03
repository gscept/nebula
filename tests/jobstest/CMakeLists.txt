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
SOURCE_GROUP("jobstest" FILES ${SRCS})

set(PCH_files	
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCS} ${PCH_files})

ADD_EXECUTABLE(jobstest ${ALLSRCS})
SET_TARGET_PROPERTIES(jobstest PROPERTIES FOLDER "N3SDK/Tests")

N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(jobstest)
N_SET_EXE_SUBSYSTEM(jobstest Windows)
IF(N_USE_PRECOMPILED_HEADERS)
	N_SET_PCH(jobstest stdneb.h stdneb.cc ../)
ENDIF()
ADD_DEPENDENCIES(jobstest
	foundation
	tinyxml
	zlib
	)
TARGET_LINK_LIBRARIES(jobstest 
	foundation
	tinyxml
	zlib
	render_particlejob
	${N_LIBS}
)
