#-------------------------------------------------------------------------------
# idldocument
#-------------------------------------------------------------------------------
fips_begin_module(idldocument)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    fips_deps(foundation toolkitutil)
        fips_files(stdneb.cc)
        fips_dir(idldocument)
            fips_files(
				idlarg.cc
				idlarg.h
				idlattribute.cc
				idlattribute.h
				idlattributelib.cc
				idlattributelib.h
				idlcodegenerator.cc
				idlcodegenerator.h
				idlcommand.cc
				idlcommand.h
				idldependency.cc
				idldependency.h
				idldocument.cc
				idldocument.h
				idllibrary.cc
				idllibrary.h
				idlmessage.cc
				idlmessage.h
				idlproperty.cc
				idlproperty.h
				idlprotocol.cc
				idlprotocol.h
			)
fips_end_module()

