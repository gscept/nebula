//------------------------------------------------------------------------------
//  sphere.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "math/sphere.h"
#include "util/random.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
    Check if sphere intersects with box.
    Taken from "Simple Intersection Tests For Games",
    Gamasutra, Oct 18 1999
*/
bool
sphere::intersects(const bbox& box) const
{
    n_error("sphere::intersects(): NOT IMPLEMENTED!");
    return false;
/*
    float s, d = 0;

    // find the square of the distance
    // from the sphere to the box,
    if (p.x < box.vmin.x)
    {
        s = p.x - box.vmin.x;
        d += s*s;
    }
    else if (p.x > box.vmax.x)
    {
        s = p.x - box.vmax.x;
        d += s*s;
    }

    if (p.y < box.vmin.y)
    {
        s = p.y - box.vmin.y;
        d += s*s;
    }
    else if (p.y > box.vmax.y)
    {
        s = p.y - box.vmax.y;
        d += s*s;
    }

    if (p.z < box.vmin.z)
    {
        s = p.z - box.vmin.z;
        d += s*s;
    }
    else if (p.z > box.vmax.z)
    {
        s = p.z - box.vmax.z;
        d += s*s;
    }

    return d <= r*r;
*/
}

//------------------------------------------------------------------------------
/**
*/
bool
sphere::intersects_ray(const line& ray) const
{
    vector oc = ray.start() - p;
    scalar a = dot(ray.vec(), ray.vec());
    scalar b = 2.0f * dot(oc, ray.vec());
    scalar c = dot(oc, oc) - r * r;
    scalar discriminant = b * b - 4.0f * a*c;
    return (discriminant > 0);
}

//------------------------------------------------------------------------------
/**
*/
vec3
sphere::random_point_on_unit_sphere()
{
    float x = Util::RandomFloatNTP();
    float y = Util::RandomFloatNTP();
    float z = Util::RandomFloatNTP();
    vec3 v( x, y, z );
    return normalize(v);
}

//------------------------------------------------------------------------------
/**
    Check if 2 moving spheres have contact.
    Taken from "Simple Intersection Tests For Games"
    article in Gamasutra, Oct 18 1999

    @param  va  [in] distance travelled by 'this'
    @param  sb  [in] the other sphere
    @param  vb  [in] distance travelled by sb
    @param  u0  [out] normalized intro contact
    @param  u1  [out] normalized outro contact
*/
bool
sphere::intersect_sweep(const vector& va, const sphere& sb, const vector& vb, float& u0, float& u1) const
{
    n_error("sphere::intersect_sweep(): NOT IMPLEMENTED!");
    return false;
    
/*
    vector3 vab(vb - va);
    vector3 ab(sb.p - p);
    float rab = r + sb.r;

    // check if spheres are currently overlapping...
    if ((ab % ab) <= (rab * rab)) 
    {
        u0 = 0.0f;
        u1 = 0.0f;
        return true;
    } 
    else 
    {
        // check if they hit each other
        float a = vab % vab;
        if ((a < -TINY) || (a > +TINY)) 
        {
            // if a is '0' then the objects don't move relative to each other
            float b = (vab % ab) * 2.0f;
            float c = (ab % ab) - (rab * rab);
            float q = b*b - 4*a*c;
            if (q >= 0.0f) 
            {
                // 1 or 2 contacts
                float sq = (float) sqrt(q);
                float d  = 1.0f / (2.0f*a);
                float r1 = (-b + sq) * d;
                float r2 = (-b - sq) * d;
                if (r1 < r2) 
                {
                    u0 = r1;
                    u1 = r2;
                } 
                else 
                {
                    u0 = r2;
                    u1 = r1;
                }
                return true;
            } 
            else 
            {
                return false;
            }
        } 
        else 
        {
            return false;
        }
    }
*/
}

//------------------------------------------------------------------------------
/**
    Project the sphere (defined in global space) to a screen space rectangle, 
    given the current View and Projection matrices. The method assumes that
    the sphere is at least partially visible.
*/
rectangle<scalar>
sphere::project_screen_rh(const mat4& view, const mat4& projection, float nearZ) const
{
    n_error("sphere::project_screen_rh(): NOT IMPLEMENTED!");
    return rectangle<scalar>(0, 0, 0, 0);
/*
    // compute center point of the sphere in view space
    vector3 viewPos = view * this->p;
    if (viewPos.z > -nearZ)
    {
        viewPos.z = -nearZ;
    }
    vector3 screenPos  = projection.mult_divw(viewPos);
    screenPos.y = -screenPos.y;

    // compute size of sphere at its front size
    float frontZ = viewPos.z + this->r;
    if (frontZ > -nearZ)
    {
        frontZ = -nearZ;
    }
    vector3 screenSize = projection.mult_divw(vector3(this->r, this->r, frontZ));
    screenSize.y = -screenSize.y;        
    float left   = n_saturate(0.5f * (1.0f + (screenPos.x - screenSize.x)));
    float right  = n_saturate(0.5f * (1.0f + (screenPos.x + screenSize.x)));
    float top    = n_saturate(0.5f * (1.0f + (screenPos.y + screenSize.y)));
    float bottom = n_saturate(0.5f * (1.0f + (screenPos.y - screenSize.y)));

    return rectangle(vector2(left, top), vector2(right, bottom));
*/
}

} // namespace Math
