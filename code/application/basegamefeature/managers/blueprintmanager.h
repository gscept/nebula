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
#include "memdb/table.h"
#include "game/category.h"

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
	
	/// get a blueprint id
	static BlueprintId const GetBlueprintId(Util::StringAtom name);

	/// get a template id
	static TemplateId const GetTemplateId(Util::StringAtom name);

// private api
public:
	/// create an instance from blueprint. Note that this does not tie it to an entity! It's not recommended to create entities this way. @see entitymanager.h
	EntityMapping Instantiate(BlueprintId blueprint);
	EntityMapping Instantiate(TemplateId templateId);

private:
	/// constructor
	BlueprintManager();
	/// destructor
	~BlueprintManager();

	/// called when attached to game server. Needs to be attached after categorymanager
	static void OnActivate();

	/// parse entity blueprints file
	virtual bool ParseBlueprints();
	/// setup categories
	void SetupCategories();

	struct PropertyEntry
	{
		Util::StringAtom propertyName;
	};

	/// an entity blueprint
	struct Blueprint
	{
		// this is setup when calling SetupCategories
		/// The blueprint table. Contains all templates for the blueprint.
		MemDb::TableId tableId;
		
		/// category hash for the specific setup of properties
		CategoryHash categoryHash;
		/// the category id for the specific category that we instantiate to.
		CategoryId categoryId;

		// these are created by ParseBlueprints()
		Util::StringAtom name;
		Util::Array<PropertyEntry> properties;
	};

	/// contains all blueprints and their information.
	Util::Array<Blueprint> blueprints;

	/// maps from template name to template id, which is both BlueprintId and the the row within a blueprint table.
	Util::HashTable<Util::StringAtom, TemplateId> templateMap;

	/// maps from blueprint name to blueprint id, which is the index in the blueprints array.
	Util::HashTable<Util::StringAtom, BlueprintId> blueprintMap;

	static Util::String blueprintFilename;
	static Util::String blueprintFolder;
};

} // namespace Game
