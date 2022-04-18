#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------

FILE(GLOB SRC "gamestates/*.cc")
FILE(GLOB HEAD "gamestates/*.h")
LIST(APPEND SRCSGAMESTATES ${SRC} ${HEAD})
SOURCE_GROUP("levelviewer\\gamestates" FILES ${SRCSGAMESTATES})

FILE(GLOB SRC "*.cc")
FILE(GLOB HEAD "*.h")
LIST(APPEND SRCSBASE ${SRC} ${HEAD})
SOURCE_GROUP("levelviewer" FILES ${SRCSBASE})

set(PCH_files   
    ../stdneb.cc
)
SOURCE_GROUP("Precompiled" FILES ${PCH_files})

LIST(APPEND ALLSRCS ${SRCSBASE} ${SRCSGAMESTATES} ${PCH_files} levelviewer.rc)

N_WRAP_ADD_PROJ_NIDL_FILES(levelviewer ALLSRCS nidl/levelviewercommands.nidl)

ADD_EXECUTABLE(levelviewer ${ALLSRCS})
SET_TARGET_PROPERTIES(levelviewer PROPERTIES FOLDER "N3SDK/Toolkit")
TARGET_INCLUDE_DIRECTORIES(levelviewer PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

N_SET_COMPILER_DEFAULTS()
N_SET_EXE_LINKER_DEFAULTS(levelviewer)
N_SET_EXE_SUBSYSTEM(levelviewer Windows)
IF(N_USE_PRECOMPILED_HEADERS)
    N_SET_PCH(levelviewer stdneb.h stdneb.cc ../)
ENDIF()
ADD_DEPENDENCIES(levelviewer
    graphicsfeature physicsfeature scriptingfeature stateobjectfeature ui
    remoteinterface
    dynui   
    )
TARGET_LINK_LIBRARIES(levelviewer   
    remoteinterface
    graphicsfeature physicsfeature scriptingfeature stateobjectfeature ui   inputfeature
    dynui   
)
