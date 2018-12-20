fips_begin_lib(tinyxml)
fips_include_directories(${CODE_ROOT}/foundation)
fips_files(tinystr.cc  tinystr.h  tinyxml.cc  tinyxml.h  tinyxmlerror.cc  tinyxmlparser.cc)
fips_end_lib()