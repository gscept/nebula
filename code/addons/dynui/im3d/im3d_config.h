#pragma once

#include "render/stdneb.h"
#include "core/config.h"
#include "memory/memory.h"
#include "math/vec4.h"
#include "math/vec2.h"
#include "math/mat4.h"

// Deprecated behavior.
#define IM3D_USE_DEPRECATED_DRAW_CONE 1 // Enable deprecated DrawCone() and DrawConeFilled() API (deprecated v1.18).
#include "memory/memory.h"

// User-defined assertion handler (default is cassert assert()).
#define IM3D_ASSERT(e) n_assert(e)

// User-defined malloc/free. Define both or neither (default is cstdlib malloc()/free()).
#define IM3D_MALLOC(size) Memory::Alloc(Memory::DefaultHeap, size)
#define IM3D_FREE(ptr) Memory::Free(Memory::DefaultHeap, ptr) 
// User-defined API declaration (e.g. __declspec(dllexport)).
//#define IM3D_API

// Use a thread-local context pointer.
//#define IM3D_THREAD_LOCAL_CONTEXT_PTR 1

// Use row-major internal matrix layout. 
//#define IM3D_MATRIX_ROW_MAJOR 1

// Force vertex data alignment (default is 4 bytes).
#define IM3D_VERTEX_ALIGNMENT 4

// Enable internal culling for primitives (everything drawn between Begin*()/End()). The application must set a culling frustum via AppData.
//#define IM3D_CULL_PRIMITIVES 1

// Enable internal culling for gizmos. The application must set a culling frustum via AppData.
//#define IM3D_CULL_GIZMOS 1

// Set a layer ID for all gizmos to use internally.
//#define IM3D_GIZMO_LAYER_ID 0xD4A1B5

// Conversion to/from application math types.
#define IM3D_VEC2_APP \
    Vec2(const Math::vec2& _v)        { x = _v.x; y = _v.y;     } \
    operator Math::vec2() const       { return Math::vec2(x, y); }
#define IM3D_VEC3_APP \
    Vec3(const Math::point& _v)       { x = _v.x; y = _v.y; z = _v.z; } \
    operator Math::point() const      { return Math::point(x, y, z);    } \
    Vec3(const Math::vector& _v)      { x = _v.x; y = _v.y; z = _v.z; } \
    operator Math::vector() const     { return Math::vector(x, y, z);    } \
    Vec3(const Math::vec3& _v)        { x = _v.x; y = _v.y; z = _v.z; } \
    operator Math::vec3() const       { return Math::vec3(x, y, z); }
#define IM3D_VEC4_APP \
    Vec4(const Math::point& _v)       { x = _v.x; y = _v.y; z = _v.z; w = 1.0f; } \
    operator Math::point() const      { return Math::point(x, y, z);    } \
    Vec4(const Math::vec4& _v)        { x = _v.x; y = _v.y; z = _v.z; w = _v.w; } \
    operator Math::vec4() const       { return Math::vec4(x, y, z, w); }
#define IM3D_MAT3_APP \
    Mat3(const Math::mat4& _m)    { float b[16]; _m.storeu(b); for(int j = 0 ; j < 3;++j) for (int i = 0; i < 3; ++i) m[i+j*3] = b[i + j*4]; } \
    operator Math::mat4() const   { Math::mat4 ret; float b[16]; ret.storeu(b); int k = 0; for(int j = 0 ; j < 3;++j) for (int i = 0; i < 3; ++i) b[i+4*j] = m[i+j*3]; ret.loadu(b); return ret; }
#define IM3D_MAT4_APP \
    Mat4(const Math::mat4& _m)    { _m.storeu(m);} \
    operator Math::mat4() const   { Math::mat4 ret; ret.loadu(m); return ret; }
