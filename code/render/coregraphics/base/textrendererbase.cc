//------------------------------------------------------------------------------
//  textrendererbase.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coregraphics/base/textrendererbase.h"

namespace Base
{
__ImplementClass(Base::TextRendererBase, 'DTRB', Core::RefCounted);
__ImplementSingleton(Base::TextRendererBase);

using namespace CoreGraphics;
using namespace Threading;
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
TextRendererBase::TextRendererBase() :
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
TextRendererBase::~TextRendererBase()
{
    n_assert(!this->IsOpen());
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
TextRendererBase::Open()
{
    n_assert(!this->IsOpen());
    this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void
TextRendererBase::Close()
{
    n_assert(this->IsOpen());
    this->isOpen = false;
    this->textElements.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
TextRendererBase::AddTextElement(const TextElement& textElement)
{
    n_assert(this->IsOpen());
    this->textElements.Append(textElement);
}

//------------------------------------------------------------------------------
/**
*/
void
TextRendererBase::AddTextElements(const Array<TextElement>& textElements)
{
    n_assert(this->IsOpen());
    this->textElements.AppendArray(textElements);
}

//------------------------------------------------------------------------------
/**
*/
void
TextRendererBase::DeleteTextElementsByThreadId(ThreadId threadId)
{
    n_assert(this->IsOpen());
    IndexT i;
    for (i = this->textElements.Size() - 1; i != InvalidIndex; i--)
    {
        ThreadId textThreadId = this->textElements[i].GetThreadId();
        n_assert(textThreadId != InvalidThreadId);
        if (textThreadId == threadId)
        {
            this->textElements.EraseIndex(i);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
TextRendererBase::DrawTextElements()
{
    // Handle this in the subclass and DON'T call the parent class' method,
    // since this will delete all text entries which may cause text flickering
    // when the game thread runs slower then the render thread.
    this->textElements.Clear();
}

} // namespace Base