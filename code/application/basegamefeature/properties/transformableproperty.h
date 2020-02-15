#pragma once
//------------------------------------------------------------------------------
/**
    Game::TransformableProperty

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/property.h"
#include "game/database/attribute.h"

namespace Attr
{
__DeclareAttribute(LocalTransform, Math::matrix44, 'Lm44', Math::matrix44::identity());
__DeclareAttribute(WorldTransform, Math::matrix44, 'Wm44', Math::matrix44::identity());
__DeclareAttribute(Parent, Game::Entity , 'TFPT', Game::Entity::Invalid());
}

namespace Game
{

class TransformableProperty : public Game::Property
{
    __DeclareClass(TransformableProperty);
public:
    TransformableProperty();
    ~TransformableProperty();

    void SetupExternalAttributes() override;
};

} // namespace Game
