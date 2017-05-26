#pragma once
//------------------------------------------------------------------------------
/**
    @class Base::TextRendererBase
    
    Base class for text rendering (don't use this for high-quality
    text rendering).
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "util/string.h"
#include "math/float4.h"
#include "math/float2.h"
#include "threading/threadid.h"
#include "coregraphics/textelement.h"

//------------------------------------------------------------------------------
namespace Base
{
class TextRendererBase : public Core::RefCounted
{
    __DeclareClass(TextRendererBase);
    __DeclareSingleton(TextRendererBase);
public:
    /// constructor
    TextRendererBase();
    /// destructor
    virtual ~TextRendererBase();

    /// open the the text renderer
    void Open();
    /// close the text renderer
    void Close();
    /// check if text renderer open
    bool IsOpen() const;
    /// draw the accumulated text
    void DrawTextElements();
    /// delete added text by thread id
    void DeleteTextElementsByThreadId(Threading::ThreadId threadId);

    /// add text element for rendering
    void AddTextElement(const CoreGraphics::TextElement& textElement);
    /// add multiple text elements for rendering
    void AddTextElements(const Util::Array<CoreGraphics::TextElement>& textElement);

protected:
    Util::Array<CoreGraphics::TextElement> textElements;
    bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
TextRendererBase::IsOpen() const
{
    return this->isOpen;
}

} // namespace Base
//------------------------------------------------------------------------------


    