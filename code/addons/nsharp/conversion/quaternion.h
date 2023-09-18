#pragma once
//------------------------------------------------------------------------------
/**
	Mono::Quaternion

	Contains helper functions for creating and converting
	nebula quaternion's to mono objects.

	(C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "monoconfig.h"
#include "math/quat.h"
#include "mono/metadata/image.h"
#include "mono/metadata/object.h"

namespace Mono
{

class Quaternion
{
public:
	static void Setup(MonoImage* image);
	static MonoObject* Convert(Math::quat const& quat);
};

} // namespace Mono
