nebula_begin_module(graphicsfeature)
nebula_flatc(SYSTEM graphicsfeature/graphicsfeatureschema.fbs)
fips_deps(render application dynui staticui nflatbuffer)
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
	nebula_add_nidl(
		graphics.nidl
	)
nebula_end_module()

