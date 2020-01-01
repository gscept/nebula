#pragma once
#ifndef MATH_RECTANGLE_H
#define MATH_RECTANGLE_H
//------------------------------------------------------------------------------
/**
    @class Math::rectangle

    A 2d rectangle class.
    
    (C) 2003 RadonLabs GmbH
	(C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Math
{
template<class TYPE> class rectangle
{
public:
    /// default constructor
    rectangle();
    /// constructor 1
    rectangle(TYPE l, TYPE t, TYPE r, TYPE b);
    /// set content
    void set(TYPE l, TYPE t, TYPE r, TYPE b);
    /// return true if point is inside
    bool inside(TYPE x, TYPE y) const;
    /// return width
    TYPE width() const;
    /// return height
    TYPE height() const;
    /// return center x
    TYPE centerX() const;
    /// return center y
    TYPE centerY() const;

    TYPE left;
    TYPE top;
    TYPE right;
    TYPE bottom;
};

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
rectangle<TYPE>::rectangle()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE>
rectangle<TYPE>::rectangle(TYPE l, TYPE t, TYPE r, TYPE b) :
    left(l),
    top(t),
    right(r),
    bottom(b)
{
    n_assert(this->left <= this->right);
    n_assert(this->top <= this->bottom);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> void
rectangle<TYPE>::set(TYPE l, TYPE t, TYPE r, TYPE b)
{
    n_assert(l <= r);
    n_assert(t <= b);
    this->left = l;
    this->top = t;
    this->right = r;
    this->bottom = b;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> bool
rectangle<TYPE>::inside(TYPE x, TYPE y) const
{
    return (this->left <= x) && (x <= this->right) && (this->top <= y) && (y <= this->bottom);
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE
rectangle<TYPE>::width() const
{
    return this->right - this->left;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE
rectangle<TYPE>::height() const
{
    return this->bottom - this->top;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE
rectangle<TYPE>::centerX() const
{
    return (this->left + this->right) / 2;
}

//------------------------------------------------------------------------------
/**
*/
template<class TYPE> TYPE
rectangle<TYPE>::centerY() const
{
    return (this->top + this->bottom) / 2;
}
} // namespace Math
//------------------------------------------------------------------------------
#endif



