#-------------------------------------------------------------------------------
# Physics using PhysX 4.x
#-------------------------------------------------------------------------------

fips_begin_module(physics)
fips_include_directories(${FIPS_DEPLOY_DIR}/physx/include ${CMAKE_CURRENT_SOURCE_DIR})
fips_deps(foundation)
fips_files(physicsinterface.h physicsinterface.cc)
fips_dir(physics)
fips_files( 
            shapes.cc
            shapes.h
            actor.cc
            actor.h
            callbacks.cc
            callbacks.h
            utils.h
            visualdebugger.cc
            visualdebugger.h
            )
fips_end_module()
target_link_libraries(physics PxLibs) 