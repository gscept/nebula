#-------------------------------------------------------------------------------
# Physics using PhysX 4.x
#-------------------------------------------------------------------------------

nebula_begin_module(physics)
fips_include_directories(${FIPS_DEPLOY_DIR}/physx/include ${CMAKE_CURRENT_SOURCE_DIR})
fips_deps(foundation resource render nflatbuffer)
fips_files(physicsinterface.h physicsinterface.cc)
fips_dir(physics)
fips_files( 
            actorcontext.cc
            actorcontext.h
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
nebula_flatc(SYSTEM physics/actor.fbs physics/collisions.fbs physics/material.fbs)
nebula_end_module()
target_link_libraries(physics PxLibs) 
target_include_directories(physics PUBLIC ${CODE_ROOT}/physics)
