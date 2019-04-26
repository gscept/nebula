#pragma once
//------------------------------------------------------------------------------
/**
	TagComponent

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"
#include "game/component/attribute.h"

namespace Attr
{
	__DeclareAttribute(Tag, Util::Guid, 'TAG!', Attr::ReadWrite, Util::Guid());
}

namespace Game
{

class TagComponent
{
	__DeclareComponent(TagComponent)
public:

private:
};

} // namespace Game