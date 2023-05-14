#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::transform44

    A 4x4 matrix which is described by translation, rotation and scale.
    
    @copyright
    (C) 2004 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math/mat4.h"
#include "math/vec3.h"
#include "math/quat.h"

//------------------------------------------------------------------------------
namespace Math
{
class NEBULA_ALIGN16 transform44
{
public:
    /// constructor
    transform44();
    /// equality operator
    bool operator==(const transform44& rhs) const;
    /// load content from unaligned memory
    void loadu(const scalar* ptr);  
    /// write content to unaligned memory through the write cache
    void storeu(scalar* ptr) const;
    /// set position
    void setposition(const vec3& p);
    /// set position
    void setposition(const point& p);
    /// get position
    const point& getposition() const;
    /// set rotate
    void setrotate(const quat& r);
    /// get rotate
    const quat& getrotate() const;
    /// set scale
    void setscale(const vec3& s);
    /// get scale
    const vec3& getscale() const;
    /// set optional rotate pivot
    void setrotatepivot(const point& p);
    /// get optional rotate pivot
    const point& getrotatepivot() const;
    /// set optional scale pivot
    void setscalepivot(const point& p);
    /// get optional scale pivot
    const point& getscalepivot() const;
    /// set optional offset matrix
    void setoffset(const mat4& m);
    /// get optional offset matrix
    const mat4& getoffset() const;
    /// get resulting 4x4 matrix
    const mat4& getmatrix();
    /// return true if the transformation matrix is dirty
    bool isdirty() const;

private:
    point position;
    quat rotate;
    vec3 scale;
    point rotatePivot;
    point scalePivot;
    mat4 offset;
    mat4 matrix;
    bool isDirty;
    bool offsetValid;
};

//------------------------------------------------------------------------------
/**
*/
inline
transform44::transform44() :
    position(0.0f, 0.0f, 0.0f),
    rotate(0.0f, 0.0f, 0.0f, 1.0f),
    scale(1.0f, 1.0f, 1.0f),
    rotatePivot(0.0f, 0.0f, 0.0f),
    scalePivot(0.0f, 0.0f, 0.0f),
    offset(mat4()),
    isDirty(false),
    offsetValid(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline bool
transform44::operator==(const transform44& rhs) const
{
    return (this->position == rhs.position) && (this->rotate == rhs.rotate) && (this->scale == rhs.scale) && (this->rotatePivot == rhs.rotatePivot) && (this->scalePivot == rhs.scalePivot) && (this->matrix == rhs.matrix);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
transform44::loadu(const scalar* ptr)
{
    this->position.loadu(ptr);
    this->rotate.loadu(ptr + 4);
    this->scale.loadu(ptr + 8);
    this->rotatePivot.loadu(ptr + 12);
    this->scalePivot.loadu(ptr + 16);
    this->offset.loadu(ptr + 20);   
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
transform44::storeu(scalar* ptr) const
{
    this->position.storeu(ptr);
    this->rotate.storeu(ptr + 4);
    this->scale.storeu(ptr + 8);
    this->rotatePivot.storeu(ptr + 12);
    this->scalePivot.storeu(ptr + 16);
    this->offset.storeu(ptr + 20);
}

//------------------------------------------------------------------------------
/**
*/
inline void
transform44::setposition(const vec3& p)
{
    this->position = p;
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline void
transform44::setposition(const point& p)
{
    this->position = xyz(Math::vec4(p));
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline const point&
transform44::getposition() const
{
    return this->position;
}

//------------------------------------------------------------------------------
/**
*/
inline void
transform44::setrotate(const quat& r)
{
    this->rotate = r;
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline const quat&
transform44::getrotate() const
{
    return this->rotate;
}

//------------------------------------------------------------------------------
/**
*/
inline void
transform44::setscale(const vec3& s)
{
    this->scale = s;
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline const vec3&
transform44::getscale() const
{
    return this->scale;
}

//------------------------------------------------------------------------------
/**
*/
inline void
transform44::setrotatepivot(const point& p)
{
    this->rotatePivot = p;
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline const point&
transform44::getrotatepivot() const
{
    return this->rotatePivot;
}

//------------------------------------------------------------------------------
/**
*/
inline void
transform44::setscalepivot(const point& p)
{
    this->scalePivot = p;
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline const point&
transform44::getscalepivot() const
{
    return this->scalePivot;
}

//------------------------------------------------------------------------------
/**
*/
inline void
transform44::setoffset(const mat4& m)
{
    this->offset = m;
    this->offsetValid = true;
    this->isDirty = true;
}

//------------------------------------------------------------------------------
/**
*/
inline const mat4&
transform44::getoffset() const
{
    return this->offset;
}

//------------------------------------------------------------------------------
/**
*/
inline const mat4&
transform44::getmatrix()
{
    if (this->isDirty)
    {
        quat ident;
        this->matrix = transformation(xyz(this->scalePivot), ident, this->scale, xyz(this->rotatePivot), this->rotate, xyz(this->position));
        if (this->offsetValid)
        {
            this->matrix = this->offset * this->matrix;
        }
        this->isDirty = false;
    }
    return this->matrix;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
transform44::isdirty() const
{
    return this->isDirty;
}

} // namespace Math
//------------------------------------------------------------------------------

    
