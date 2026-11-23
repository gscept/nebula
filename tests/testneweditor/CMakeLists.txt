#-------------------------------------------------------------------------------
# test the new editor's scene document
#-------------------------------------------------------------------------------

nebula_begin_app(testneweditor cmdline)
fips_src(. *.* GROUP test neweditor)

# The document is compiled in here on its own. The editor module is an object library, which brings
# the whole editor in with it, and this test only needs the document: the game database, the json
# reader and writer, and the extension hooks
target_sources(testneweditor PRIVATE ${NROOT}/toolkit/neweditor/model/scenedocument.cc)
target_include_directories(testneweditor PRIVATE ${NROOT}/toolkit)

fips_deps(foundation testbase application memdb)
target_precompile_headers(testneweditor REUSE_FROM foundation)
nebula_end_app()
