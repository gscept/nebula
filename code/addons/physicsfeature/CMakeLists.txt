nebula_begin_module(physicsfeature)
fips_deps(physics application)
target_precompile_headers(physicsfeature REUSE_FROM application)
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
		nebula_add_nidl(
			physics.nidl
		)
nebula_end_module()
