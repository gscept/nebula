#-------------------------------------------------------------------------------
# Physics
#-------------------------------------------------------------------------------

IF(MSVC12)
    SET(PHYSX_LIB_DIR ${PHYSX_DIR}/lib/vc12win32)
ELSEIF(MSVC14)
    SET(PHYSX_LIB_DIR ${PHYSX_DIR}/lib/vc14win32)		
ENDIF()
SET(PHYSX_RELEASE_LIBS_NAMES 	LowLevel.lib LowLevelCloth.lib PhysX3_x86.lib Phys3XCharacterKinematic_x86.lib PhysX3Common_x86.lib PhysX3Cooking_x86.lib PhysX3Extensions.lib PhysX3Vehicle.lib 
                        PvdRuntime.lib PxTask.lib SceneQuery.lib SimulationController.lib PhysXVisualDebuggerSDK.lib PhysXProfileSDK.lib)
SET(PHYSX_CHECKED_LIBS_NAMES 	LowLevelCHECKED.lib LowLevelClothCHECKED.lib PhysX3CHECKED_x86.lib PhysX3CharacterKinematicCHECKED_x86.lib PhysX3CommonCHECKED_x86.lib PhysX3CookingCHECKED_x86.lib PhysX3ExtensionsCHECKED.lib PhysX3VehicleCHECKED.lib 
                        PvdRuntimeCHECKED.lib PxTaskCHECKED.lib SceneQueryCHECKED.lib SimulationController.lib PhysXVisualDebuggerSDKCHECKED.lib PhysXProfileSDKCHECKEd.lib)
SET(PHYSX_DEBUG_LIBS_NAMES 	LowLevelDEBUG.lib LowLevelClothDEBUG.lib PhysX3DEBUG_x86.lib PhysX3CharacterKinematicDEBUG_x86.lib PhysX3CommonDEBUG_x86.lib PhysX3CookingDEBUG_x86.lib PhysX3ExtensionsDEBUG.lib PhysX3VehicleDEBUG.lib 
                        PvdRuntimeDEBUG.lib PxTaskDEBUG.lib SceneQueryDEBUG.lib SimulationController.lib PhysXVisualDebuggerSDKDEBUG.lib PhysXProfileSDKDEBUG.lib)
SET(PHYSX_RELASE_LIBS)
FOREACH(CUR_LIB ${PHYSX_RELEASE_LIBS_NAMES})
    LIST(APPEND PHYSX_RELEASE_LIBS ${PHYSX_LIB_DIR}/${CUR_LIB})
ENDFOREACH()
SET(PHYSX_CHECKED_LIBS)
FOREACH(CUR_LIB ${PHYSX_CHECKED_LIBS_NAMES})
    LIST(APPEND PHYSX_CHECKED_LIBS ${PHYSX_LIB_DIR}/${CUR_LIB})
ENDFOREACH()
SET(PHYSX_DEBUG_LIBS)
FOREACH(CUR_LIB ${PHYSX_DEBUG_LIBS_NAMES})
    LIST(APPEND PHYSX_DEBUG_LIBS ${PHYSX_LIB_DIR}/${CUR_LIB})
ENDFOREACH()

fips_begin_module(physics-physx)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR})    
    fips_deps(foundation physics)
    
    # ${PHYSX_CHECKED_LIBS} is faster with debug syms
    fips_libs_debug(${PHYSX_DEBUG_LIBS})
    fips_libs_release(${PHYSX_RELASE_LIBS})
            
    fips_files(
        physxscene.h
        physxscene.cc
        physxbody.h
        physxbody.cc
        physxcollider.h
        physxcollider.cc				
        physxstatic.h
        physxstatic.cc
        physxjoint.cc
        physxjoint.h
        physxprobe.h
        physxprobe.cc
        physxcharacter.h
        physxcharacter.cc		
        physxphysicsserver.h
        physxphysicsserver.cc		
        physxphysicsmesh.cc
        physxphysicsmesh.h
        physxpointtopoint.cc
        physxpointtopoint.h
        physxslider.cc
        physxslider.h
        physxhinge.cc
        physxhinge.h
        physxuniversaljoint.cc
        physxuniversaljoint.h
        physxutils.h
    )    
fips_end_module()
