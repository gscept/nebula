INCLUDE_DIRECTORIES(${Nebula3_SOURCE_DIR}/code
					${Nebula3_SOURCE_DIR}/code/addons/
					${Nebula3_SOURCE_DIR}/code/render/
					${Nebula3_SOURCE_DIR}/code/foundation/
					${Nebula3_SOURCE_DIR}/code/extlibs/					
					)





set(render_animjobsamplemix_src
      ../coreanimation/jobs/animjobsamplemix.cc
    )    
	
add_library(render_animjobsamplemix STATIC ${render_animjobsamplemix_src} )
SET_TARGET_PROPERTIES(render_animjobsamplemix PROPERTIES FOLDER "N3SDK/N3/Jobs")

set(render_animjobsample_src
      ../coreanimation/jobs/animjobsample.cc
    )    
	
add_library(render_animjobsample STATIC ${render_animjobsample_src} )
SET_TARGET_PROPERTIES(render_animjobsample PROPERTIES FOLDER "N3SDK/N3/Jobs")

set(render_charevalskeletonjob_src
      ../characters/jobs/charevalskeletonjob.cc
    )    
	
add_library(render_charevalskeletonjob STATIC ${render_charevalskeletonjob_src} )
SET_TARGET_PROPERTIES(render_charevalskeletonjob PROPERTIES FOLDER "N3SDK/N3/Jobs")

set( render_particlejob_src
      ../particles/jobs/particlejob.cc
    )    
	
add_library(render_particlejob STATIC ${render_particlejob_src} )
SET_TARGET_PROPERTIES(render_particlejob PROPERTIES FOLDER "N3SDK/N3/Jobs")

set( render_visibilityboxsystemjob_src
      ../visibility/jobs/visibilityboxsystemjob.cc
    )    
	
add_library(render_visibilityboxsystemjob STATIC ${render_visibilityboxsystemjob_src} )
SET_TARGET_PROPERTIES(render_visibilityboxsystemjob PROPERTIES FOLDER "N3SDK/N3/Jobs")

set( render_visibilityquadtreejob_src
      ../visibility/jobs/visibilityquadtreejob.cc
    )    
	
add_library(render_visibilityquadtreejob STATIC ${render_visibilityquadtreejob_src} )
SET_TARGET_PROPERTIES(render_visibilityquadtreejob PROPERTIES FOLDER "N3SDK/N3/Jobs")







N_SET_COMPILER_DEFAULTS()
# dont use precompiled headers for these (if you do, make sure to add the right precompiled header to the files as well)
#IF(N_USE_PRECOMPILED_HEADERS)
#    IF(NOT MSVC)
#        N_SET_PCH(render_animjobsamplemix stdneb.h stdneb.cc ../)
#        N_SET_PCH(render_animjobsample stdneb.h stdneb.cc ../)
#        N_SET_PCH(render_charevalskeletonjob stdneb.h stdneb.cc ../)
#        N_SET_PCH(render_particlejob stdneb.h stdneb.cc ../)
#        N_SET_PCH(render_visibilityboxsystemjob stdneb.h stdneb.cc ../)
#        N_SET_PCH(render_visibilityquadtreejob stdneb.h stdneb.cc ../)
#    ENDIF()
#ENDIF()
