#pragma once
//------------------------------------------------------------------------------
/**
    @class TookitUtil::AssetConverterApp
    
    Assert converter app
    
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "toolkit-common/toolkitapp.h"
#include "modelutil/modeldatabase.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class AssetConverterApp : public ToolkitUtil::ToolkitApp
{

public:
    /// constructor
    AssetConverterApp();
    /// destructor
    virtual ~AssetConverterApp();

    /// run the application
    virtual void Run();

private:
    /// print help text
    void ShowHelp();

    Ptr<ToolkitUtil::ModelDatabase> modelDatabase;
}; 
} // namespace Tookit
//------------------------------------------------------------------------------