#pragma once
//------------------------------------------------------------------------------
/**
    Game::TransformableProperty

    Handles entities that can be parented to each other, and that should move with each other.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/property.h"
#include "game/database/attribute.h"
#include "basegamefeature/managers/entitymanager.h"

namespace Attr
{
__DeclareAttribute(LocalTransform, AccessMode::ReadWrite, Math::mat4, 'Lm44', Math::mat4());
__DeclareAttribute(WorldTransform, AccessMode::ReadWrite, Math::mat4, 'Wm44', Math::mat4());
__DeclareAttribute(Parent, AccessMode::ReadOnly, Game::Entity , 'TFPT', Game::Entity::Invalid());
}

namespace Game
{

class TransformableProperty : public Game::Property
{
    __DeclareClass(TransformableProperty);
public:
    TransformableProperty();
    ~TransformableProperty();

    void Init() override;

    void OnActivate(Game::InstanceId instance) override;
    void OnDeactivate(Game::InstanceId instance) override;

    void SetupExternalAttributes() override;
private:
    struct Data 
    {
        Game::PropertyData<Game::Entity> owner;
        Game::PropertyData<Math::mat4> localTransform;
        Game::PropertyData<Math::mat4> worldTransform;
        Game::PropertyData<Game::Entity> parent;
    } data;
};

} // namespace Game
