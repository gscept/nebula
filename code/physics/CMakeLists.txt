#-------------------------------------------------------------------------------
# Physics using PhysX 5.x
#-------------------------------------------------------------------------------

nebula_begin_lib(physics)

target_include_directories(physics PUBLIC ${FIPS_DEPLOY_DIR}/physx/include ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/physics)
target_link_libraries(physics PxLibs) 
target_precompile_headers(physics PRIVATE [["foundation/stdneb.h"]])

# The physics subsystem is format agnostic, the optional format readers under physics/formats
# pull in the dependencies of the formats that are enabled
fips_deps(foundation resource nflatbuffer input imgui)
if (N_ENABLE_PHYSICS_FORMAT_NVX)
    fips_deps(render)
endif()

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
            geometry.cc
            geometry.h
            cooking.cc
            cooking.h
            utils.h
            visualdebugger.cc
            visualdebugger.h
            physxstate.cc
            physxstate.h
            )
fips_dir(physics/formats GROUP formats)
    fips_files(formats.h formats.cc)
    if (N_ENABLE_PHYSICS_FORMAT_NVX)
        fips_files(nvx.cc)
    endif()
    if (N_ENABLE_PHYSICS_FORMAT_DDS)
        fips_files(dds.cc)
    endif()
nebula_flatc(SYSTEM physics/actor.fbs physics/collisions.fbs physics/material.fbs physics/constraints.fbs)
nebula_end_lib()
