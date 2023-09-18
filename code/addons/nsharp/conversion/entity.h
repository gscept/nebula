#pragma once
//------------------------------------------------------------------------------
/**
	Mono::Entity

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "monoconfig.h"
#include "game/entity.h"
#include "mono/metadata/image.h"
#include "mono/metadata/object.h"

namespace Mono
{

class Entity
{
public:
	static void Setup(MonoImage* image);
	static MonoObject* Convert(Game::Entity entity);
}; // namespace Mono

}