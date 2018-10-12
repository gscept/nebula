#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::TextElement
    
    Describes a text element for the text renderer.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "util/string.h"
#include "math/float4.h"
#include "math/float2.h"
#include "threading/threadid.h"

//------------------------------------------------------------------------------
namespace CoreGraphics
{
class TextElement
{
public:
    /// default constructor
    TextElement();
    /// constructor
    TextElement(Threading::ThreadId threadId, const Util::String& text, const Math::float4& color, const Math::float2& pos, const float size);
    
    /// get thread id
    Threading::ThreadId GetThreadId() const;
    /// get text
    const Util::String& GetText() const;
    /// get color
    const Math::float4& GetColor() const;
    /// get position
    const Math::float2& GetPosition() const;
	/// get size
	const float GetSize() const;
    
private:
    Threading::ThreadId threadId;
    Util::String text;
    Math::float4 color;
    Math::float2 pos;
	float size;
};

//------------------------------------------------------------------------------
/**
*/
inline Threading::ThreadId
TextElement::GetThreadId() const
{
    return this->threadId;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String&
TextElement::GetText() const
{
    return this->text;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float4&
TextElement::GetColor() const
{
    return this->color;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::float2&
TextElement::GetPosition() const
{
    return this->pos;
}


//------------------------------------------------------------------------------
/**
*/
inline const float 
TextElement::GetSize() const
{
	return this->size;
}

} // namespace CoreGraphics
//------------------------------------------------------------------------------


    