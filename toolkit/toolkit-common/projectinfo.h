#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ProjectInfo
  
    Reads the projectinfo.json file in the projects root directory and 
    exposes its content through a C++ class.
    
    Setup() performs the following steps:
    
    - query the "proj:" path from the Win32 registry
    - read the "proj:projectinfo.json" file
    - setup assigns "src:" and "dst:" 
    - add assigns from th projectinfo
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "platform.h"
#include "util/dictionary.h"
#include "io/uri.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class ProjectInfo
{
public:
    /// error code enums
    enum Result
    {
        Success = 1,

        NoProjPathInRegistry,
        NoProjectInfoFile,
        CouldNotOpenProjectInfoFile,
        ProjectFileContentInvalid,
    };

    /// constructor
    ProjectInfo();
    /// destructor
    ~ProjectInfo();
    
    /// setup the project info object
    Result Setup();
    /// discard the project info object
    void Discard();
    /// return true if object has been setup
    bool IsValid() const;
    /// get error string for result code
    Util::String GetErrorString(Result res) const;

    
    
    /// return true if a platform attribute exists
    bool HasAttr(const Util::String& attrName) const;
    /// get platform attribute value
    const Util::String& GetAttr(const Util::String& attrName) const;

    /// return true if a list attribute exists
    bool HasListAttr(const Util::String& attrName) const;
    ///
    const Util::Dictionary<Util::String, Util::String>& GetListAttr(const Util::String& attrName) const;
    /// get platform attribute as path (resolves env- and reg-variables)
    Util::String GetPathAttr(const Util::String& attrName) const;
    /// create a projectinfo object directly from a path
    static ProjectInfo SetupFromFile(const Util::String & path);

private:
    /// query registry for proj: path and setup assign in IO server
    Util::String QueryProjectPathFromRegistry();
    /// query registry for the toolkit directory
    Util::String QueryToolkitPathFromRegistry();
    /// parse the project info json file
    Result ParseProjectInfoFile(const IO::URI & path);
    /// apply all assigns from the projectinfo file (non-list keys)
    void AddAttrAssigns();
    
    Util::Dictionary<Util::String, Util::String> attrs;
    Util::Dictionary<Util::String, Util::Dictionary<Util::String, Util::String>> listAttrs;
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
