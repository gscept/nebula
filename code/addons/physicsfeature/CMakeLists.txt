nebula_begin_module(physicsfeature)
fips_deps(physics application graphicsfeature)
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
		)
	fips_dir(components)
		nebula_idl_compile(
			physicsfeature.json
		)
nebula_end_module()
