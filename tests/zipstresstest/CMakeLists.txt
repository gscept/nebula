#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------
INCLUDE_DIRECTORIES(                ${Nebula_SOURCE_DIR}/code/application
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
SOURCE_GROUP("zipstresstest" FILES ${SRCS})

set(PCH_files   
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCS} ${PCH_files})

ADD_EXECUTABLE(zipstresstest ${ALLSRCS})
SET_TARGET_PROPERTIES(zipstresstest PROPERTIES FOLDER "N3SDK/Tests")

N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(zipstresstest)
N_SET_EXE_SUBSYSTEM(zipstresstest Console)
IF(N_USE_PRECOMPILED_HEADERS)
    N_SET_PCH(zipstresstest stdneb.h stdneb.cc ../)
ENDIF()
ADD_DEPENDENCIES(zipstresstest
    foundation
    zlib
    tinyxml
    )
TARGET_LINK_LIBRARIES(zipstresstest 
    foundation
    zlib
    tinyxml
    ${N_LIBS}
)
