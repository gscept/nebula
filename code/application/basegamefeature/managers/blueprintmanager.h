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
#include "game/manager.h"
#include "util/stringatom.h"
#include "game/api.h"
#include "ids/idgenerationpool.h"

namespace Game
{

class World;

class BlueprintManager : public Game::Manager
{
    __DeclareClass(BlueprintManager)
    __DeclareSingleton(BlueprintManager)
public:
    struct Template
    {
        BlueprintId bid;
        MemDb::RowId row; // row in blueprint table
        Util::StringAtom name;
    };

    /// constructor
    BlueprintManager();
    /// destructor
    ~BlueprintManager();

    void OnActivate() override;
    void OnDeactivate() override;
    /// set a optional blueprints.xml, which is used instead of standard blueprint.xml
    static void SetBlueprintsFilename(const Util::String& name, const Util::String& folder);
    /// get a blueprint id
    static BlueprintId const GetBlueprintId(Util::StringAtom name);
    /// get a template id
    static TemplateId const GetTemplateId(Util::StringAtom name);
    /// get template name
    static Util::StringAtom const GetTemplateName(TemplateId const templateId);
    /// 
    static Util::Array<Template> const& ListTemplates();

    /// create an instance from blueprint. Note that this does not tie it to an entity! It's not recommended to create entities this way. @see Game::EntityManager @see api.h
    EntityMapping Instantiate(World* const world, BlueprintId blueprint);
    /// create an instance from template. Note that this does not tie it to an entity! It's not recommended to create entities this way. @see Game::EntityManager @see api.h
    EntityMapping Instantiate(World* const world, TemplateId templateId);

private:
    /// parse entity blueprints file
    bool ParseBlueprint(Util::String const& blueprintsPath);
    /// load a template folder
    bool LoadTemplateFolder(Util::String const& path);
    /// parse blueprint template file
    bool ParseTemplate(Util::String const& templatePath);
    /// setup blueprint database
    void SetupBlueprints();
    /// create a table in the world db
    MemDb::TableId CreateCategory(World* const world, BlueprintId bid);

    struct ComponentEntry
    {
        Util::StringAtom name;
    };

    /// an entity blueprint
    struct Blueprint
    {
        // this is setup when calling SetupCategories
        /// The blueprint table. Contains all templates for the blueprint.
        MemDb::TableId tableId;
        // these are created by ParseBlueprints()
        Util::StringAtom name;
        /// contains all the components for this blueprint
        Util::Array<ComponentEntry> components;
    };

    Util::Array<Template> templates;

    /// contains all blueprints and their information.
    Util::Array<Blueprint> blueprints;

    Ids::IdGenerationPool templateIdPool;

    /// maps from template name to template id
    Util::HashTable<Util::StringAtom, TemplateId> templateMap;

    /// maps from blueprint name to blueprint id, which is the index in the blueprints array.
    Util::HashTable<Util::StringAtom, BlueprintId> blueprintMap;

    static Util::String blueprintFolder;
    static Util::String templatesFolder;
};

} // namespace Game
