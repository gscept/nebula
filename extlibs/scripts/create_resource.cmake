if (MSVC)
set(rcid 1 CACHE INTERNAL "rcid")
function(create_resource root input_paths lib)
  set(rc-file-contents "")
  set(emplace-statements "")
  set(offset ${rcid})
  foreach(p ${input_paths})
    file(RELATIVE_PATH rel-path ${root} ${p})
    string(APPEND emplace-statements "    m.emplace(\"${rel-path}\", ${rcid});\n")
    string(APPEND create-statements "    create_resource(${rcid}),\n")
    string(APPEND rc-file-contents "${rcid} RCDATA \"${p}\"\n")
    math(EXPR rcid "${rcid}+1")
    set(rcid ${rcid} CACHE INTERNAL "rcid")
  endforeach(p input_paths)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/resource.rc ${rc-file-contents})
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${lib}/include/${lib}.h "\
#pragma once

#include <cstddef>
#include <map>
#include <string>

namespace ${lib} {

struct resource {
  std::size_t size_{0U};
  void const* ptr_{nullptr};
};

resource get_resource(std::string const&);
int get_resource_id(std::string const&);
resource make_resource(int id);
std::map<std::string, int> const& get_resource_ids();

}  // namespace ${lib}
")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/${lib}.cc "\
#include \"${lib}.h\"

#include <map>
#include <string>

#include \"windows.h\"

namespace ${lib} {

static auto resource_ids = [] {
  std::map<std::string, int> m;
${emplace-statements}\
  return m;
}();

resource create_resource(int id) {
  auto const a = FindResource(nullptr, MAKEINTRESOURCEA(id), RT_RCDATA);
  auto const mem = LoadResource(nullptr, a);
  auto const size = SizeofResource(nullptr, a);
  auto const ptr = LockResource(mem);
  return resource{size, ptr};
}

resource make_resource(int id) {
  resource res[] = {
    ${create-statements}\
  };
  return res[id - ${offset}];
}

int get_resource_id(std::string const& s) {
  return resource_ids.at(s);
}

resource get_resource(std::string const& s) {
  return make_resource(get_resource_id(s));
}

std::map<std::string, int> const& get_resource_ids() {
  return resource_ids;
}

}  // namespace ${lib}
")
  add_library(${lib}-res EXCLUDE_FROM_ALL OBJECT ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/resource.rc)

  add_library(${lib} EXCLUDE_FROM_ALL ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/${lib}.cc)
  target_compile_features(${lib} PUBLIC cxx_std_17)
  target_include_directories(${lib} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${lib}/include)
  target_link_libraries(${lib} ${lib}-res)
endfunction(create_resource)

elseif(APPLE)

function(create_resource root input_paths lib)
  # Write an empty dummy object file because the ld call needs this.
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/${lib}-stub.c "")
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/stub.o
    COMMAND
      ${CMAKE_C_COMPILER}
        -o ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/stub.o
        -c ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/${lib}-stub.c
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lib}/all_res.bin
    COMMAND cat ${input_paths} > ${CMAKE_CURRENT_BINARY_DIR}/${lib}/all_res.bin
    COMMENT Concatenate ${lib} resource file with all resources.
    DEPENDS ${input_paths}
  )

  if (DEFINED ARGV3)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lib}/${lib}-res.cc
      COMMAND embedfiles ${CMAKE_CURRENT_BINARY_DIR}/${lib}/${lib}-res.cc ${lib} ${input_paths}
      DEPENDS embedfiles
    )
    add_library(${lib}-res ${CMAKE_CURRENT_BINARY_DIR}/${lib}/${lib}-res.cc)
  endif()

  set(id 0)
  set(res-offset 0)
  set(res-total-size 0)
  foreach(p ${input_paths})
    file(SIZE "${p}" res-input-length)
    math(EXPR res-total-size "${res-total-size}+${res-input-length}")
    math(EXPR res-size "${res-total-size}-${res-offset}")

    string(APPEND resource-statements "    create_resource(${res-offset}, ${res-size}),\n")

    file(RELATIVE_PATH rel-path ${root} ${p})
    string(APPEND emplace-statements "    m.emplace(\"${rel-path}\", ${id});\r\n")

    math(EXPR res-offset "${res-offset}+${res-size}")
    math(EXPR id "${id}+1")
  endforeach(p input_paths)

  string(SUBSTRING ${lib} 0 15 short-lib)
  if ("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
    message(STATUS "res arch is CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")
  else()
    set(arch-flag -arch ${CMAKE_OSX_ARCHITECTURES})
    message(STATUS "res arch is CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  endif()
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/${lib}-res.o
    COMMAND
      ld
        ${arch-flag}
        -r -o ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/${lib}-res.o
        -sectcreate binary ${short-lib} ${CMAKE_CURRENT_BINARY_DIR}/${lib}/all_res.bin
        ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/stub.o
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/stub.o
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${lib}/all_res.bin
    WORKING_DIRECTORY ${root}
    COMMENT "Generating resource ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/${lib}-res.o"
    VERBATIM
  )

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${lib}/include/${lib}.h "\
#pragma once

#include <cstddef>
#include <map>
#include <string>

namespace ${lib} {

struct resource {
  std::size_t size_{0U};
  void const* ptr_{nullptr};
};

resource get_resource(std::string const&);
int get_resource_id(std::string const&);
resource make_resource(int id);
std::map<std::string, int> const& get_resource_ids();

}  // namespace ${lib}
")


if (DEFINED ARGV3)
  set(res-${lib}-access "\
namespace ${lib} {

extern uint32_t size;
extern uint8_t const* const base;")
else()
  set(res-${lib}-access "\
#include <mach-o/getsect.h>

extern const struct mach_header_64 _mh_execute_header;

namespace ${lib} {

static auto size = size_t{};
static uint8_t const* const base = getsectiondata(
    &_mh_execute_header, \"binary\", \"${short-lib}\", &size);")
endif()

    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/${lib}.cc "\
#include \"${lib}.h\"

#include <map>
#include <string>

${res-${lib}-access}

static auto resource_ids = [] {
  std::map<std::string, int> m;
${emplace-statements}\
  return m;
}();

resource create_resource(size_t const offset, size_t const size) {
  return resource{size, base + offset};
}

resource make_resource(int id) {
  static resource resources[] = {
${resource-statements}\
  };
  return resources[id];
}

int get_resource_id(std::string const& s) {
  return resource_ids.at(s);
}

resource get_resource(std::string const& s) {
  return make_resource(get_resource_id(s));
}

std::map<std::string, int> const& get_resource_ids() {
  return resource_ids;
}

}  // namespace ${lib}
")


  if (NOT DEFINED ARGV3)
    set_source_files_properties(${lib}-res.o
      PROPERTIES
      EXTERNAL_OBJECT true
      GENERATED true)
    add_library(${lib}-res OBJECT IMPORTED GLOBAL)
    set_target_properties(${lib}-res
      PROPERTIES
      IMPORTED_OBJECTS
      ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/${lib}-res.o)
  endif()

  add_library(${lib} EXCLUDE_FROM_ALL STATIC ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/${lib}.cc)
  target_compile_features(${lib} PUBLIC cxx_std_17)
  target_include_directories(${lib} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${lib}/include)
  target_link_libraries(${lib} ${lib}-res)
endfunction()

else()

function(create_resource root input_paths lib)
  if (CMAKE_CROSSCOMPILING)
    set(resource-linker ${CMAKE_LINKER})
  else()
    set(resource-linker ld)
  endif()
  if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "i686")
    set(flags "-melf_i386")
  endif()
  set(id 0)
  foreach(p ${input_paths})
    file(RELATIVE_PATH rel-path ${root} ${p})
    string(REGEX REPLACE "[^0-9a-zA-Z]" "_" mangled-path ${rel-path})
    string(APPEND extern-definitions "extern const char _binary_${mangled-path}_start, _binary_${mangled-path}_end;\n")
    string(APPEND resource-statements "  resource{\
static_cast<std::size_t>(&_binary_${mangled-path}_end - &_binary_${mangled-path}_start),\
reinterpret_cast<void const*>(&_binary_${mangled-path}_start)},\n")
    string(APPEND emplace-statements "    m.emplace(\"${rel-path}\", ${id});\r\n")
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/res_${id}.o
      COMMAND ${resource-linker} ${flags} -r -b binary -o ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/res_${id}.o ${rel-path}
      DEPENDS ${p}
      WORKING_DIRECTORY ${root}
      COMMENT "Generating resource ${rel-path} (${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/res_${id}.o)"
      VERBATIM
    )
    list(APPEND o-files ${CMAKE_CURRENT_BINARY_DIR}/${lib}/obj/res_${id}.o)
    math(EXPR id "${id}+1")
  endforeach(p input_paths)

  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${lib}/include/${lib}.h "\
#pragma once

#include <cstddef>
#include <map>
#include <string>

namespace ${lib} {

struct resource {
  std::size_t size_{0U};
  void const* ptr_{nullptr};
};

resource get_resource(std::string const&);
int get_resource_id(std::string const&);
resource make_resource(int id);
std::map<std::string, int> const& get_resource_ids();

}  // namespace ${lib}
")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/${lib}.cc "\
#include \"${lib}.h\"

#include <map>
#include <string>

${extern-definitions}

namespace ${lib} {

static auto resource_ids = [] {
  std::map<std::string, int> m;
${emplace-statements}\
  return m;
}();

resource resources[] = {
${resource-statements}\
};

resource make_resource(int id) {
  return resources[id];
}

int get_resource_id(std::string const& s) {
  return resource_ids.at(s);
}

resource get_resource(std::string const& s) {
  return make_resource(get_resource_id(s));
}

std::map<std::string, int> const& get_resource_ids() {
  return resource_ids;
}

}  // namespace ${lib}
")
  set_source_files_properties(${o-files} PROPERTIES
    EXTERNAL_OBJECT true
    GENERATED true
  )
  add_library(${lib}-res OBJECT IMPORTED GLOBAL)
  set_target_properties(${lib}-res PROPERTIES IMPORTED_OBJECTS "${o-files}")

  add_library(${lib} EXCLUDE_FROM_ALL STATIC ${CMAKE_CURRENT_BINARY_DIR}/${lib}/src/${lib}.cc)
  target_link_libraries(${lib} ${lib}-res)
  target_compile_features(${lib} PUBLIC cxx_std_17)
  target_include_directories(${lib} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${lib}/include)
endfunction()

endif()
