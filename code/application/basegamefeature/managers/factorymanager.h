#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::FactoryManager

	Loads the 'data:tables/blueprint.json' file and subsequently sets up 
	categories based on the blueprints in the category manager.

	You can instantiate entities from blueprints via the factory manager.

	Entities that has been instantiated from the factory manager should be destroyed
	by the EntityManager.

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

struct EntityCreateInfo
{
	/// which entity category to instantiate from
	Util::StringAtom categoryName;
	/// attributes specified here will be set before activating the entity.
	Util::Array<Game::Attribute> attributes;
};

class FactoryManager : public Game::Manager
{
	__DeclareClass(FactoryManager)
	__DeclareSingleton(FactoryManager)
public:
	/// constructor
	FactoryManager();
	/// destructor
	~FactoryManager();

	/// called when attached to game server. Needs to be attached after categorymanager
	void OnActivate() override;

	/// create a new entity from its category name
	Game::Entity CreateEntityByCategory(Util::StringAtom const categoryName) const;
	
	/// create a new entity from a createinfo description
	Game::Entity CreateEntity(EntityCreateInfo const& info) const;

	/// set a optional blueprints.xml, which is used instead of standard blueprint.xml
	static void SetBlueprintsFilename(const Util::String& name, const Util::String& folder);
	/// setup attributes on properties
	void SetupAttributes();
	/// setup categories
	void SetupCategories();

	/// create a new property by type name, extend in subclass for new types
	Ptr<Game::Property> CreateProperty(const Util::String& type) const;

private:
	/// parse entity blueprints file
	virtual bool ParseBluePrints();

	struct PropertyEntry
	{
		Util::String propertyName;
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
