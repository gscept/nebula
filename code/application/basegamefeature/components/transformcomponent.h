#pragma once
//------------------------------------------------------------------------------
/**
	TransformComponentBase

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "util/dictionary.h"
#include "transformcomponentbase.h"

namespace Game
{

class TransformComponent : public TransformComponentBase
{
	__DeclareClass(TransformComponent)
public:
	TransformComponent();
	~TransformComponent();

	void SetLocalTransform(const uint32_t& instance, const Math::matrix44& val);
};

} // namespace Game