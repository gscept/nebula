fips_begin_lib(corelibs)
# this is ugly, required as tinyxml is using foundation functions
# tinyxml should probably be integrated fully or replaced with tinyxml2
fips_include_directories(${NROOT}/code/foundation)
fips_files_ex(base64/ *.* GROUP base64)
fips_files_ex(stackwalker/ *.* GROUP stackwalker)
fips_files_ex(tinyxml/ *.* GROUP tinyxml)
fips_files_ex(zlib/ *.* GROUP zlib)
fips_end_lib()
TARGET_INCLUDE_DIRECTORIES(corelibs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})