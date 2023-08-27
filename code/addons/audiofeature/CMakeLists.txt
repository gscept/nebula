nebula_begin_module(audiofeature)
fips_deps(audio application)
fips_ide_group(features)
target_precompile_headers(audiofeature REUSE_FROM application)
fips_files(
		audiofeatureunit.h
		audiofeatureunit.cc
	)
	fips_dir(managers)
		fips_files(
			audiomanager.h
			audiomanager.cc
		)
	fips_dir(components)
		nebula_add_nidl(
			audio.nidl
		)
nebula_end_module()
