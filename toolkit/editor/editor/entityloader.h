#pragma once
//------------------------------------------------------------------------------
/**
    @file entityloader.h

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "basegamefeature/levelparser.h"

namespace Editor
{

bool SaveEntities(const char* uri);

class EntityLoader : public BaseGameFeature::LevelParser
{
    __DeclareClass(EntityLoader)

public:
    EntityLoader();
    ~EntityLoader();

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
};

} // namespace Edit
