#-------------------------------------------------------------------------------
# editor managers
#-------------------------------------------------------------------------------

set(SOURCES_files
	editorbasegamefeature.cc
	editorbasegamefeature.h
	editorblueprintmanager.h
	editorblueprintmanager.cc
)
source_group("managers" FILES ${SOURCES_files})

set(SOURCES
	${SOURCES_files}
)

set(SOURCES_files		
	../stdneb.cc
)
	
SOURCE_GROUP("Precompiled" FILES ${SOURCES_files})
	set(SOURCES_SRCS
	${SOURCES}
	${SOURCES_files}
)

add_library( editorfeatures STATIC ${SOURCES_SRCS} )
TARGET_LINK_LIBRARIES(editorfeatures application toolkitutil idldocument)
SET_TARGET_PROPERTIES(editorfeatures PROPERTIES FOLDER "N3SDK/Toolkit")
N_SET_COMPILER_DEFAULTS()
IF(N_USE_PRECOMPILED_HEADERS)
    N_SET_PCH(editorfeatures stdneb.h stdneb.cc ../)
ENDIF()