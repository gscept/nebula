//------------------------------------------------------------------------------
//  transformtest.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "transformtest.h"
#include "math/point.h"
#include "math/vector.h"
#include "math/mat4.h"
#include "math/transform.h"

namespace Test
{
__ImplementClass(Test::TransformTest, 'TFTS', Test::TestCase);

using namespace Math;

namespace
{
inline bool
NearVec3(const vec3& a, const vec3& b, float eps = 0.0001f)
{
    return nearequal(a, b, eps);
}

inline bool
NearPoint(const point& a, const point& b, float eps = 0.0001f)
{
    return fequal(a.x, b.x, eps) && fequal(a.y, b.y, eps) && fequal(a.z, b.z, eps);
}

inline bool
NearVector(const vector& a, const vector& b, float eps = 0.0001f)
{
    return fequal(a.x, b.x, eps) && fequal(a.y, b.y, eps) && fequal(a.z, b.z, eps);
}
} // namespace

//------------------------------------------------------------------------------
/**
*/
void
TransformTest::Run()
{
    // default construction
    {
        transform t;
        VERIFY(NearVec3(t.position, vec3(0.0f)));
        VERIFY(NearVec3(t.scale, vec3(1.0f)));
        VERIFY(fequal(t.rotation.x, 0.0f, 0.0001f));
        VERIFY(fequal(t.rotation.y, 0.0f, 0.0001f));
        VERIFY(fequal(t.rotation.z, 0.0f, 0.0001f));
        VERIFY(fequal(t.rotation.w, 1.0f, 0.0001f));
    }

    // value construction
    {
        const vec3 p(1.0f, 2.0f, 3.0f);
        const quat r = rotationquataxis(vec3(0.0f, 1.0f, 0.0f), 0.5f);
        const vec3 s(2.0f, 3.0f, 4.0f);
        transform t(p, r, s);

        VERIFY(NearVec3(t.position, p));
        VERIFY(NearVec3(t.scale, s));
        VERIFY(fequal(t.rotation.x, r.x, 0.0001f));
        VERIFY(fequal(t.rotation.y, r.y, 0.0001f));
        VERIFY(fequal(t.rotation.z, r.z, 0.0001f));
        VERIFY(fequal(t.rotation.w, r.w, 0.0001f));
    }

    // Set() writes all components correctly
    {
        transform t;
        const vec3 p(7.0f, 8.0f, 9.0f);
        const quat r = rotationquataxis(vec3(1.0f, 0.0f, 0.0f), 0.25f);
        const vec3 s(1.5f, 2.5f, 3.5f);
        t.Set(p, r, s);

        VERIFY(NearVec3(t.position, p));
        VERIFY(NearVec3(t.scale, s));
        VERIFY(fequal(t.rotation.x, r.x, 0.0001f));
        VERIFY(fequal(t.rotation.y, r.y, 0.0001f));
        VERIFY(fequal(t.rotation.z, r.z, 0.0001f));
        VERIFY(fequal(t.rotation.w, r.w, 0.0001f));
    }

    // point transform applies scale, then rotation, then translation
    {
        transform t(vec3(10.0f, 0.0f, 0.0f), rotationquataxis(vec3(0.0f, 1.0f, 0.0f), 0.0f), vec3(2.0f, 2.0f, 2.0f));
        point p(1.0f, 2.0f, 3.0f);
        point out = t * p;
        VERIFY(NearPoint(out, point(12.0f, 4.0f, 6.0f)));
    }

    // vector transform ignores position
    {
        transform t(vec3(10.0f, 20.0f, 30.0f), rotationquataxis(vec3(0.0f, 1.0f, 0.0f), 0.0f), vec3(2.0f, 3.0f, 4.0f));
        vector v(1.0f, 2.0f, 3.0f);
        vector out = t * v;
        VERIFY(NearVector(out, vector(2.0f, 6.0f, 12.0f)));
    }

    // composition order: (a * b) applies a first, then b
    {
        transform a(vec3(1.0f, 0.0f, 0.0f), rotationquataxis(vec3(0.0f, 0.0f, 1.0f), 0.0f), vec3(2.0f, 2.0f, 2.0f));
        transform b(vec3(0.0f, 5.0f, 0.0f), rotationquataxis(vec3(0.0f, 0.0f, 1.0f), 0.0f), vec3(1.0f, 1.0f, 1.0f));
        point p(1.0f, 1.0f, 1.0f);

        point sequential = b * (a * p);
        point combined = (a * b) * p;
        VERIFY(NearPoint(combined, sequential));
    }

    // GetRelative contract in header: relative = this * inverse(child)
    // therefore relative * child should reconstruct this
    {
        transform parent(vec3(10.0f, 6.0f, 4.0f), rotationquataxis(vec3(0.0f, 1.0f, 0.0f), 0.3f), vec3(2.0f, 2.0f, 2.0f));
        transform child(vec3(3.0f, 1.0f, 2.0f), rotationquataxis(vec3(0.0f, 1.0f, 0.0f), 0.1f), vec3(1.5f, 1.5f, 1.5f));
        transform rel = parent.GetRelative(child);
        transform reconstructed = rel * child;

        point testPoint(0.5f, 1.0f, -2.0f);
        point p0 = parent * testPoint;
        point p1 = reconstructed * testPoint;
        VERIFY(NearPoint(p0, p1, 0.001f));
    }

    // lerp endpoints
    {
        transform a(vec3(0.0f, 0.0f, 0.0f), quat(), vec3(1.0f, 1.0f, 1.0f));
        transform b(vec3(10.0f, 20.0f, 30.0f), rotationquataxis(vec3(0.0f, 1.0f, 0.0f), 0.7f), vec3(3.0f, 4.0f, 5.0f));

        transform l0 = lerp(a, b, 0.0f);
        transform l1 = lerp(a, b, 1.0f);

        VERIFY(NearVec3(l0.position, a.position));
        VERIFY(NearVec3(l0.scale, a.scale));
        VERIFY(NearVec3(l1.position, b.position));
        VERIFY(NearVec3(l1.scale, b.scale));
    }

    // FromMat4 decompose roundtrip for affine transform (uniform scale)
    {
        const vec3 s(2.0f, 2.0f, 2.0f);
        const quat r = rotationquataxis(vec3(0.0f, 1.0f, 0.0f), 0.6f);
        const vec3 t(5.0f, 6.0f, 7.0f);

        mat4 m = affine(s, r, t);
        transform tr = transform::FromMat4(m);

        VERIFY(NearVec3(tr.position, t, 0.001f));
        VERIFY(NearVec3(tr.scale, s, 0.001f));

        point p(1.0f, 2.0f, 3.0f);
        point fromTransform = tr * p;
        point fromMatrix = m * p;
        VERIFY(NearPoint(fromTransform, fromMatrix, 0.001f));
    }
}

} // namespace Test
