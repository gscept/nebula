#pragma once
//------------------------------------------------------------------------------
/**
    @class  Game::BlueprintManager

    Loads the 'data:tables/blueprint.json' file and subsequently sets up 
    categories based on the blueprints in the entity manager.

    You can instantiate entities from blueprints via the entity interface.

    @see api.h
    @see Game::EntityManager

    @copyright
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
#include "game/api.h"

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
    /// create an instance from blueprint. Note that this does not tie it to an entity! It's not recommended to create entities this way. @see Game::EntityManager @see api.h
    EntityMapping Instantiate(BlueprintId blueprint);
    /// create an instance from template. Note that this does not tie it to an entity! It's not recommended to create entities this way. @see Game::EntityManager @see api.h
    EntityMapping Instantiate(TemplateId templateId);

private:
    /// constructor
    BlueprintManager();
    /// destructor
    ~BlueprintManager();

    /// called when attached to game server. Needs to be attached after categorymanager
    static void OnActivate();

    /// parse entity blueprints file
    bool ParseBlueprint(Util::String const& blueprintsPath);
    /// load a template folder
    bool LoadTemplateFolder(Util::String const& path);
    /// parse blueprint template file
    bool ParseTemplate(Util::String const& templatePath);
    /// setup blueprint database
    void SetupBlueprints();
    /// create a category in the world db
    CategoryId CreateCategory(BlueprintId bid);

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
        // these are created by ParseBlueprints()
        Util::StringAtom name;
        /// contains all the properties for this blueprint
        Util::Array<PropertyEntry> properties;
    };

    /// contains all blueprints and their information.
    Util::Array<Blueprint> blueprints;

    /// maps from template name to template id, which is both BlueprintId and the the row within a blueprint table.
    Util::HashTable<Util::StringAtom, TemplateId> templateMap;

    /// maps from blueprint name to blueprint id, which is the index in the blueprints array.
    Util::HashTable<Util::StringAtom, BlueprintId> blueprintMap;

    static Util::String blueprintFolder;
    static Util::String templatesFolder;
};

} // namespace Game
