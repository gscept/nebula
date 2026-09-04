#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Inspector

    Property inspector

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "editor/ui/window.h"
#include "editor/editor.h"

namespace Presentation
{

class Inspector : public BaseWindow
{
public:
    Inspector();
    ~Inspector();

    void Update();
    void Run(SaveMode save) override;

    void ShowAddComponentMenu();

private:
    struct IntermediateComponents
    {
        bool isDirty = false;
        void* buffer = nullptr;
        SizeT bufferSize = 0;
    };
    Util::Array<IntermediateComponents> tempComponents;
    Editor::Entity latestInspectedEntity;
    uint64_t latestEntityVersion;
};

} // namespace Presentation

