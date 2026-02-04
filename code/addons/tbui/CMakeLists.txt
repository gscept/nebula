#-------------------------------------------------------------------------------
# tbui module
#-------------------------------------------------------------------------------

nebula_begin_module(tbui)
fips_ide_group(addons)

fips_deps(render turbobadger)
target_precompile_headers(tbui PRIVATE <render/stdneb.h>)
target_include_directories(tbui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
nebula_flatc(SYSTEM addons/tbui/tbconfig.fbs)

fips_files(
    tbuicontext.cc
    tbuicontext.h
    tbuiinputhandler.cc
    tbuiinputhandler.h
    tbuiview.cc
    tbuiview.h
)

compile_gpulang(tbui.gpul)

fips_dir(backend)
    fips_files(
        tbuifontrenderer.cc
        tbuibatch.h
        tbuibitmap.cc
        tbuibitmap.h
        tbuiclipboardinterface.cc
        tbuiclipboardinterface.h
        tbuifileinterface.cc
        tbuifileinterface.h
        tbuirenderer.cc
        tbuirenderer.h
        tbuisysteminterface.cc
        tbuisysteminterface.h
        tbuivertex.h
    )

nebula_end_module()
