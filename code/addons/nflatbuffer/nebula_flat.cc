#include "foundation/stdneb.h"
#include "nebula_flat.h"
#include "flat/foundation/math.h"

#define LOAD_STORE_PACK(FT,NT)\
FT Pack(const NT& v){\
FT V;\
v.store(V.mutable_v()->data());\
return V;\
}\
NT UnPack(const FT& v){\
    NT m;\
    m.load(v.v()->data());\
    return m;\
}

namespace flatbuffers
{
LOAD_STORE_PACK(flat::Vec2, Math::vec2);
LOAD_STORE_PACK(flat::Vec3, Math::vec3);
LOAD_STORE_PACK(flat::Vec4, Math::vec4);
LOAD_STORE_PACK(flat::Mat4, Math::mat4);
}