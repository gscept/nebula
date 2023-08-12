#-------------------------------------------------------------------------------
# Toolkit
#-------------------------------------------------------------------------------

find_package(FBX REQUIRED)

fips_begin_lib(toolkitutil)
    target_compile_definitions(toolkitutil PUBLIC FBXSDK_SHARED)
    target_include_directories(toolkitutil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/render ${CODE_ROOT}/)
    target_include_directories(toolkitutil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${FBX_INCLUDE_DIRS})
    target_precompile_headers(toolkitutil PRIVATE [["foundation/stdneb.h"]] [["render/stdneb.h"]] [["model/import/base/scenenode.h"]])

    fips_deps(foundation render gltf physics)
    fips_libs(${FBX_LIBRARY})
    if(FIPS_WINDOWS)
        fips_libs(CMP_Core_MT CMP_Framework_MT)
    else()
        fips_libs(xml2 CMP_Framework CMP_Common CMP_Core CMP_Core_SSE CMP_Core_AVX512 CMP_Core_AVX CMP_Compressonator CMP_GpuDecode Image_DDS Image_EXR Image_KTX Image_TGA)
    endif()
        fips_files(
            assetfile.cc
            assetfile.h
            assetregistry.cc
            assetregistry.h
            assetupdater.cc
            assetupdater.h
            audioexporter.cc
            audioexporter.h

            #resourcedictionarybuilder.cc
            #resourcedictionarybuilder.h
        )

        fips_dir(asset)
            fips_files(
                assetexporter.cc
                assetexporter.h
            )
        fips_dir(model/animutil)
            fips_files(
                animbuilder.cc
                animbuilder.h
                animbuilderclip.cc
                animbuilderclip.h
                animbuildercurve.cc
                animbuildercurve.h
                animbuildersaver.cc
                animbuildersaver.h
            )
        fips_dir(model/meshutil)
            fips_files(
                meshbuilder.cc
                meshbuilder.h
                meshbuildergroup.cc
                meshbuildergroup.h
                meshbuildersaver.cc
                meshbuildersaver.h
                meshbuildertriangle.cc
                meshbuildertriangle.h
                meshbuildervertex.cc
                meshbuildervertex.h
            )
        fips_dir(model/n3util)
            fips_files(
                n3modeldata.h
            )
        fips_dir(model/skeletonutil)
            fips_files(
                skeletonbuilder.h
                skeletonbuildersaver.cc
                skeletonbuildersaver.h
            )
    IF(NOT N_MINIMAL_TOOLKIT)
        #fips_dir(posteffect)
        #   fips_files(
        #       posteffectexporter.cc
        #       posteffectexporter.h
        #       posteffectparser.cc
        #       posteffectparser.h
        #   )
    ENDIF()
    IF(FIPS_WINDOWS)
        SET(SRCSX_PL        
        directxtexconversionjob.cc
        directxtexconversionjob.h
        compressonatorconversionjob.cc
        compressonatorconversionjob.h
        )
    ELSE()
    SET(SRCSX_PL        
        compressonatorconversionjob.cc
        compressonatorconversionjob.h
        )
    ENDIF()
    IF(NOT N_MINIMAL_TOOLKIT)
        fips_dir(texutil)
            fips_files(
                # cubefilterer.cc
                # cubefilterer.h
                # imageconverter.cc
                # imageconverter.h
                textureattrs.cc
                textureattrs.h
                textureattrtable.cc
                textureattrtable.h
                textureconversionjob.cc
                textureconversionjob.h
                textureconverter.cc
                textureconverter.h
                ${SRCSX_PL}
            )
    ENDIF()     
        fips_dir(model/import/base)
            fips_files(
                modelexporter.cc
                modelexporter.h 
                scene.cc
                scene.h
                scenenode.cc
                scenenode.h
                uniquestring.cc
                uniquestring.h
            )

        fips_dir(model/import/fbx)
            fips_files(
                nfbxexporter.cc 
                nfbxexporter.h
            )

            fips_dir(model/import/fbx/node)
                fips_files(
                    nfbxjointnode.cc
                    nfbxjointnode.h
                    nfbxlightnode.cc
                    nfbxlightnode.h
                    nfbxmeshnode.cc
                    nfbxmeshnode.h
                    nfbxnode.cc
                    nfbxnode.h
                    nfbxscene.cc
                    nfbxscene.h
                )

        fips_dir(model/import/gltf)
            fips_files(
                ngltfexporter.cc
                ngltfexporter.h
                ngltfmaterialexporter.cc
                ngltfmaterialexporter.h
            )
            fips_dir(model/import/gltf/node)
                fips_files(
                    ngltfmesh.cc
                    ngltfmesh.h
                    ngltfnode.cc
                    ngltfnode.h
                    ngltfscene.cc
                    ngltfscene.h
                    meshprimitive.cc
                    meshprimitive.h
                )
IF(NOT N_MINIMAL_TOOLKIT)
        #fips_dir(game)
        #   fips_files(
        #       gameexporter.cc
        #       gameexporter.h
        #       leveldbwriter.cc
        #       leveldbwriter.h
        #       levelparser.cc
        #       levelparser.h
        #   )

        #fips_dir(gui)
        #   fips_files(
        #       guiexporter.cc
        #       guiexporter.h
        #   )
ENDIF()         
        fips_dir(model/modelutil)
            fips_files(
                clip.cc
                clip.h
                clipevent.cc
                clipevent.h
                modelattributes.cc
                modelattributes.h
                modelbuilder.cc
                modelbuilder.h
                modelconstants.cc
                modelconstants.h
                modeldatabase.cc
                modeldatabase.h
                modelphysics.cc
                modelphysics.h
                take.cc
                take.h
            )
        fips_dir(model)
            fips_files(
                animsplitterhelper.cc
                animsplitterhelper.h
                batchattributes.cc
                batchattributes.h
                binarymodelwriter.cc
                binarymodelwriter.h
                modelwriter.cc
                modelwriter.h
                scenewriter.cc
                scenewriter.h
                skinhelper.cc
                skinhelper.h
                xmlmodelwriter.cc
                xmlmodelwriter.h
            )
        fips_dir(surface)
            fips_files(
                surfaceexporter.cc
                surfaceexporter.h
                surfacebuilder.h
                surfacebuilder.cc
            )
fips_end_lib()

IF(FIPS_WINDOWS)
    FILE(COPY ${FBX_DLL} DESTINATION ${FIPS_DEPLOY_DIR}/${CMAKE_PROJECT_NAME}/${FIPS_CONFIG}/)
ENDIF()
