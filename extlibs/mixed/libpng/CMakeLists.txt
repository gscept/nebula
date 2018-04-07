#-------------------------------------------------------------------------------
# libpng
#-------------------------------------------------------------------------------

FILE(GLOB SRC "*.c")
FILE(GLOB HEAD "*.h")
LIST(APPEND extlibs_libpng_SRCS ${SRC} ${HEAD})

SOURCE_GROUP(libpng FILES ${extlibs_libpng_SRCS})

ADD_LIBRARY(libpng STATIC ${extlibs_libpng_SRCS})
TARGET_INCLUDE_DIRECTORIES(libpng PUBLIC ${Nebula3_SOURCE_DIR}/code/extlibs private ${Nebula3_SOURCE_DIR}/code/extlibs/zlib)
TARGET_COMPILE_OPTIONS(libpng PRIVATE $<$<C_COMPILER_ID:MSVC>:/wd4996>)
TARGET_LINK_LIBRARIES(libpng zlib)
SET_TARGET_PROPERTIES(libpng PROPERTIES FOLDER "extlibs")

N_SET_COMPILER_DEFAULTS()
SET(CMAKE_C_FLAGS "")
