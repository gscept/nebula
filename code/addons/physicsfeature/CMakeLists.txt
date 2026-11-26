nebula_begin_module(physicsfeature)
fips_deps(physics application)
if(N_EDITOR)
	# The feature defines its own editor contribution, so it needs the editor's registry
	fips_deps(neweditor)
endif()
target_precompile_headers(physicsfeature PRIVATE <application/stdneb.h>)
fips_ide_group(features)
fips_files(
		physicsfeatureunit.h
		physicsfeatureunit.cc
	)
	fips_dir(managers)
		fips_files(
			physicsmanager.h
			physicsmanager.cc
			charactermanager.h
			charactermanager.cc
		)
	fips_dir(components)
		nebula_idl_compile(
			physicsfeature.json
		)
		IF (USE_DOTNET)
			nebula_idl_generate_cs_target(
				physicsfeature.json
			)

			# The app's managed project compiles the generated structs, not this module. Runtime.Setup only
			# registers native components from the engine assembly and the plugin assembly, so a component in
			# its own assembly never gets an id and GetComponentId<T> asserts. The cache variables are what
			# carry the generated file and the hand written wrapper across the subdirectory, and the app
			# decides whether it wants them at all
			set(PHYSICSFEATURE_CS_FILES ${NIDL_Cs_Files} CACHE INTERNAL "")
			set(PHYSICSFEATURE_CS_DEPS ${NIDL_Cs_Deps} CACHE INTERNAL "")
			set(PHYSICSFEATURE_MANAGED_CS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/scriptapi/Physics.cs CACHE INTERNAL "")
		ENDIF()
	fips_dir(scriptapi)
		fips_files(
			physicsapi.h
		)
	fips_dir(editor)
		fips_files(
			physicseditor.h
			physicsgizmo.h
			physicseditor.cc
			physicsgizmo.cc
		)
fips_dir(.)
nebula_end_module()

IF (USE_DOTNET)
	# Managed code resolves [DllImport("__Internal")] against the main program handle, which requires the
	# entry points in the executable's dynamic symbol table. The app links nsharp with -Wl,--exclude-libs,ALL,
	# and that keeps a NEBULA_EXPORT symbol inside this module's archive out of it, so the script api is an
	# object library whose objects are put into the app directly
	add_library(physicsfeature-scriptapi OBJECT
		scriptapi/physicsapi.cc
	)
	target_link_libraries(physicsfeature-scriptapi PRIVATE foundation physics application physicsfeature)
	target_include_directories(physicsfeature-scriptapi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	target_precompile_headers(physicsfeature-scriptapi REUSE_FROM physics)
	set_target_properties(physicsfeature-scriptapi PROPERTIES CXX_VISIBILITY_PRESET hidden)

	# The layout the entry points write is the one the generator produced from physicsfeature.json, so the
	# generated header has to exist before these objects compile
	add_dependencies(physicsfeature-scriptapi physicsfeature)

	# The app target is in another subdirectory, and a cache variable is what carries this across
	set(PHYSICSFEATURE_SCRIPTAPI_TARGET physicsfeature-scriptapi CACHE INTERNAL "")
ENDIF()
