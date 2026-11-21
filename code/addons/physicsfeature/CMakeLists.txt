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
	fips_dir(editor)
		fips_files(
			physicseditor.h
			physicseditor.cc
		)
nebula_end_module()
