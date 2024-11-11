fips_ide_group(nebula)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.27")
cmake_policy(SET CMP0147 NEW)
endif()
cmake_policy(SET CMP0079 NEW)

SET(NROOT ${CMAKE_CURRENT_LIST_DIR}/..)
SET(CODE_ROOT ${CMAKE_CURRENT_LIST_DIR}/code)

set (CMAKE_MODULE_PATH "${NROOT}/extlibs/scripts")

option(N_USE_PRECOMPILED_HEADERS "Use precompiled headers" OFF)
option(N_ENABLE_SHADER_COMMAND_GENERATION "Generate shader compile file for live shader reload" ON)
option(N_EDITOR "Build as an editor build" ON)
option(N_USE_CHECKED_PHYSX "Use Checked PhysX in optimized builds" ON)
option(N_USE_COMPILETIME_PROJECT_ROOT "Embed the selected work directory into binary for development builds" ON)

if(N_USE_COMPILETIME_PROJECT_ROOT)
    if(EXISTS "${CMAKE_BINARY_DIR}/project_root.txt")
        FILE(READ "${CMAKE_BINARY_DIR}/project_root.txt" PRJ_ROOT)
        add_definitions(-DNEBULA_PROJECT_ROOT=\"${PRJ_ROOT}\")
    endif()
endif()

include(create_resource)

if(FIPS_WINDOWS)
    option(N_STATIC_BUILD "Use static runtime in windows builds" ON)
    if(N_STATIC_BUILD)
         add_definitions(-D__N_STATIC_BUILD)
    endif()
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX /fp:fast /GS- /wd4324 /wd4100 /wd4996 /wd4458 /wd4201 /wd4505 /wd4244 /wd4018 /permissive- /Zc:rvalueCast /W3")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX /fp:fast /GS-")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /RTC1 /RTCc /JMC")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /RTC1 /JMC")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Qpar")
    if (N_DEBUG_SYMBOLS)
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG")
    endif()
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG:full")
    add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
elseif(FIPS_LINUX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -msse4.2 -march=sandybridge -ffast-math -fPIC -fno-trapping-math -funsafe-math-optimizations -ffinite-math-only -mrecip=all -Wall")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2 -march=sandybridge -ffast-math -fPIC -fno-trapping-math -funsafe-math-optimizations -ffinite-math-only -mrecip=all -Wall")
    set(CXX_WARNING_FLAGS "-Wno-sign-compare -Wno-unused-parameter -Wno-deprecated-copy -Wno-deprecated-volatile -Wno-unused-function -Wno-unknown-pragmas -Wno-ignored-pragmas -Wno-missing-braces -Wno-overloaded-virtual -Wno-unused-variable -Wno-tautological-constant-out-of-range-compare -w")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -mavx ${CXX_WARNING_FLAGS} ")

    
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(FIPS_WINDOWS_LTCG OFF)

OPTION(N_USE_AVX "Use AVX instructionset" ON)
OPTION(N_USE_FMA "Use FMA instructionset" OFF)
OPTION(N_USE_CURL "Use libcurl for httpclient" ON)

if (N_USE_CURL)
    add_definitions(-DUSE_CURL)
endif()

if (N_USE_AVX)
    add_definitions(-DN_USE_AVX)
endif()
if (N_USE_FMA)
    add_definitions(-DN_USE_FMA)
endif()
add_definitions(-DIL_STATIC_LIB=1)

find_package(Python 3.7 COMPONENTS Development REQUIRED)
# MESSAGE(WARNING "Python Debug: ${Python_LIBRARY_DEBUG} Release: ${Python_LIBRARY_RELEASE} ${Python_INCLUDE_DIRS}")

#physx

SET(PX_LIBRARY_NAMES PhysX PhysXCommon PhysXCooking  PhysXFoundation)
SET(PX_LIBRARY_STATIC_NAMES PhysXCharacterKinematic PhysXExtensions PhysXPvdSDK  PhysXVehicle PhysXVehicle2)
SET(PX_LINK_PREFIX _static_64)

macro(find_physx_libraries build_dir target_variable)
    SET(PX_DIR_TYPE ${FIPS_DEPLOY_DIR}/physx/bin/${PX_TARGET}/${build_dir}/)
    foreach(CUR_LIB ${PX_LIBRARY_NAMES})
        find_library(PX_${build_dir}_${CUR_LIB} ${CUR_LIB}${PX_LINK_PREFIX} PATHS ${PX_DIR_TYPE})
        LIST(APPEND ${target_variable} ${PX_${build_dir}_${CUR_LIB}})
    endforeach()

    foreach(CUR_LIB ${PX_LIBRARY_STATIC_NAMES})
        find_library(PX_${build_dir}_${CUR_LIB} ${CUR_LIB}_static_64 PATHS ${PX_DIR_TYPE})
        LIST(APPEND ${target_variable} ${PX_${build_dir}_${CUR_LIB}})
    endforeach()
endmacro()

if(FIPS_WINDOWS)
    
    if(NOT PX_TARGET)
        MESSAGE(FATAL_ERROR "PX_TARGET undefined, select a physx build folder (eg. win.x86_64.vs143)")
    endif()

    SET(PX_DEBUG_LIBS)
    SET(PX_RELEASE_LIBS)
    find_physx_libraries("debug" PX_DEBUG_LIBS)
    if (N_USE_CHECKED_PHYSX)
        find_physx_libraries("checked" PX_RELEASE_LIBS)
    else()
        find_physx_libraries("release" PX_RELEASE_LIBS)
    endif()
    add_library(PxLibs INTERFACE)
    target_link_libraries(PxLibs INTERFACE $<$<CONFIG:Debug>:${PX_DEBUG_LIBS}> $<$<CONFIG:Release>:${PX_RELEASE_LIBS}>)
else()
    SET(PX_DIR_LINUX ${FIPS_DEPLOY_DIR}/physx/bin/linux.clang/checked/)
    SET(PX_LIBRARY_NAMES PhysX PhysXCooking PhysXCharacterKinematic PhysXExtensions PhysXPvdSDK PhysXVehicle  PhysXCommon PhysXFoundation)
    SET(PX_POST _static_64.a)
    SET(PX_LIBRARIES)
    foreach(CUR_LIB ${PX_LIBRARY_NAMES})
        find_library(PX_${CUR_LIB} lib${CUR_LIB}${PX_POST} PATHS ${PX_DIR_LINUX})
        LIST(APPEND PX_LIBRARIES ${PX_${CUR_LIB}})
    endforeach()

    add_library(PxLibs INTERFACE)
    target_link_libraries(PxLibs INTERFACE ${PX_LIBRARIES})
    MESSAGE(STATUS "Found PhysX: ${PX_LIBRARIES}")
endif()

target_include_directories(PxLibs INTERFACE ${FIPS_DEPLOY_DIR}/physx/include)

set(DEF_RENDERER "N_RENDERER_VULKAN")
set(N_RENDERER ${DEF_RENDERER} CACHE STRING "Nebula 3D Render Device")
set_property(CACHE N_RENDERER PROPERTY STRINGS "N_RENDERER_VULKAN" )
set(${N_RENDERER} ON)

if(N_RENDERER_VULKAN)
    OPTION(N_VALIDATION "Use Vulkan validation" OFF)
    if (N_VALIDATION)
        add_definitions(-DNEBULAT_VULKAN_VALIDATION)
    endif()
    add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="vkdebug")
    add_definitions(-D__VULKAN__)
    add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=Vulkan)
endif()

option(N_NEBULA_DEBUG_SHADERS "Compile shaders with debug flag" OFF)

option(USE_DOTNET "Build with .NET support" OFF)

IF (USE_DOTNET)
    cmake_policy(PUSH)
    # Ignore policy that disallows environment variables
    cmake_policy(SET CMP0074 NEW)
    find_package(Dotnet REQUIRED)
    cmake_policy(POP)
ENDIF (USE_DOTNET)

macro(nebula_flatc root)
    string(COMPARE EQUAL ${root} "SYSTEM" use_system)
    set_nebula_export_dir()

    if(${use_system})
        set(rootdir ${NROOT}/syswork)
    else()
        set(rootdir ${WORK_DIR}/work)
    endif()

    foreach(fb ${ARGN})
        set(target_has_flatc 1)
        set(datadir ${rootdir}/data/flatbuffer/)
        get_filename_component(filename ${fb} NAME)
        get_filename_component(foldername ${fb} DIRECTORY)
        string(REPLACE ".fbs" ".h" out_header ${filename})
       
        set(abs_output_folder "${CMAKE_BINARY_DIR}/generated/flat/${foldername}")
        set(fbs ${datadir}${fb})
        set(output ${abs_output_folder}/${out_header})
        add_custom_command(OUTPUT ${output}
                PRE_BUILD COMMAND ${FLATC} -c --gen-object-api --gen-mutable --include-prefix flat --keep-prefix --cpp-str-flex-ctor --cpp-str-type Util::String -I "${datadir}" -I "${NROOT}/syswork/data/flatbuffer/" --filename-suffix "" -o "${abs_output_folder}" "${fbs}"
                PRE_BUILD COMMAND ${FLATC} -b -o "${EXPORT_DIR}/data/flatbuffer/${foldername}/" -I "${datadir}" -I "${NROOT}/syswork/data/flatbuffer/" --schema ${fbs}
                MAIN_DEPENDENCY "${fbs}"
                DEPENDS ${FLATC}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT "Compiling ${fb} flatbuffer"
                VERBATIM
                )
        target_sources(${CurTargetName} PRIVATE ${fbs})
        target_sources(${CurTargetName} PRIVATE ${output})

        SOURCE_GROUP("${CurGroup}\\Generated" FILES "${output}")
        source_group("res\\flatbuffer" FILES ${fbs})
    endforeach()
endmacro()

macro(compile_gpulang_intern)
    set(shd ${ARGV0})
    set(nebula_shader ${ARGV1})
    if(SHADERC)
        if(N_NEBULA_DEBUG_SHADERS)
            set(shader_debug "-debug")
        endif()

        if (nebula_shader)
            set(foldername system_shaders/${CurDir})
            set(base_path ${NROOT}/syswork/shaders/vk)
        else()
            set(foldername ${CurDir})
            set(base_path ${CMAKE_CURRENT_SOURCE_DIR}/${CurDir})
        endif()

        cmake_path(SET shd_path ${shd})

        cmake_path(RELATIVE_PATH shd_path BASE_DIRECTORY ${base_path} OUTPUT_VARIABLE rel_path)
        cmake_path(GET rel_path STEM basename)

        set(binaryOutput ${EXPORT_DIR}/shaders/gpulang/${foldername}${basename}.gplb)
        set(headerOutput ${CMAKE_BINARY_DIR}/shaders/gpulang/${CurTargetName}/${foldername}${basename}.h)

        # first calculate dependencies
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${foldername})
        set(depoutput ${CMAKE_BINARY_DIR}/${foldername}${basename}.dep)
        # create it the first time by force, after that with dependencies
        # since custom command does not want to play ball atm, we just generate it every time
        if(NOT EXISTS ${depoutput} OR ${shd} IS_NEWER_THAN ${depoutput})
            execute_process(COMMAND ${GPULANGC} -M -i ${shd} -I ${NROOT}/syswork/shaders/gpulang -I ${foldername} -I ${CMAKE_BINARY_DIR}/material_templates/render/materials -o ${depoutput} -h ${headerOutput}.h -t shader)
        endif()

        # sadly this doesnt work for some reason
        #add_custom_command(OUTPUT ${depoutput}
        #COMMAND ${SHADERC} -M -i ${shd} -I ${NROOT}/work/shaders -I ${foldername} -o ${CMAKE_BINARY_DIR} -t shader
        #DEPENDS ${SHADERC} ${shd}
        #WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
        #COMMENT ""
        #VERBATIM
        #)
        if(EXISTS ${depoutput})
            file(READ ${depoutput} deps)
        endif()

        add_custom_command(OUTPUT ${binaryOutput}
            COMMAND ${GPULANGC} -i ${shd} -I ${NROOT}/syswork/shaders/gpulang -I ${foldername} -I ${CMAKE_BINARY_DIR}/material_templates/render/materials -o ${binaryOutput} -h ${headerOutput} -t shader ${shader_debug}
            MAIN_DEPENDENCY ${shd}
            DEPENDS ${GPULANGC} ${deps}
            WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
            COMMENT ""
            VERBATIM
            )

        SET(tmp_dir ${CurDir})
        SET(CurDir "")
        fips_files(${shd})

        SET(CurDir ${tmp_dir})

        if(N_ENABLE_SHADER_COMMAND_GENERATION)
            # create compile flags file for live shader compile
            file(WRITE ${FIPS_PROJECT_DEPLOY_DIR}/shaders/${basename}.txt "${GPULANGC} -i ${shd} -I ${NROOT}/syswork/shaders/gpulang -I ${foldername} -o ${binaryOutput} -h ${headerOutput} -t shader ${shader_debug}")
        endif()
    endif()
endmacro()

macro(add_shader_intern)
    set(shd ${ARGV0})
    set(nebula_shader ${ARGV1})
    if(SHADERC)
        if(N_NEBULA_DEBUG_SHADERS)
            set(shader_debug "-debug")
        endif()

        if (nebula_shader)
            set(foldername system_shaders/${CurDir})
            set(base_path ${NROOT}/syswork/shaders/vk)
        else()
            set(foldername ${CurDir})
            set(base_path ${CMAKE_CURRENT_SOURCE_DIR}/${CurDir})
        endif()

        cmake_path(SET shd_path ${shd})

        cmake_path(RELATIVE_PATH shd_path BASE_DIRECTORY ${base_path} OUTPUT_VARIABLE rel_path)
        cmake_path(GET rel_path STEM basename)

        set(binaryOutput ${EXPORT_DIR}/shaders/${foldername}${basename}.fxb)
        set(headerOutput ${CMAKE_BINARY_DIR}/shaders/${CurTargetName}/${foldername}${basename}.h)


        # first calculate dependencies
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${foldername})
        set(depoutput ${CMAKE_BINARY_DIR}/${foldername}${basename}.dep)
        # create it the first time by force, after that with dependencies
        # since custom command does not want to play ball atm, we just generate it every time
        if(NOT EXISTS ${depoutput} OR ${shd} IS_NEWER_THAN ${depoutput})
            execute_process(COMMAND ${SHADERC} -M -i ${shd} -I ${NROOT}/syswork/shaders/vk -I  ${foldername} -I ${CMAKE_BINARY_DIR}/material_templates/render/materials -o ${depoutput} -h ${headerOutput}.h -t shader)
        endif()

        # sadly this doesnt work for some reason
        #add_custom_command(OUTPUT ${depoutput}
        #COMMAND ${SHADERC} -M -i ${shd} -I ${NROOT}/work/shaders -I ${foldername} -o ${CMAKE_BINARY_DIR} -t shader
        #DEPENDS ${SHADERC} ${shd}
        #WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
        #COMMENT ""
        #VERBATIM
        #)
        if(EXISTS ${depoutput})
            file(READ ${depoutput} deps)
        endif()

        add_custom_command(OUTPUT ${binaryOutput}
            COMMAND ${SHADERC} -i ${shd} -I ${NROOT}/syswork/shaders/vk -I ${foldername} -I ${CMAKE_BINARY_DIR}/material_templates/render/materials -o ${binaryOutput} -h ${headerOutput} -t shader ${shader_debug}
            MAIN_DEPENDENCY ${shd}
            DEPENDS ${SHADERC} ${deps}
            WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
            COMMENT ""
            VERBATIM
            )

        SET(tmp_dir ${CurDir})
        SET(CurDir "")
        fips_files(${shd})

        SET(CurDir ${tmp_dir})

        if(N_ENABLE_SHADER_COMMAND_GENERATION)
            # create compile flags file for live shader compile
            file(WRITE ${FIPS_PROJECT_DEPLOY_DIR}/shaders/${basename}.txt "${SHADERC} -i ${shd} -I ${NROOT}/syswork/shaders/vk -I ${CMAKE_BINARY_DIR}/${foldername} -I ${CMAKE_BINARY_DIR}/material_templates/render/materials -o ${binaryOutput} -h ${headerOutput} -t shader ${shader_debug}")
        endif()
    endif()
endmacro()

macro(nebula_add_script_library target)
    fips_deps(nsharp ${target})
    IF(WIN32)
        set_target_properties(${CurTargetName} PROPERTIES VS_GLOBAL_ResolveNuGetPackages "false")
    ENDIF(WIN32)
endmacro()

macro(nebula_idl_compile)
    SOURCE_GROUP("NIDL Files" FILES ${ARGN})
    set(target_has_nidl 1)
    foreach(nidl ${ARGN})
        get_filename_component(f_abs ${CurDir}${nidl} ABSOLUTE)
        get_filename_component(f_dir ${f_abs} PATH)
        STRING(REPLACE ".json" ".cc" out_source ${nidl})
        STRING(REPLACE ".json" ".h" out_header ${nidl})
        STRING(FIND "${CMAKE_CURRENT_SOURCE_DIR}"  "/" last REVERSE)
        STRING(SUBSTRING "${CMAKE_CURRENT_SOURCE_DIR}" ${last}+1 -1 folder)
        set(abs_output_folder "${CMAKE_BINARY_DIR}/nidl/${CurTargetName}/${CurDir}")
        add_custom_command(OUTPUT "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_header}"
            PRE_BUILD COMMAND ${PYTHON} ${NROOT}/fips-files/generators/NIDL.py "${f_abs}" "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_header}"
            WORKING_DIRECTORY "${NROOT}"
            MAIN_DEPENDENCY "${f_abs}"
            DEPENDS ${NROOT}/fips-files/generators/NIDL.py
            VERBATIM PRE_BUILD)
        SOURCE_GROUP("${CurGroup}\\Generated" FILES "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_header}" )
        source_group("${CurGroup}" FILES ${f_abs})
        target_sources(${CurTargetName} PRIVATE "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_header}")
        target_sources(${CurTargetName} PRIVATE "${f_abs}")
    endforeach()
    include_directories("${CMAKE_BINARY_DIR}/nidl/${CurTargetName}")
endmacro()

macro(nebula_material_template_add)
    source_group("Material Template Files" FILES ${ARGN})
    set(target_has_materials 1)
    foreach(temp ${ARGN})
        get_filename_component(f_abs ${CurDir}${temp} ABSOLUTE)
        list(APPEND material_definition_files ${f_abs})
        source_group("${CurGroup}\\Templates" FILES ${f_abs})
        target_sources(${CurTargetName} PRIVATE "${f_abs}")
    endforeach()
endmacro()

macro(nebula_project_material_template_add)
    file(GLOB project_materials ${WORK_DIR}/code/materials/*.json)
    source_group("Material Template Files" FILES ${project_materials})
    set(target_has_materials 1)
    foreach(temp ${project_materials})
        get_filename_component(f_abs ${temp} NAME)
        list(APPEND material_definition_files ${temp})
        source_group("${CurGroup}\\Templates" FILES ${temp})
        target_sources(${CurTargetName} PRIVATE "${temp}")
    endforeach()
endmacro()

macro(nebula_material_template_compile)
    set(out_header "materialtemplates.h")
    set(out_source "materialtemplates.cc")
    set(out_shader "material_interfaces.fx")
    set(out_shader_header "material_interfaces.h")

    set(abs_output_folder "${CMAKE_BINARY_DIR}/material_templates/render/materials")
    file(MAKE_DIRECTORY ${abs_output_folder})
    add_custom_target(materialtemplates
        COMMAND ${PYTHON} ${NROOT}/fips-files/generators/materialtemplatec.py ${material_definition_files} ${SHADERC} ${NROOT}/syswork/shaders/vk "${abs_output_folder}"
        BYPRODUCTS "${abs_output_folder}/${out_header}" "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_shader}" "${abs_output_folder}/${out_shader_header}"
        WORKING_DIRECTORY "${NROOT}"
        SOURCES ${material_definition_files}
        DEPENDS ${NROOT}/fips-files/generators/materialtemplatec.py ${material_definition_files}
        VERBATIM)
    add_dependencies(render materialtemplates)
    set_target_properties(materialtemplates PROPERTIES FOLDER "Material Definitions")
    source_group("materials\\Generated" FILES "${abs_output_folder}/${out_header}" "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_shader}" )
    source_group("materials\\Templates" FILES "${out_header}" "${out_source}" "${out_shader}")
    target_sources(render PRIVATE "${abs_output_folder}/${out_header}" "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_shader}")

    target_include_directories(render PUBLIC "${CMAKE_BINARY_DIR}/material_templates/render")
    add_dependencies(render materialtemplates)
endmacro()

macro(nebula_framescript_add)
    source_group("Frame Script Files" FILES ${ARGN})
    set(target_has_frame_script 1)
    foreach(temp ${ARGN})
        get_filename_component(f_abs ${CurDir}${temp} ABSOLUTE)
        list(APPEND frame_script_definition_files ${f_abs})
        source_group("${CurGroup}\\FrameScripts" FILES ${f_abs})
        target_sources(${CurTargetName} PRIVATE "${f_abs}")
    endforeach()
endmacro()

macro(nebula_framescript_compile)
    set(abs_output_folder "${CMAKE_BINARY_DIR}/framescripts/render/frame")
    file(MAKE_DIRECTORY ${abs_output_folder})

    foreach(script ${frame_script_definition_files})
        get_filename_component(script_name ${script} NAME_WE)
        set(out_header ${script_name}.h)
        set(out_source ${script_name}.cc)
        set(target_name framescript_${script_name})
        add_custom_target(${target_name}
            COMMAND ${PYTHON} ${NROOT}/fips-files/generators/framescriptc.py ${script} "${abs_output_folder}/${out_header}" "${abs_output_folder}/${out_source}"
            BYPRODUCTS "${abs_output_folder}/${out_header}" "${abs_output_folder}/${out_source}"
            WORKING_DIRECTORY "${NROOT}"
            SOURCES ${script}
            DEPENDS ${NROOT}/fips-files/generators/framescriptc.py ${script}
            VERBATIM)
        add_dependencies(render ${target_name})
        set_target_properties(${target_name} PROPERTIES FOLDER "Frame Scripts")
        source_group("framescripts\\Generated" FILES "${abs_output_folder}/${out_header}" "${abs_output_folder}/${out_source}" )
        source_group("framescripts\\JSON" FILES "${script}")
        target_sources(render PRIVATE "${abs_output_folder}/${out_header}" "${abs_output_folder}/${out_source}")

    endforeach()
    target_include_directories(render PUBLIC "${CMAKE_BINARY_DIR}/framescripts/render")
endmacro()

# Call inside fips_sharedlib, after calling nebula_idl_generate_cs_target
macro(nebula_register_nidl_cs)
    add_dependencies(${CurTargetName} ${NIDL_Cs_Deps})
        
    if(WIN32)
        foreach(ncsfile IN LISTS NIDL_Cs_Files)
            # HACK:
            # cmake doesn't group these when using source_group for some reason    
            # override the csproj Link tag with custom link path.
            # for some reason, the link gets set twice in the file. Doesn't seem to matter though...
            get_filename_component(fileName ${ncsfile} NAME)
            set_source_files_properties(
                "${ncsfile}"
                PROPERTIES VS_CSHARP_Link "NativeComponents\\${fileName}"
            )
        endforeach()
    endif(WIN32)

    target_sources(${CurTargetName} PRIVATE ${NIDL_Cs_Files})
endmacro()

macro(nebula_idl_generate_cs_target)
    set(NIDL_Cs_Deps "")
    set(NIDL_Cs_Files "")
    foreach(nidl ${ARGN})
        get_filename_component(f_abs ${CurDir}${nidl} ABSOLUTE)
        get_filename_component(f_dir ${f_abs} PATH)
        STRING(FIND "${CMAKE_CURRENT_SOURCE_DIR}"  "/" last REVERSE)
        STRING(SUBSTRING "${CMAKE_CURRENT_SOURCE_DIR}" ${last}+1 -1 folder)
        
        STRING(REPLACE ".json" "" nidlName ${nidl})
        get_filename_component(nidlName ${nidlName} NAME)
        
        set(abs_output_folder "${CMAKE_BINARY_DIR}/nidl/cs/${nidlName}/")
        get_filename_component(nidl ${nidl} NAME)
        STRING(REPLACE ".json" ".cs" out_cs ${nidl})
        
        if (NOT EXISTS "${abs_output_folder}")
            file(MAKE_DIRECTORY ${abs_output_folder})
        endif()
        if (NOT EXISTS "${abs_output_folder}${out_cs}")
            file(TOUCH "${abs_output_folder}${out_cs}")
        endif()
        
        add_custom_target(GenerateNIDL_${nidlName}
            COMMAND ${PYTHON} ${NROOT}/fips-files/generators/NIDL.py "${f_abs}" "${abs_output_folder}/${out_cs}" "--csharp"
            BYPRODUCTS "${abs_output_folder}${out_cs}"
            COMMENT "IDLC: Generating ${out_cs}..."
            DEPENDS ${NROOT}/fips-files/generators/NIDL.py
            WORKING_DIRECTORY "${NROOT}"
            VERBATIM
            SOURCES "${f_abs}"
        )
        set_target_properties(GenerateNIDL_${nidlName} PROPERTIES FOLDER "NIDL")
        set_target_properties(GenerateNIDL_${nidlName} PROPERTIES VS_GLOBAL_ResolveNuGetPackages "false")
        list(APPEND NIDL_Cs_Files "${abs_output_folder}${out_cs}")
        list(APPEND NIDL_Cs_Deps GenerateNIDL_${nidlName})
    endforeach()
endmacro()

macro(add_frameshader_intern)
    foreach(frm ${ARGN})
            get_filename_component(basename ${frm} NAME)
            set(output ${EXPORT_DIR}/frame/${basename})
            add_custom_command(OUTPUT ${output}
                COMMAND ${CMAKE_COMMAND} -E copy ${frm} ${EXPORT_DIR}/frame/
                MAIN_DEPENDENCY ${frm}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT "Copying Frameshader ${frm} to ${EXPORT_DIR}/frame"
                VERBATIM
                )
            fips_files(${frm})
            SOURCE_GROUP("res\\frameshaders" FILES ${frm})
        endforeach()
endmacro()

macro(add_frameshader)
    set_nebula_export_dir()
    foreach(frm ${ARGN})
        add_frameshader_intern(${CMAKE_CURRENT_SOURCE_DIR}/${frm})
    endforeach()
endmacro()

macro(add_material_intern)
    set_nebula_export_dir()
    foreach(mat ${ARGN})
        get_filename_component(basename ${mat} NAME)
        set(output ${EXPORT_DIR}/materials/${basename})
        add_custom_command(OUTPUT ${output}
            COMMAND ${CMAKE_COMMAND} -E copy ${mat} ${EXPORT_DIR}/materials/
            MAIN_DEPENDENCY ${mat}
            WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
            COMMENT "Copying material ${mat} to ${EXPORT_DIR}/materials"
            VERBATIM
        )
        fips_files(${mat})
        SOURCE_GROUP("res\\materials" FILES ${mat})
    endforeach()
endmacro()

macro(add_material)
    foreach(mat ${ARGN})
        add_material_intern(${CMAKE_CURRENT_SOURCE_DIR}/${mat})
    endforeach()
endmacro()

macro(add_blueprint_intern)
    foreach(bp ${ARGN})
            get_filename_component(basename ${bp} NAME)
            set(output ${EXPORT_DIR}/data/tables/${basename})
            add_custom_command(OUTPUT ${output}
                COMMAND ${CMAKE_COMMAND} -E copy ${bp} ${EXPORT_DIR}/data/tables/
                MAIN_DEPENDENCY ${bp}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT "Copying blueprint ${bp} to ${EXPORT_DIR}/data/tables"
                VERBATIM
                )
            fips_files(${bp})
            SOURCE_GROUP("res\\blueprints" FILES ${bp})
        endforeach()
endmacro()

macro(add_blueprint)
    set_nebula_export_dir()
    foreach(bp ${ARGN})
        add_blueprint_intern(${CMAKE_CURRENT_SOURCE_DIR}/${bp})
    endforeach()
endmacro()

# minimal resolving for toolkit and proj assigns used in projectinfo
function(resolve_assigns str resolved projdir)
    string(REPLACE "toolkit:" ${NROOT}/ out ${str})
    string(REPLACE "proj:" ${projdir}/ out2 ${out})
    set (${resolved} ${out2} PARENT_SCOPE)
endfunction()

macro(add_template_intern)
    foreach(tp ${ARGN})
        get_filename_component(basename ${tp} NAME)
        get_filename_component(dir ${tp} DIRECTORY)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/data/tables/templates/" "" dir ${dir})
        set(output ${EXPORT_DIR}/data/tables/templates/${dir}/${basename})
        add_custom_command(OUTPUT ${output}
            COMMAND ${CMAKE_COMMAND} -E copy ${tp} ${EXPORT_DIR}/data/tables/templates/${dir}/
            MAIN_DEPENDENCY ${tp}
            WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
            COMMENT "Copying template ${tp} to ${EXPORT_DIR}/data/tables/templates"
            VERBATIM
            )
        fips_files(${tp})
        SOURCE_GROUP("res\\templates\\${dir}" FILES ${tp})
    endforeach()
endmacro()

macro(add_template_dir)
    set_nebula_export_dir()
    foreach(tpdir ${ARGN})
        file(GLOB_RECURSE templates "${tpdir}/*.json")
        foreach (tp ${templates})
            add_template_intern(${tp})
        endforeach()
    endforeach()
endmacro()

include(JSONParser)

macro(set_nebula_export_dir)
    if(FIPS_WINDOWS)
        get_filename_component(workdir "[HKEY_CURRENT_USER\\SOFTWARE\\gscept\\ToolkitShared;workdir]" ABSOLUTE)
        # get_filename_component returns /registry when a key is not found...
        if(${workdir} STREQUAL "/registry")
            MESSAGE(WARNING "Registry keys for project not found, did you set your workdir?")
            return()
        endif()
        if(EXISTS ${workdir}/projectinfo.json)
            FILE(READ "${workdir}/projectinfo.json" projectJson)
                sbeParseJson(projectInfo projectJson)
                resolve_assigns(${projectInfo.DestDir} targetdir ${workdir})
            SET(EXPORT_DIR ${targetdir})
            set(WORK_DIR "${workdir}")
        else()
            MESSAGE(WARNING "No projectinfo found in project folder, setting default export")
            set(EXPORT_DIR "${workdir}/export")
            set(WORK_DIR "${workdir}")
        endif()
        
    else()
        if(EXISTS $ENV{HOME}/.config/nebula/gscept.cfg)
            FILE(READ "$ENV{HOME}/.config/nebula/gscept.cfg" SettingsJson)
            sbeParseJson(Settings SettingsJson)
            set(workdir ${Settings.ToolkitShared.workdir})

             if(EXISTS ${workdir}/projectinfo.json)
                FILE(READ "${workdir}/projectinfo.json" projectJson)
                sbeParseJson(projectInfo projectJson)
                resolve_assigns(${projectInfo.DestDir} targetdir ${workdir})
                SET(EXPORT_DIR ${targetdir})
                set(WORK_DIR "${workdir}")
            else()
                MESSAGE(WARNING "No projectinfo found in project folder, setting default export")
                set(EXPORT_DIR "${workdir}/export")
                set(WORK_DIR "${workdir}")
            endif()
        else()
            # use environment
            set(EXPORT_DIR $ENV{NEBULA_WORK}/export)
            set(WORK_DIR $ENV{NEBULA_WORK})
        endif()
    endif()
endmacro()

macro(add_shaders_recursive)

    set(base_path ${ARGV0})
    set(full_path ${ARGV1})
    set(system_shader ARGV3)
    file(GLOB CHILDREN LIST_DIRECTORIES true ${full_path}/*)
    file(GLOB FXH ${full_path}/*.fxh)
    file(GLOB FX ${full_path}/*.fx)
    file(RELATIVE_PATH DIR ${base_path} ${full_path})
    if (FXH)
        fips_files(${FXH})
        if (system_shader)    
            SOURCE_GROUP(TREE "${full_path}" PREFIX "res\\shaders\\${DIR}" FILES ${FXH})
        endif()
    endif()
    set(CurDir ${DIR}/)
    foreach(shd ${FX})
        add_shader_intern(${shd} ${system_shader})
    endforeach()
    set(CurDir "")

    if (system_shader)
        SOURCE_GROUP(TREE "${full_path}" PREFIX "res\\shaders\\${DIR}" FILES ${FX})
    endif()
    
    foreach(CHILD ${CHILDREN})
        if (IS_DIRECTORY ${CHILD})
            add_shaders_recursive(${base_path} ${CHILD} ${system_shader})
        endif()
    endforeach()
endmacro()

macro(add_nebula_shaders)
    if(NOT SHADERC)
        MESSAGE(WARNING "Not compiling shaders, anyfxcompiler not found, did you run fips anyfx setup?")
    else()
        set_nebula_export_dir()
        
        add_shaders_recursive("${NROOT}/syswork/shaders/vk" "${NROOT}/syswork/shaders/vk" true)
        add_shaders_recursive("${workdir}/syswork/shaders/vk" "${workdir}/syswork/shaders/vk" false)
        
        # add configurations for the .vscode anyfx linter
        execute_process(COMMAND python ${NROOT}/fips-files/anyfx_linter/add_include_dir.py ${FIPS_PROJECT_DIR}/.vscode/anyfx_properties.json ${NROOT}/syswork/shaders/vk)

        file(GLOB_RECURSE FRM "${NROOT}/syswork/frame/win32/*.json")
        foreach(shd ${FRM})
            add_frameshader_intern(${shd})
        endforeach()

        file(GLOB_RECURSE FRMW "${workdir}/work/frame/win32/*.json")
        foreach(shd ${FRMW})
            add_frameshader_intern(${shd})
        endforeach()

         file(GLOB_RECURSE MAT "${NROOT}/syswork/materials/*.json")
        foreach(shd ${MAT})
            add_material_intern(${shd})
        endforeach()

        file(GLOB_RECURSE MATW "${workdir}/work/materials/*.json")
        foreach(shd ${MATW})
            add_material_intern(${shd})
        endforeach()
    endif()
endmacro()

macro(nebula_add_blueprints)
    set_nebula_export_dir()
    if (EXISTS "${FIPS_PROJECT_DIR}/work/data/tables/blueprints.json")
        add_blueprint_intern("${FIPS_PROJECT_DIR}/work/data/tables/blueprints.json")
    else()
        add_blueprint_intern("${NROOT}/syswork/data/tables/blueprints.json")
    endif()    
endmacro()

macro(compile_gpulang)
    if(NOT GPULANGC)
        MESSAGE(WARNING "Not compiling shaders, GPULang not found, did you run fips gpulang setup?")
    else()
        set_nebula_export_dir()
        foreach(shd ${ARGN})
            compile_gpulang_intern(${CMAKE_CURRENT_SOURCE_DIR}/${CurDir}${shd} false)
        endforeach()
    endif()
endmacro()

macro(add_shaders)
    if(NOT SHADERC)
        MESSAGE(WARNING "Not compiling shaders, anyfxcompiler not found, did you run fips anyfx setup?")
    else()
        set_nebula_export_dir()
        foreach(shd ${ARGN})
            add_shader_intern(${CMAKE_CURRENT_SOURCE_DIR}/${CurDir}${shd} false)
        endforeach()

        # add configurations for the .vscode anyfx linter
        SET(folders)
        foreach(shd ${ARGN})
            get_filename_component(foldername ${CMAKE_CURRENT_SOURCE_DIR}/${CurDir}${shd} DIRECTORY)
            list(APPEND folders ${foldername})
        endforeach()
        execute_process(COMMAND python ${NROOT}/fips-files/anyfx_linter/add_include_dir.py ${FIPS_PROJECT_DIR}/.vscode/anyfx_properties.json ${folders})

    endif()
endmacro()

macro(nebula_begin_app name type)
    fips_begin_app(${name} ${type})
    set(target_has_nidl 0)
    set(target_has_shaders 0)
    set(target_has_flatc 0)
    set(target_has_materials 0)
    set(target_has_frame_script 0)
    if(N_EDITOR)
        add_compile_definitions(WITH_NEBULA_EDITOR)
    endif()
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}")
    set_target_properties(${name} PROPERTIES COMPILE_WARNING_AS_ERROR TRUE)
endmacro()

macro(nebula_end_app)
    set(curtarget ${CurTargetName})
    fips_end_app()
    if(target_has_nidl)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/${CurTargetName}")
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/")
    endif()
    if (target_has_shaders)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/shaders/${CurTargetName}")
    endif()
    if (target_has_flatc)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/generated/")
    endif()
    if (target_has_materials)
        nebula_material_template_compile()
    endif()
    if (target_has_frame_script)
        nebula_framescript_compile()
    endif()
    set_target_properties(${curtarget} PROPERTIES ENABLE_EXPORTS false)
    if (TARGET system_resources-res)
        target_link_libraries(${curtarget} $<TARGET_OBJECTS:system_resources-res>)
    endif()
    if(N_DEBUG_SYMBOLS)
        target_compile_options(${curtarget} PRIVATE $<IF:$<CONFIG:Debug>,/Zi,/Z7>)
    endif()
endmacro()

macro(nebula_begin_module name)
    fips_begin_lib(${name})
    set(target_has_nidl 0)
    set(target_has_shaders 0)
    set(target_has_flatc 0)
    set(target_has_materials 0)
    set(target_has_frame_script 0)
    if(N_EDITOR)
        add_compile_definitions(WITH_NEBULA_EDITOR)
    endif()
    set_target_properties(${name} PROPERTIES COMPILE_WARNING_AS_ERROR TRUE)
endmacro()

macro(nebula_end_module)
    set(curtarget ${CurTargetName})
    fips_end_lib()
    if(target_has_nidl)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/${CurTargetName}")
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/")
    endif()
    if (target_has_shaders)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/shaders/${CurTargetName}")
    endif()
    if (target_has_flatc)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/generated")
    endif()
    if (target_has_materials)
        nebula_material_template_compile()
    endif()
    if (target_has_frame_script)
        nebula_framescript_compile()
    endif()
    if(N_DEBUG_SYMBOLS)
        target_compile_options(${curtarget} PRIVATE $<IF:$<CONFIG:Debug>,/Zi,/Z7>)
    endif()
endmacro()

macro(nebula_begin_lib name)
    fips_begin_lib(${name})
    set(target_has_nidl 0)
    set(target_has_shaders 0)
    set(target_has_flatc 0)
    set(target_has_materials 0)
    set(target_has_frame_script 0)
    if(N_EDITOR)
        add_compile_definitions(WITH_NEBULA_EDITOR)
    endif()
endmacro()

macro(nebula_end_lib)
    set(curtarget ${CurTargetName})
    fips_end_lib()
    if(target_has_nidl)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/${CurTargetName}")
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/")
    endif()
    if (target_has_shaders)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/shaders/${CurTargetName}")
    endif()
    if (target_has_flatc)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/generated")
    endif()
    if (target_has_materials)
        nebula_material_template_compile()
    endif()
    if (target_has_frame_script)
        nebula_framescript_compile()
    endif()
    if(N_DEBUG_SYMBOLS)
        target_compile_options(${curtarget} PRIVATE $<IF:$<CONFIG:Debug>,/Zi,/Z7>)
    endif()
endmacro()

macro(reuse_pch target from)

endmacro()
