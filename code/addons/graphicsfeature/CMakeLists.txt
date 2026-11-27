nebula_begin_module(graphicsfeature)
fips_deps(render application dynui nflatbuffer)
if(N_EDITOR)
    # The feature defines its own editor contribution, so it needs the editor's registry, and the
    # asset registry for the asset types it owns
    fips_deps(neweditor editorassets)
endif()
add_addon_hook(tbui)
target_precompile_headers(graphicsfeature PRIVATE <application/stdneb.h>)
fips_ide_group(features)
fips_files(
	graphicsfeatureunit.h
	graphicsfeatureunit.cc
)
fips_dir(terrain)
	fips_files(
		terrainfile.h
		terrainfile.cc
	)
fips_dir(managers)
	fips_files(
		graphicsmanager.h
		graphicsmanager.cc
		cameramanager.h
		cameramanager.cc
	)
fips_dir(components)
	nebula_idl_compile(
		camera.json
		decal.json
		gi.json
		lighting.json
		model.json
        terrain.json
	)
fips_dir(editor)
	fips_files(
		graphicseditor.h
		graphicseditor.cc
		graphicsprofilerpanel.h
		graphicsprofilerpanel.cc
		environmentpanel.h
		environmentpanel.cc
		rendersettings.h
		rendersettings.cc
		resourcespanel.h
		resourcespanel.cc
	)
fips_dir(editor/terrain)
	fips_files(
		terrainsession.h
		terrainsession.cc
		terraineditor.h
		terraineditor.cc
	)
fips_dir(editor/assets)
	fips_files(
		assetpreview.h
		assetpreview.cc
		clipplayback.h
		clipplayback.cc
		materialdocument.h
		materialdocument.cc
		modeldocument.h
		modeldocument.cc
		skeletonview.h
		skeletonview.cc
		texturedocument.h
		texturedocument.cc
	)
if(N_EDITOR)
	# the asset preview's frame script, compiled with the engine's like the others
	fips_dir(editor)
		nebula_framescript_add(preview.json)
endif()
nebula_end_module()

