#pragma once
//------------------------------------------------------------------------------
/**
    @function TypePunning
    
    Function to implement type-punning, explanation here:
        http://mail-index.netbsd.org/tech-kern/2003/08/11/0001.html
        http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Optimize-Options.html#Optimize-Options
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Util
{

template<typename A, typename B>
A& TypePunning(B &v)
{
    union
    {
        A *a;
        B *b;
    } pun;
    pun.b = &v;
    return *pun.a;
}

template<typename A, typename B>
const A& TypePunning(const B &v)
{
    union
    {
        const A *a;
        const B *b;
    } pun;
    pun.b = &v;
    return *pun.a;
}

} // namespace Util
//------------------------------------------------------------------------------

