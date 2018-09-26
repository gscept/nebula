#-------------------------------------------------------------------------------
# benchmarkaddon
#-------------------------------------------------------------------------------
INCLUDE_DIRECTORIES(				${Nebula3_SOURCE_DIR}/code/application
									${Nebula3_SOURCE_DIR}/code/addons									
									${Nebula3_SOURCE_DIR}/code/application/basegamefeature
									${Nebula3_SOURCE_DIR}/code/application/corefeature
									${Nebula3_SOURCE_DIR}/code/application/graphicsfeature
									${Nebula3_SOURCE_DIR}/code/application/physicsfeature
									${Nebula3_SOURCE_DIR}/code/extlibs
									${Nebula3_SOURCE_DIR}/code/extlibs/ode
									${Nebula3_SOURCE_DIR}/code/foundation
									${Nebula3_SOURCE_DIR}/code/render
									${Nebula3_SOURCE_DIR}/code/sui
									${Nebula3_SOURCE_DIR}/code/tools		
									${Nebula3_SOURCE_DIR}/code/toolkit
									${Nebula3_SOURCE_DIR}/code/benchmarks/
									${Nebula3_SOURCE_DIR}/code
					)

FILE(GLOB SRC "*.cc")
FILE(GLOB HEAD "*.h")
LIST(APPEND SRCS ${SRC} ${HEAD})
SOURCE_GROUP("benchmarkaddon" FILES ${SRCS})

set(PCH_files	
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCS} ${PCH_files})

ADD_EXECUTABLE(benchmarkaddon ${ALLSRCS})
SET_TARGET_PROPERTIES(benchmarkaddon PROPERTIES FOLDER "N3SDK/Benchmarks")

N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(benchmarkaddon)
N_SET_EXE_SUBSYSTEM(benchmarkaddon Windows)
IF(N_USE_PRECOMPILED_HEADERS)
	N_SET_PCH(benchmarkaddon stdneb.h stdneb.cc ../)
ENDIF()
ADD_DEPENDENCIES(benchmarkaddon
	foundation
	zlib
	tinyxml
	render
	render_visibilityboxsystemjob render_visibilityquadtreejob
	render_animjobsamplemix render_animjobsample render_particlejob render_charevalskeletonjob
	foundation framecapture benchmarkbase
	attr db sqlite
	)
TARGET_LINK_LIBRARIES(benchmarkaddon 
	foundation
	zlib
	render
	render_visibilityboxsystemjob render_visibilityquadtreejob
	render_animjobsamplemix render_animjobsample render_particlejob render_charevalskeletonjob
	foundation framecapture
	tinyxml benchmarkbase
	attr db sqlite
	${N_LIBS}
)
