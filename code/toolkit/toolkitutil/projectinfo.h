#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ProjectInfo
  
    Reads the projectinfo.xml file in the projects root directory and 
    exposes its content through a C++ class.
    
    Setup() performs the following steps:
    
    - query the "proj:" path from the Win32 registry
    - read the "proj:projectinfo.xml" file
    - setup assigns "src:" and "dst:"
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "toolkitutil/platform.h"
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

    /// get the default platform which has been defined in the project info
    Platform::Code GetDefaultPlatform() const;
    /// return true if a platform exists in the project info
    bool HasPlatform(Platform::Code platform) const;
    /// set the project info object to a specific platform
    void SetCurrentPlatform(Platform::Code platform);
    /// get currently selected platform
    Platform::Code GetCurrentPlatform() const;
    
    /// return true if a platform attribute exists
    bool HasAttr(const Util::String& attrName) const;
    /// get platform attribute value
    const Util::String& GetAttr(const Util::String& attrName) const;
    /// get platform attribute as path (resolves env- and reg-variables)
    Util::String GetPathAttr(const Util::String& attrName) const;
    /// create a projectinfo object directly from a path
    static ProjectInfo SetupFromFile(const Util::String & path);

private:
    /// query registry for proj: path and setup assign in IO server
    Util::String QueryProjectPathFromRegistry();
    /// query registry for the toolkit directory
    Util::String QueryToolkitPathFromRegistry();
    /// parse the project info XML file
    Result ParseProjectInfoFile(const IO::URI & path);

    Platform::Code defPlatform;
    Platform::Code curPlatform;
    Util::Dictionary<Platform::Code, Util::Dictionary<Util::String,Util::String>> platformAttrs;
};

//------------------------------------------------------------------------------
/**
*/
inline Platform::Code
ProjectInfo::GetDefaultPlatform() const
{
    return this->defPlatform;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
ProjectInfo::HasPlatform(Platform::Code platform) const
{
    return this->platformAttrs.Contains(platform);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ProjectInfo::SetCurrentPlatform(Platform::Code platform)
{
    this->curPlatform = platform;
}

//------------------------------------------------------------------------------
/**
*/
inline Platform::Code
ProjectInfo::GetCurrentPlatform() const
{
    return this->curPlatform;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
