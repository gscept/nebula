#-------------------------------------------------------------------------------
# Physics using PhysX 5.x
#-------------------------------------------------------------------------------

nebula_begin_lib(physics)

target_include_directories(physics PUBLIC ${FIPS_DEPLOY_DIR}/physx/include ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/physics)
target_link_libraries(physics PxLibs) 
target_precompile_headers(physics PRIVATE [["foundation/stdneb.h"]])
fips_deps(foundation resource render nflatbuffer)
fips_files(physicsinterface.h physicsinterface.cc)
fips_dir(physics)
fips_files( 
            actorcontext.cc
            actorcontext.h
            charactercontext.cc
            charactercontext.h
            streamactorpool.cc
            streamactorpool.h
            callbacks.cc
            callbacks.h
            debugui.cc
            debugui.h
            utils.h
            visualdebugger.cc
            visualdebugger.h
            physxstate.cc
            physxstate.h
            )
nebula_flatc(SYSTEM physics/actor.fbs physics/collisions.fbs physics/material.fbs physics/constraints.fbs)
nebula_end_lib()

