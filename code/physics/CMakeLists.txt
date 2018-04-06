#-------------------------------------------------------------------------------
# Physics
#-------------------------------------------------------------------------------

	set(SOURCES_files_physics

		physicsserver.cc
		physicsserver.h

		visualdebuggerserver.cc
		visualdebuggerserver.h

		scene.h
		scene.cc

		physicsbody.h
		physicsbody.cc

		physicsobject.h
		physicsobject.cc

		collider.h
		collider.cc

		contactlistener.h
		contactlistener.cc

		staticobject.h
		staticobject.cc

		filterset.h
		filterset.cc

		materialtable.h
		materialtable.cc

		contact.h
		contact.cc

		physicsprobe.h
		physicsprobe.cc

		model/templates.h
		model/templates.cc

		joint.h
		joint.cc

		character.h
		character.cc

		#controller.h
		#controller.cc

		#vehicle.h
		#vehicle.cc

		#material.h
		#material.cc
    )
    set(SOURCES_files_physics_util
		util/areaimpulse.h
		util/areaimpulse.cc

		util/explosionareaimpulse.h
		util/explosionareaimpulse.cc

		util/mousegripper.h
		util/mousegripper.cc
	)

    set(SOURCES_files_physics_resource
		resource/streamphysicsmeshloader.h
		resource/streamphysicsmeshloader.cc
		resource/physicsmesh.h
		resource/physicsmesh.cc
		resource/physicsmeshbase.h
		resource/physicsmeshbase.cc
		resource/managedphysicsmesh.h
		resource/managedphysicsmesh.cc
		resource/managedphysicsmodel.h
		resource/managedphysicsmodel.cc
		model/physicsmodel.h
		model/physicsmodel.cc
		resource/streamphysicsmodelloader.h
		resource/streamphysicsmodelloader.cc
	)

    set(SOURCES_files_physics_base
    	base/basescene.h
		base/basescene.cc
		base/basebody.h
		base/basebody.cc
		base/basecollider.h
		base/basecollider.cc
		base/basecontact.h
		base/basecontact.cc
		base/basecontactlistener.h
		base/basecontactlistener.cc
		base/basestatic.h
		base/basestatic.cc
		base/baseprobe.h
		base/baseprobe.cc
		base/basecharacter.h
		base/basecharacter.cc
		#base/basecontroller.h
		#base/basecontroller.cc
		#base/basevehicle.h
		#base/basevehicle.cc
		base/physicsserverbase.h
		base/physicsserverbase.cc
		base/basevisualdebuggerserver.h
		base/basevisualdebuggerserver.cc
		#base/basematerial.h
		#base/basematerial.cc
    )
	
	 set(SOURCES_files_physics_physx
    	physx/physxscene.h
		physx/physxscene.cc
		physx/physxbody.h
		physx/physxbody.cc
		physx/physxcollider.h
		physx/physxcollider.cc				
		physx/physxstatic.h
		physx/physxstatic.cc
		physx/physxjoint.cc
		physx/physxjoint.h
		physx/physxprobe.h
		physx/physxprobe.cc
		physx/physxcharacter.h
		physx/physxcharacter.cc		
		physx/physxphysicsserver.h
		physx/physxphysicsserver.cc		
		physx/physxphysicsmesh.cc
		physx/physxphysicsmesh.h
		physx/physxpointtopoint.cc
		physx/physxpointtopoint.h
		physx/physxslider.cc
		physx/physxslider.h
		physx/physxhinge.cc
		physx/physxhinge.h
		physx/physxuniversaljoint.cc
		physx/physxuniversaljoint.h
		physx/physxutils.h
    )

    SET(SOURCES_files_bullet 
		bullet/bulletcharacter.h
		bullet/bulletcharacter.cc
		bullet/bulletbody.cc
		bullet/bulletbody.h
		bullet/bulletcollider.cc
		bullet/bulletcollider.h
		bullet/bulletphysicsmesh.cc
		bullet/bulletphysicsmesh.h
		bullet/bulletphysicsserver.cc
		bullet/bulletphysicsserver.h
		bullet/bulletscene.cc
		bullet/bulletscene.h
		bullet/bulletstatic.h
		bullet/bulletstatic.cc
		bullet/bulletprobe.h
		bullet/bulletprobe.cc
		bullet/bulletjoint.h
		bullet/bulletjoint.cc
		bullet/conversion.h
		bullet/nebulamotionstate.h
		bullet/debugdrawer.h
		bullet/debugdrawer.cc
		bullet/bullethinge.h
		bullet/bulletuniversaljoint.h
    	bullet/bullethinge.cc
		bullet/bulletuniversaljoint.cc
		bullet/bulletslider.h
		bullet/bulletslider.cc
		bullet/bulletpointtopoint.h
		bullet/bulletpointtopoint.cc
		bullet/raycallbacks.h
	)

	SET(SOURCES_files_havok
		havok/conversion.h
		havok/havokutil.cc
		havok/havokutil.h
		havok/havokdebugdrawer.cc
		havok/havokdebugdrawer.h

		havok/havokphysicsserver.cc
		havok/havokphysicsserver.h
		havok/havokvisualdebuggerserver.cc
		havok/havokvisualdebuggerserver.h
		havok/havokstatic.cc
		havok/havokstatic.h
		havok/havokscene.cc
		havok/havokscene.h
		havok/havokprobe.cc
		havok/havokprobe.h
		havok/havokbody.cc
		havok/havokbody.h
		havok/havokjoint.cc
		havok/havokjoint.h
		havok/havokcollider.cc
		havok/havokcollider.h
		havok/havokcontact.cc
		havok/havokcontact.h
		havok/havokcontactlistener.cc
		havok/havokcontactlistener.h
		havok/havokcharacterbase.cc
		havok/havokcharacterbase.h
		havok/havokcharacterphantom.cc
		havok/havokcharacterphantom.h
		havok/havokcharacterrigidbody.cc
		havok/havokcharacterrigidbody.h
		havok/havokphysicsmesh.cc
		havok/havokphysicsmesh.h
		havok/havokhinge.cc
		havok/havokhinge.h
		havok/havokpointtopoint.cc
		havok/havokpointtopoint.h
		havok/havokslider.cc
		havok/havokslider.h
		havok/havokuniversaljoint.cc
		havok/havokuniversaljoint.h
		havok/havokragdolljoint.cc
		havok/havokragdolljoint.h
		havok/havokchainjoint.cc
		havok/havokchainjoint.h
	)

    SET(SOURCES_files_physics_joints
			joints.h
			base/basejoint.h
			base/basejoint.cc
    		joints/basehinge.h
    		joints/basehinge.cc
			joints/baseuniversaljoint.h   
			joints/baseuniversaljoint.cc  
			joints/baseragdolljoint.h   
			joints/baseragdolljoint.cc  
			joints/basechainjoint.h   
			joints/basechainjoint.cc  
			joints/baseslider.h         
			joints/baseslider.cc 			
			joints/basepointtopoint.h       
			joints/basepointtopoint.cc			
			joints/sliderjoint.h
			joints/sliderjoint.cc
			joints/universaljoint.h
			joints/universaljoint.cc
			joints/ragdolljoint.h
			joints/ragdolljoint.cc
			joints/chainjoint.h         
			joints/chainjoint.cc
			joints/hingejoint.h           
			joints/hingejoint.cc 			
			joints/pointtopointjoint.h       
			joints/pointtopointjoint.cc

	)


    source_group("physics" FILES ${SOURCES_files_physics})
    source_group("physics\\resource" FILES ${SOURCES_files_physics_resource})
    source_group("physics\\bullet" FILES ${SOURCES_files_bullet})
	source_group("physics\\havok" FILES ${SOURCES_files_havok})    
	source_group("physics\\physx" FILES ${SOURCES_files_physics_physx})    
    source_group("physics\\base" FILES ${SOURCES_files_physics_base})
	source_group("physics\\util" FILES ${SOURCES_files_physics_util})
	source_group("physics\\joints" FILES ${SOURCES_files_physics_joints})

    set(SOURCES_physics_core
      ${SOURCES_files_physics}
      ${SOURCES_files_physics_base}
      ${SOURCES_files_physics_resource}
	  ${SOURCES_files_physics_util}
	  ${SOURCES_files_physics_joints}
    )

IF(N_BUILD_BULLET)

	INCLUDE_DIRECTORIES(${Nebula3_SOURCE_DIR}/code/extlibs/
					${Nebula3_SOURCE_DIR}/code/extlibs/bullet/bullet/src
					)
	set(SOURCES_physics
		${SOURCES_physics_core}
		${SOURCES_files_bullet}
	)
ENDIF()

IF(N_BUILD_PHYSX)
	set(SOURCES_physics
		${SOURCES_physics_core}
		${SOURCES_files_physics_physx}
	)
	SET(PHYSX_DIR NOTFOUND CACHE PATH "PhysX SDK Path")
ENDIF()


IF(N_BUILD_HAVOK)					
	set(SOURCES_physics
		${SOURCES_physics_core}
		${SOURCES_files_havok_source}
		${SOURCES_files_havok}
	)

	# need to enable SIMD (2 means disabled, is the default) else there will be some linker error
	ADD_DEFINITIONS(-DHK_CONFIG_SIMD=1)
ENDIF()
	
	set(physics_precompiled_src
	    ../stdneb.cc
		../stdneb.h
	)
	SOURCE_GROUP("Precompiled" FILES ${physics_precompiled_src})


add_library(physics STATIC ${SOURCES_physics} ${physics_precompiled_src})
TARGET_INCLUDE_DIRECTORIES(physics 	PUBLIC ${Nebula3_SOURCE_DIR}/code/render)

TARGET_LINK_LIBRARIES(physics PUBLIC foundation render)
SET_TARGET_PROPERTIES(physics PROPERTIES FOLDER "N3SDK/N3")

ADD_DEPENDENCIES(physics foundation_NIDLS)



IF(N_BUILD_HAVOK)
	
	#FIXME fugly, need a real havok detection
	FIND_PATH(HAVOC_DIR "Havok Limited Use License Agreement for PC XS 03-20-2013.txt"	${Nebula3_SOURCE_DIR}/libs/hk2013_1_0_r1)				
	SET(Havok_SOURCE_DIR ${HAVOC_DIR}/Source)	
	if(MSVC90)
		SET(Havok_LIB_DIR ${HAVOC_DIR}/Lib/win32_net_9-0)
	ELSEIF(MSVC10)
		SET(Havok_LIB_DIR ${HAVOC_DIR}/Lib/win32_vs2010)
	ELSEIF(MSVC11)
		SET(Havok_LIB_DIR ${HAVOC_DIR}/Lib/win32_vs2012_win7)
	ENDIF()

	SET(HAVOK_LIBS hkaAnimation.lib hkaInternal.lib hkaPhysics2012Bridge.lib hkBase.lib hkcdCollide.lib hkcdInternal.lib hkCompat.lib
					hkgBridge.lib hkgCommon.lib hkGeometryUtilities.lib hkInternal.lib hkpCollide.lib hkpConstraint.lib hkpConstraintSolver.lib
					hkpDynamics.lib hkpInternal.lib hkpUtilities.lib hkpVehicle.lib hkSceneData.lib hkSerialize.lib hkVisualize.lib)
	
	SET(HAVOK_DBG_LIBS)	
	FOREACH(CUR_LIB ${HAVOK_LIBS})
		LIST(APPEND HAVOK_DBG_LIBS ${Havok_LIB_DIR}/debug/${CUR_LIB})
	ENDFOREACH()
	
	SET(HAVOK_RELEASE_LIBS)
	FOREACH(CUR_LIB ${HAVOK_LIBS})
		LIST(APPEND HAVOK_RELEASE_LIBS ${Havok_LIB_DIR}/dev/${CUR_LIB})
	ENDFOREACH()
	SET(HAVOK_PUBLIC_LIBS)
	FOREACH(CUR_LIB ${HAVOK_LIBS})
		LIST(APPEND HAVOK_PUBLIC_LIBS ${Havok_LIB_DIR}/release/${CUR_LIB})
	ENDFOREACH()

	TARGET_LINK_LIBRARIES(physics PUBLIC $<$<CONFIG:Debug>:${HAVOK_DBG_LIBS}> $<$<CONFIG:Release>:${HAVOK_PUBLIC_LIBS}> $<$<CONFIG:RelWithDebInfo>:${HAVOK_RELEASE_LIBS}>)

ELSEIF(N_BUILD_BULLET)
	ADD_DEPENDENCIES(physics BulletDynamics BulletCollision LinearMath)
	TARGET_LINK_LIBRARIES(physics PUBLIC $<$<BOOL:${N_BUILD_BULLET}>:BulletDynamics BulletCollision LinearMath> )
ELSEIF(N_BUILD_PHYSX)
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
ENDIF()


TARGET_INCLUDE_DIRECTORIES(physics PUBLIC $<$<BOOL:${N_BUILD_HAVOK}>:${Havok_SOURCE_DIR}>)
TARGET_INCLUDE_DIRECTORIES(physics PUBLIC $<$<BOOL:${N_BUILD_PHYSX}>:${PHYSX_DIR}/include>)
TARGET_LINK_LIBRARIES(physics PUBLIC $<$<CONFIG:Debug>:${PHYSX_DEBUG_LIBS}> $<$<CONFIG:Release>:${PHYSX_RELASE_LIBS}> $<$<CONFIG:RelWithDebInfo>:${PHYSX_CHECKED_LIBS}>)

N_SET_COMPILER_DEFAULTS()
IF(N_USE_PRECOMPILED_HEADERS)
    N_SET_PCH(physics stdneb.h stdneb.cc ../)
ENDIF()
