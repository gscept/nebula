#pragma once
//------------------------------------------------------------------------------
/**
    @class  Presentation::AssetBrowser

    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "editor/ui/window.h"

namespace Presentation
{

class Previewer;
class AssetBrowser : public BaseWindow
{
    __DeclareClass(AssetBrowser)
public:
    AssetBrowser();
    ~AssetBrowser();

    void Update();
    void Run();
private:
    void DisplayFileTree();

    Ptr<Previewer> previewer;
};
__RegisterClass(AssetBrowser)

} // namespace Presentation

