//------------------------------------------------------------------------------
//  projectinfo.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "projectinfo.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/jsonreader.h"
#include "io/projectlocator.h"
#include "system/nebulasettings.h"
#include "system/environment.h"
#include "app/application.h"
#include "util/commandlineargs.h"


namespace ToolkitUtil
{
using namespace Util;
using namespace IO;
using namespace System;

//------------------------------------------------------------------------------
/**
*/
ProjectInfo::ProjectInfo() 
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ProjectInfo::~ProjectInfo()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
ProjectInfo::HasAttr(const Util::String& attrName) const
{
    return this->attrs.Contains(attrName);
}
//------------------------------------------------------------------------------
/**
*/
bool
ProjectInfo::HasListAttr(const Util::String& attrName) const
{
    return this->listAttrs.Contains(attrName);
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
ProjectInfo::GetAttr(const Util::String& attrName) const
{
    n_assert(attrName.IsValid());

    if (!this->HasAttr(attrName))
    {
        n_error("ProjectInfo: attr '%s' does not exist in projectinfo!",
            attrName.AsCharPtr());
    }
    return this->attrs[attrName];
}

//------------------------------------------------------------------------------
/**
*/
const Util::Dictionary<Util::String, Util::String>&
ProjectInfo::GetListAttr(const Util::String& attrName) const
{
    n_assert(attrName.IsValid());

    if (!this->HasListAttr(attrName))
    {
        n_error("ProjectInfo: attr '%s' does not exist in projectinfo!",
            attrName.AsCharPtr());
    }
    return this->listAttrs[attrName];
}

//------------------------------------------------------------------------------
/**
*/
bool
ProjectInfo::IsValid() const
{
    return (this->attrs.Size() > 0);
}

//------------------------------------------------------------------------------
/**
*/
ProjectInfo::Result
ProjectInfo::Setup()
{
    n_assert(!this->IsValid());

    // check for cmdline overrides for project
    const Util::CommandLineArgs & args = App::Application::Instance()->GetCmdLineArgs();

    // the project info file always lives in the project root, so the project
    // root has to be resolved before the file can be read
    String projPath = args.HasArg("-workdir") ? args.GetString("-workdir") : ProjectLocator::GetProjectRoot();
    if (projPath.IsEmpty())
    {
        return NoProjectRoot;
    }
    AssignRegistry::Instance()->SetAssign(Assign("proj", projPath));

    // same for the toolkit, first overrides, then the build system value
    String toolkitPath = args.HasArg("-toolkit") ? args.GetString("-toolkit") : ProjectLocator::GetToolkitRoot();
    if (toolkitPath.IsEmpty())
    {
        toolkitPath = "home:";
    }
    AssignRegistry::Instance()->SetAssign(Assign("toolkit", toolkitPath));

    // parse the project info file
    Util::String projectPath = String("proj:") + String(ProjectLocator::ProjectInfoFileName);
    if (args.HasArg("-project"))
    {
        // the argument is normally the project directory, but a path directly
        // to the project info file is also accepted
        String projectArg = args.GetString("-project");
        if (IoServer::Instance()->FileExists(projectArg))
        {
            projectPath = projectArg;
            projPath = projectPath.ExtractToLastSlash();
            AssignRegistry::Instance()->SetAssign(Assign("proj", projPath));
        }
        else
        {
            projectPath = String::AppendPath(projectArg, String(ProjectLocator::ProjectInfoFileName));
            AssignRegistry::Instance()->SetAssign(Assign("proj", projectArg));
        }
    }

    Result res = ProjectInfo::Success;
    if (IoServer::Instance()->FileExists(projectPath))
    {
        res = this->ParseProjectInfoFile(URI(projectPath));
        this->AddAttrAssigns();
    }
    else
    {
        return NoProjectInfoFile;
    }
    return res;
}

//------------------------------------------------------------------------------
/**
*/
void
ProjectInfo::AddAttrAssigns()
{
    for (auto const& attr : this->attrs)
    {
        AssignRegistry::Instance()->SetAssign(Assign(attr.Key(), attr.Value()));
    }
}
//------------------------------------------------------------------------------
/**
*/
void
ProjectInfo::Discard()
{
    this->attrs.Clear();
    this->listAttrs.Clear();
}

//------------------------------------------------------------------------------
/**
    Parse the projectinfo.xml file which is expected in the project root 
    directory.
*/
ProjectInfo::Result
ProjectInfo::ParseProjectInfoFile(const IO::URI & path)
{
    n_assert(!this->IsValid());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(path);
    Ptr<JsonReader> jsonReader = JsonReader::Create();
    jsonReader->SetStream(stream);
    if (jsonReader->Open())
    {

        if (jsonReader->SetToFirstChild()) do
        {
            String thing = jsonReader->GetCurrentNodeName();
            if (jsonReader->HasChildren())
            {
                String currentKey = jsonReader->GetCurrentNodeName();
                Dictionary<String, String> values;
                jsonReader->SetToFirstChild();
                do
                {
                    values.Add(jsonReader->GetString("Name"), jsonReader->GetString("Value"));
                } 
                while (jsonReader->SetToNextChild());
                this->listAttrs.Add(currentKey, values);
            }
            else if (jsonReader->IsString())
            {
                this->attrs.Add(jsonReader->GetCurrentNodeName(), jsonReader->GetString());
            }
        } 
        while (jsonReader->SetToNextChild());
        return Success;
    }
    else
    {
        return CouldNotOpenProjectInfoFile;
    }
}

//------------------------------------------------------------------------------
/**
*/
String
ProjectInfo::GetErrorString(Result res) const
{
    switch (res)
    {
        case ProjectInfo::NoProjectRoot:
            return String("ERROR: could not determine the project root! Set NEBULA_PROJECT, pass -workdir, or run from inside a project directory!");
        case ProjectInfo::CouldNotOpenProjectInfoFile:
            return String("ERROR: could not open projectinfo.json file in project directory!");
        case ProjectInfo::ProjectFileContentInvalid:
            return String("ERROR: content of projectinfo.json is invalid!");
        case ProjectInfo::NoProjectInfoFile:
            return String("ERROR: No projectinfo.json file found in current project!");
        default:
            return String("ERROR: unknown error from ProjectInfo object!\n");
    }
}

//------------------------------------------------------------------------------
/**
    Interprets an attribute value as a path which may contain registry
    or environment aliases in the following form:

    $(reg:HKEY_LOCAL_MACHINE/bla/blub)/bla/bla
    $(env:HOME_DIR)/bla/bla
*/
String
ProjectInfo::GetPathAttr(const String& attrName) const
{
    String attrVal = this->GetAttr(attrName);
    if ((attrVal.Length() > 0) && (attrVal[0] == '$'))
    {
        // need to resolve env or reg variable
        IndexT varStartIndex = attrVal.FindCharIndex('(');
        IndexT varEndIndex   = attrVal.FindCharIndex(')');
        n_assert(varStartIndex == 1);
        n_assert(varEndIndex > varStartIndex);
        String varString = attrVal.ExtractRange(varStartIndex + 1, (varEndIndex - varStartIndex) - 1);

#if __WIN32__
        // registry or environment variable?
        if (String::MatchPattern(varString, "reg:*"))
        {
            // treat as registry key
            String regString = varString.ExtractToEnd(4);
            Array<String> regTokens = regString.Tokenize("/");
            n_assert(regTokens.Size() > 2);
            Win32::Win32Registry::RootKey rootKey = Win32::Win32Registry::AsRootKey(regTokens[0]);
            String regKeyName = regTokens.Back();
            regTokens.EraseIndex(0);
            regTokens.EraseIndex(regTokens.Size() - 1);
            String regPath = String::Concatenate(regTokens, "\\");

            // read actual registry value
            String regValue = Win32::Win32Registry::ReadString(rootKey, regPath, regKeyName);

            // build actual file name
            String retValue = regValue;
            retValue.Append(attrVal.ExtractToEnd(varEndIndex + 1));
            return retValue;
        }
        else if (String::MatchPattern(varString, "env:*"))
        {
            // an environment variable
            String envString = varString.ExtractToEnd(4);
            if (Win32::Win32Environment::Exists(envString))
            {
                String envValue = Win32::Win32Environment::Read(envString);
                String retValue = envValue;
                retValue.Append(attrVal.ExtractToEnd(varEndIndex + 1));
                return retValue;
            }
        }
#endif
    }
    
    // fallthrough: format error, return string as is
    return attrVal;
}

//------------------------------------------------------------------------------
/**    
*/
ProjectInfo
ProjectInfo::SetupFromFile(const String& file)
{
    ProjectInfo pi;
    URI uri(file);
    if(IO::IoServer::Instance()->FileExists(uri))
    {
        pi.ParseProjectInfoFile(uri);
    }
    return pi;
}

} // namespace ToolkitUtil