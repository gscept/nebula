#pragma once
//------------------------------------------------------------------------------
/**
    @file entityloader.h

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "basegamefeature/levelparser.h"
#include "util/hashtable.h"

namespace Editor
{

bool SaveEntities(const char* uri);

class EntityLoader : public BaseGameFeature::LevelParser
{
    __DeclareClass(EntityLoader)

public:
    EntityLoader();
    ~EntityLoader();

    /// Generate new persistent GUIDs instead of retaining GUIDs from the source level.
    void SetGenerateGuids(bool generate);
    /// Load editor-only collection data after the level entities have been parsed.
    void LoadCollections(const Ptr<IO::JsonReader>& reader);

private:
    /// called at beginning of load
    virtual void BeginLoad() override;
    /// add entity
    virtual void AddEntity(Game::Entity entity, Util::Guid const& guid) override;
    /// set entity name
    virtual void SetName(Game::Entity entity, const Util::String& name) override;
    /// entity loaded completely
    virtual void CommitEntity(Game::Entity entity) override;
    /// parsing done
    virtual void CommitLevel() override;

    bool generateGuids = false;
    Util::HashTable<Util::Guid, Game::Entity> sourceEntities;
    Util::Array<Game::Entity> loadedEntities;
};

} // namespace Edit
