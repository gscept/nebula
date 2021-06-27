nebula_begin_module(graphicsfeature)
fips_deps(render application dynui)
fips_include_directories(${CODE_ROOT}/foundation)
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
fips_dir(properties)
	nebula_add_nidl(
		graphics.nidl
	)
nebula_end_module()

        