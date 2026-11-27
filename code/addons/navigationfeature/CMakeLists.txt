nebula_begin_module(navigationfeature)
fips_deps(navbuild Detour DetourCrowd Recast DetourTileCache DebugUtils application render dynui)
if(N_EDITOR)
	# The feature defines its own editor contribution: the navmesh asset, its panel and its fields.
	# Physics and graphics for the sources the scene's colliders and models give a navmesh
	fips_deps(neweditor editorassets physicsfeature graphicsfeature)
endif()
target_include_directories(navigationfeature PRIVATE ${CODE_ROOT}/foundation)
fips_files(
		navigationfeatureunit.h
		navigationfeatureunit.cc
		streamnavmeshcache.h
		streamnavmeshcache.cc
		navquery.h
		navquery.cc
	)
	fips_dir(managers)
		fips_files(
			navigationmanager.h
			navigationmanager.cc
			agentmanager.h
			agentmanager.cc
		)

	fips_dir(components)
		nebula_idl_compile(
			navigation.json
		)
    fips_dir(editor)
        fips_files(
            navigationeditor.h
            navigationeditor.cc
            navigationpanel.h
            navigationpanel.cc
            navmeshdocument.h
            navmeshdocument.cc
            navmeshsources.h
            navmeshsources.cc
        )
    fips_dir(debug)
        fips_files(detourdebug.h
                   detourdebug.cc
                   #navigationpagehandler.h
                   #navigationpagehandler.cc
        )
nebula_end_module()
