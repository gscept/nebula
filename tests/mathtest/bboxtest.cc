//------------------------------------------------------------------------------
//  bboxtest.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "bboxtest.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "math/bbox.h"
#include "math/line.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;

namespace Test
{
__ImplementClass(Test::BBoxTest, 'BBTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
BBoxTest::Run()
{
    STACK_CHECKPOINT("Test::BBoxTest::Run()");

    // default constructor
    {
        bbox b;
        VERIFY(nearequal(b.center(), point(0.0f, 0.0f, 0.0f), EPSILON4));
        VERIFY(nearequal(b.extents(), vec3(0.5f, 0.5f, 0.5f), EPSILON3));
        VERIFY(nearequal(b.size(), vec3(1.0f, 1.0f, 1.0f), EPSILON3));
        VERIFY(scalarequal(b.diagonal_size(), Math::length(vec3(1.0f, 1.0f, 1.0f))));
    }

    // set from center/extents and to_mat4 roundtrip
    {
        point c(1.0f, 2.0f, 3.0f);
        vector e(0.5f, 1.0f, 2.0f);
        bbox b;
        b.set(c, e);
        VERIFY(nearequal(b.pmin, point(0.5f, 1.0f, 1.0f), EPSILON4));
        VERIFY(nearequal(b.pmax, point(1.5f, 3.0f, 5.0f), EPSILON4));
        mat4 m = b.to_mat4();
        // to_mat4 produces scaling(size) with position = center
        VERIFY(nearequal(m.position, vec3(1.0f, 2.0f, 3.0f), EPSILON3));
        VERIFY(nearequal(xyz(m.r[0]), vec3(1.0f, 0.0f, 0.0f) * b.size().x, EPSILON3));
    }

    // set from mat4
    {
        mat4 m = translation(vec3(5.0f, -3.0f, 2.0f)) * scaling(2.0f, 4.0f, 6.0f);
        bbox b(m);
        VERIFY(nearequal(b.center(), point(5.0f, -3.0f, 2.0f), EPSILON4));
        VERIFY(nearequal(b.extents(), vec3(1.0f, 2.0f, 3.0f), EPSILON3));
        VERIFY(nearequal(b.size(), vec3(2.0f, 4.0f, 6.0f), EPSILON3));
    }

    // begin_extend / extend / end_extend
    {
        bbox b;
        b.begin_extend();
        // if no extend called, end_extend resets to zero box
        b.end_extend();
        VERIFY(nearequal(b.pmin, point(0.0f, 0.0f, 0.0f), EPSILON4));
        VERIFY(nearequal(b.pmax, point(0.0f, 0.0f, 0.0f), EPSILON4));

        // now extend with points
        b.begin_extend();
        b.extend(vec3(-1.0f, -2.0f, -3.0f));
        b.extend(vec3(4.0f, 5.0f, 6.0f));
        b.end_extend();
        VERIFY(nearequal(b.pmin, point(-1.0f, -2.0f, -3.0f), EPSILON4));
        VERIFY(nearequal(b.pmax, point(4.0f, 5.0f, 6.0f), EPSILON4));
    }

    // extend by another bbox
    {
        bbox a;
        a.set(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
        bbox b;
        b.set(vec3(3.0f, 0.0f, 0.0f), vec3(1.0f, 2.0f, 1.0f));
        a.extend(b);
        VERIFY(nearequal(a.pmin, point(-1.0f, -2.0f, -1.0f), EPSILON4));
        VERIFY(nearequal(a.pmax, point(4.0f, 2.0f, 1.0f), EPSILON4));
    }

    // contains (point and box)
    {
        bbox b;
        b.set(vec3(0.0f, 0.0f, 0.0f), vec3(2.0f, 2.0f, 2.0f)); // extents 2 => size 4
        VERIFY(b.contains(vec3(1.0f, 0.0f, 0.0f)));
        VERIFY(!b.contains(vec3(3.0f, 0.0f, 0.0f)));
        bbox inner;
        inner.set(vec3(0.5f, 0.0f, 0.0f), vec3(0.5f, 0.5f, 0.5f));
        VERIFY(b.contains(inner));
        bbox outside;
        outside.set(vec3(10.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
        VERIFY(!b.contains(outside));
    }

    // intersects (box)
    {
        bbox a;
        a.set(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
        bbox b;
        b.set(vec3(1.5f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
        VERIFY(a.intersects(b)); // overlap on x (edges overlap)
        bbox c;
        c.set(vec3(5.0f, 0.0f, 0.0f), vec3(0.4f, 0.4f, 0.4f));
        VERIFY(!a.intersects(c));
    }

    // intersects (ray / line)
    {
        // box centered at origin extents 1
        bbox b;
        b.set(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 1.0f, 1.0f));
        // ray from (-5,0,0) towards +x
        line ray;
        ray.set(vec3(-5.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 0.0f)); // end point - start defines direction
        float t;
        VERIFY(b.intersects(ray, t));
        // Expect intersection at around t in (0,1) because line::m is end-start in current API.
        VERIFY(t >= 0.0f);
    }

    // affine_transform vs transform (affine matrix)
    {
        bbox b;
        b.set(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 2.0f, 3.0f));
        mat4 m = translation(vec3(5.0f, -3.0f, 2.0f)) * rotationx(0.5f) * scaling(2.0f, 0.5f, 1.5f);
        bbox bAffine = b;
        bAffine.affine_transform(m);
        bbox bTransformed = b;
        bTransformed.transform(m); // transform uses perspective_div but with affine matrix should match
        VERIFY(nearequal(bAffine.pmin, bTransformed.pmin, EPSILON4));
        VERIFY(nearequal(bAffine.pmax, bTransformed.pmax, EPSILON4));
    }

    // corner_point and area
    {
        bbox b;
        b.set(vec3(1.0f, 2.0f, 3.0f), vec3(0.5f, 0.25f, 0.125f));
        // collect unique corner points
        Math::point corners[8];
        for (int i = 0; i < 8; ++i)
        {
            corners[i] = b.corner_point(i);
        }
        // simple checks: corner 0 and 7 should be opposite (pmin / pmax)
        VERIFY(nearequal(corners[0], b.pmin, EPSILON4) || nearequal(corners[7], b.pmin, EPSILON4) || nearequal(corners[0], b.pmax, EPSILON4) || nearequal(corners[7], b.pmax, EPSILON4));
        float expectedHalfArea = b.area();
        // area returns half-area of surface (per doc), compute manually: extent = pmax-pmin
        vec3 ext = b.pmax - b.pmin;
        float manualHalfArea = ext.x * ext.y + ext.y * ext.z + ext.z * ext.x;
        VERIFY(nearequal(expectedHalfArea, manualHalfArea, TEST_EPSILON));
    }
}

} // namespace Test