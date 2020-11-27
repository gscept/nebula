#-------------------------------------------------------------------------------
# benchmarks
#-------------------------------------------------------------------------------

fips_ide_group(benchmarks)
fips_include_directories(.)
fips_add_subdirectory(benchmarkbase)
fips_add_subdirectory(benchmarkfoundation)
fips_add_subdirectory(benchmarkmemdb)