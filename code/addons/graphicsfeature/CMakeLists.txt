nebula_begin_module(graphicsfeature)
nebula_flatc(SYSTEM graphicsfeature/graphicsfeatureschema.fbs)
fips_deps(render application dynui staticui nflatbuffer)
target_precompile_headers(graphicsfeature REUSE_FROM application)
fips_ide_group(features)
fips_files(
	graphicsfeatureunit.h
	graphicsfeatureunit.cc
)
fips_dir(managers)
	fips_files(
		graphicsmanager.h
		graphicsmanager.cc
		cameramanager.h
		cameramanager.cc
	)
fips_dir(components)
	nebula_flatc_component(
		graphicsfeature.fbs
	)
nebula_end_module()

