#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::FactoryManager

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/entity.h"
#include "game/manager.h"

namespace Game
{

class Property;

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