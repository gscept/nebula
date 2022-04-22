#pragma once
//------------------------------------------------------------------------------
/**
    Viewport module

    Shows a viewport, including rendering settings, camera settings etc.

    (C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/tools/camera.h"

namespace Graphics{ class Stage; class View; }

namespace Presentation
{

namespace Modules
{

class Viewport
{
public:
    enum RenderMode
    {
        TexturedLit,
        TexturedUnlit,
        SolidLit,
        SolidUnlit,
        Wireframe,
    };

    Viewport();
    ~Viewport();
    
    void Init(Util::String const& viewName);
    void Init(Ptr<Graphics::View> const& view);

    void Render();
    void SetStage(Ptr<Graphics::Stage> const& stage);
    void SetFrameBuffer(Util::String const& name);

    void Update()
    {
        if (this->hovered)
        {
            camera.Update();
            this->hovered = false;
        }

    }

    void SetHovered(bool val)
    {
        this->hovered = val;
    }

    bool IsHovered() const
    {
        return this->hovered;
    }

private:
    RenderMode renderMode = TexturedLit;

    Ptr<Graphics::Stage> stage;
    Ptr<Graphics::View> view;

    Resources::ResourceId resourceId;

    Editor::Camera camera;
    bool hovered = false;

    Util::String frameBuffer;
};

} // namespace Modules

} // namespace Presentation
