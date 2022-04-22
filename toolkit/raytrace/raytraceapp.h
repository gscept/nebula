#pragma once
//------------------------------------------------------------------------------
/**
    Main function for raytracer application
    
    (C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkitapp.h"
#include "objectcontext.h"
#include "projectinfo.h"

namespace Toolkit
{
class RaytraceApp : public ToolkitUtil::ToolkitApp
{
public:
    /// constructor
    RaytraceApp();
    /// destructor
    virtual ~RaytraceApp();

    /// open the application
    bool Open();
    /// close the application
    void Close();
private:

    /// print help text
    void ShowHelp();

    /// load single mesh into raytracer
    Ptr<ObjectContext> LoadMesh(const IO::URI& mesh);
    /// load entire level into raytracer
    void LoadLevel(const IO::URI& level);

    Util::Array<Ptr<ObjectContext>> objects;
};
} // namespace Toolkit