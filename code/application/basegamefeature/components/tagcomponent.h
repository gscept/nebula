#pragma once
//------------------------------------------------------------------------------
/**
	TagComponent

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/component/component.h"
#include "game/attr/attributedefinition.h"

//------------------------------------------------------------------------------
namespace Attr
{
	DeclareGuid(Tag, 'TAG!', Attr::ReadWrite);
};
//------------------------------------------------------------------------------

namespace Game
{

class TagComponent : public Component<decltype(Attr::Tag)>
{
	__DeclareClass(TagComponent)
public:
	TagComponent();
	~TagComponent();

	enum Attributes
	{
		OWNER,
		TAG,
		TAGNAME,
		NumAttributes
	};

	/// Setup callbacks.
	void SetupAcceptedMessages();

	uint32_t RegisterEntity(const Entity& e);
	void DeregisterEntity(const Entity& e);
	SizeT Optimize();

private:
};

} // namespace Game