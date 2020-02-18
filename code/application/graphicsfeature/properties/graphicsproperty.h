#pragma once
//------------------------------------------------------------------------------
/**
    GraphicsFeature::GraphicsProperty

    Handles entities that should be rendered using some model resource.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "game/property.h"
#include "game/database/attribute.h"
#include "graphics/graphicsentity.h"

namespace Attr
{
__DeclareAttribute(ModelResource, Util::String, 'MdlR', Util::String("mdl:system/placeholder.n3"));
}

namespace GraphicsFeature
{

class GraphicsProperty : public Game::Property
{
    __DeclareClass(GraphicsProperty);
public:
    GraphicsProperty();
    ~GraphicsProperty();

    void Init() override;

    void OnActivate(Game::InstanceId instance) override;
    void OnDeactivate(Game::InstanceId instance) override;
    void OnBeginFrame() override;

    void SetupExternalAttributes() override;
private:
    struct State
    {
        Graphics::GraphicsEntityId gfxEntity;
    };

    struct Data
    {
        Game::PropertyData<State> state;
        Game::PropertyData<Math::matrix44> worldTransform;
        Game::PropertyData<Util::String> modelResource;
    } data;
};

} // namespace GraphicsFeature
