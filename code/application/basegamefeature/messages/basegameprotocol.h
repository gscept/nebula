#pragma once
//------------------------------------------------------------------------------
/**
	@file	basegameprotocol.h

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/messaging/message.h"
#include "math/matrix44.h"
#include "game/entity.h"

namespace Msg
{
	__DeclareMsg(SetLocalTransform, 'SLTM', Game::Entity, Math::matrix44)
	__DeclareMsg(UpdateTransform, 'UpdT', Game::Entity, Math::matrix44)

	/// Set parent. First argument is reciever, second is parent
	__DeclareMsg(SetParent, 'SetP', Game::Entity, Game::Entity)
} // namespace Msg
