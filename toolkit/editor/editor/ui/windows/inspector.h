#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::Inspector

    Property inspector

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"
#include "editor/editor.h"

namespace Presentation
{

class Inspector : public BaseWindow
{
    __DeclareClass(Inspector)
public:
    Inspector();
    ~Inspector();

    void Update();
    void Run();

    void ShowAddPropertyMenu();

private:
    struct IntermediateProperties
    {
        bool isDirty = false;
        void* buffer = nullptr;
        SizeT bufferSize = 0;
    };
    Util::Array<IntermediateProperties> tempProperties;
    Editor::Entity latestInspectedEntity;
};
__RegisterClass(Inspector)

} // namespace Presentation

