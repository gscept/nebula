#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::BlueprintManager

	Loads the 'data:tables/blueprint.json' file and subsequently sets up 
	categories based on the blueprints in the entity manager.

	You can instantiate entities from blueprints via the entity interface.

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/entity.h"
#include "game/manager.h"
#include "util/stringatom.h"

namespace Game
{

class Property;

class BlueprintManager
{
	__DeclareSingleton(BlueprintManager);
public:
	/// Create the singleton
	static ManagerAPI Create();

	/// Destroy the singleton
	static void Destroy();

	/// set a optional blueprints.xml, which is used instead of standard blueprint.xml
	static void SetBlueprintsFilename(const Util::String& name, const Util::String& folder);
	/// setup categories
	void SetupCategories();

private:
	/// constructor
	BlueprintManager();
	/// destructor
	~BlueprintManager();

	/// called when attached to game server. Needs to be attached after categorymanager
	static void OnActivate();

	/// parse entity blueprints file
	virtual bool ParseBluePrints();

	struct PropertyEntry
	{
		Util::StringAtom propertyName;
	};

	/// an entity blueprint, these are created by ParseBlueprints()
	struct BluePrint
	{
		Util::String name;
		Util::Array<PropertyEntry> properties;
	};
	Util::Array<BluePrint> bluePrints;

	static Util::String blueprintFilename;
	static Util::String blueprintFolder;
};

} // namespace Game
