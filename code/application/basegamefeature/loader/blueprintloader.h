#pragma once
//------------------------------------------------------------------------------
/**
	@class    BluePrintLoader

	Loads a blueprint into the BluePrintManager

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"
#include "core/refcounted.h"

namespace BaseGameFeature
{

class BluePrintLoader : public Core::RefCounted
{
public:
	/// load a complete level
	bool Load(const Util::String& file);
};

} // namespace BaseGameFeature
