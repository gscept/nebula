//------------------------------------------------------------------------------
//  streamtexturesaverbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/base/streamtexturesaverbase.h"

namespace Base
{
__ImplementClass(Base::StreamTextureSaverBase, 'STSB', Resources::ResourceSaver);

using namespace CoreGraphics;

//------------------------------------------------------------------------------
/**
*/
StreamTextureSaverBase::StreamTextureSaverBase() :
    format(ImageFileFormat::PNG),
    mipLevel(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
StreamTextureSaverBase::~StreamTextureSaverBase()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamTextureSaverBase::OnSave()
{
    // empty, override in subclass!
    return false;
}

} // namespace Base
