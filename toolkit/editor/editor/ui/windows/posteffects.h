#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::PostEffectSettings

    Post effect settings. Bloom is the first group.

    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"
#include "resources/resourceid.h"

namespace Presentation
{

class PostEffectSettings : public BaseWindow
{
public:
    PostEffectSettings();
    ~PostEffectSettings();

    void Run(SaveMode save) override;

private:
    Resources::ResourceId bloomDirt;
};

} // namespace Presentation
