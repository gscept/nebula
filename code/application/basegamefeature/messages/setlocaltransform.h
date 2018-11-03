#pragma once
//------------------------------------------------------------------------------
/**
	SetLocalTransformMessage

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/messaging/message.h"
#include "math/matrix44.h"
#include "game/entity.h"

namespace Msg
{
	__DeclareMsg(SetLocalTransform, 'SLTM', Game::Entity, Math::matrix44)
} // namespace Msg
