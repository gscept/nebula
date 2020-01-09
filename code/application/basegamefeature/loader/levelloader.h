#pragma once
//------------------------------------------------------------------------------
/**
	@class LevelLoader

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"

namespace BaseGameFeature
{
class LevelLoader
{
public:
	/// save a complete level
	static bool Save(const Util::String& levelName);
	/// load a complete level
	static bool Load(const Util::String& levelName);
};

} // namespace BaseGameFeature
