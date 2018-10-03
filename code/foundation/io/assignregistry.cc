//------------------------------------------------------------------------------
//  assignregistry.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/assignregistry.h"
#include "io/fswrapper.h"
#include "core/coreserver.h"

namespace IO
{
__ImplementClass(IO::AssignRegistry, 'ASRG', Core::RefCounted);
__ImplementInterfaceSingleton(IO::AssignRegistry);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
AssignRegistry::AssignRegistry() :
    isValid(false)
{
    __ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
AssignRegistry::~AssignRegistry()
{
    if (this->IsValid())
    {
        this->Discard();
    }
    __DestructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
AssignRegistry::Setup()
{
    this->critSect.Enter();

    n_assert(!this->IsValid());
    this->isValid = true;
    this->SetupSystemAssigns();
    this->SetupProjectAssigns();
    
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
AssignRegistry::Discard()
{
    this->critSect.Enter();
    
    n_assert(this->IsValid());
    this->assignTable.Clear();
    this->isValid = false;
    
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
bool
AssignRegistry::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
void
AssignRegistry::SetupSystemAssigns()
{
    this->critSect.Enter();

    n_assert(this->IsValid()); 
    
    String homeLocation = FSWrapper::GetHomeDirectory();
    if (homeLocation.IsValid())
    {
        this->SetAssign(Assign("home", homeLocation));
    }
    #if !(__WII__ || __PS3__)
    String binLocation = FSWrapper::GetBinDirectory();
    if (binLocation.IsValid())
    {
        this->SetAssign(Assign("bin", binLocation));
    }
    String tempLocation = FSWrapper::GetTempDirectory();
    if (tempLocation.IsValid())
    {
        this->SetAssign(Assign("temp", tempLocation));
    }
    String userLocation = FSWrapper::GetUserDirectory();
    if (userLocation.IsValid())
    {
        this->SetAssign(Assign("user", userLocation));
    }
    #endif
    
    #if __WIN32__
    String appDataLocation = FSWrapper::GetAppDataDirectory();
    if (appDataLocation.IsValid())
    {
        this->SetAssign(Assign("appdata", appDataLocation));
    }
    String programsLocation = FSWrapper::GetProgramsDirectory();
    if (programsLocation.IsValid())
    {
        this->SetAssign(Assign("programs", programsLocation));
    }
    #endif

    this->critSect.Leave();
}    

//------------------------------------------------------------------------------
/**
*/
void
AssignRegistry::SetupProjectAssigns()
{
    this->critSect.Enter();
        
    n_assert(this->IsValid());

    String rootDir = Core::CoreServer::Instance()->GetRootDirectory().AsString();
    this->SetAssign(Assign("root", rootDir));
    #if __WIN32__
        this->SetAssign(Assign("export", "root:export_win32"));
    #elif __XBOX360__
        this->SetAssign(Assign("export", "root:export_xbox360"));
    #elif __WII__
        this->SetAssign(Assign("export", "root:export_wii"));
    #elif __PS3__
        this->SetAssign(Assign("export", "root:export_ps3"));
    #elif __OSX__
        this->SetAssign(Assign("export", "root:export_osx"));
    #elif __linux__
        this->SetAssign(Assign("export", "root:export_linux"));
    #else
    #error "PLATFORM FIXME: setup platform specific assigns!"
    #endif

    // setup content assigns
    this->SetAssign(Assign("msh", "export:meshes"));
    this->SetAssign(Assign("ani", "export:anims"));
    this->SetAssign(Assign("data", "export:data"));        
    this->SetAssign(Assign("video", "export:video"));
    this->SetAssign(Assign("db", "export:db"));
    this->SetAssign(Assign("seq", "export:sequences"));
    this->SetAssign(Assign("audio", "export:audio"));
    this->SetAssign(Assign("stream", "export:audio"));
    this->SetAssign(Assign("shd", "export:shaders"));
    this->SetAssign(Assign("tex", "export:textures"));
    this->SetAssign(Assign("frame", "export:frame"));
    this->SetAssign(Assign("mdl", "export:models"));
    this->SetAssign(Assign("audio", "export:audio"));
    this->SetAssign(Assign("phys", "export:physics"));
    this->SetAssign(Assign("sui", "export:sui"));       
    this->SetAssign(Assign("wiidata", "export:wiidata"));   
    this->SetAssign(Assign("mat", "export:materials"));
	this->SetAssign(Assign("sur", "export:surfaces"));
    this->SetAssign(Assign("scr", "root:data/scripts"));
    this->SetAssign(Assign("gui", "root:data/gui"));

    // setup special assigns which may use original meshes but still needs an explicit load
    this->SetAssign(Assign("parmsh", "export:meshes"));

    // assign for nav meshes
    this->SetAssign(Assign("nav", "root:data/navigation"));

    // setup special system assigns (for placeholders, etc...)
    this->SetAssign(Assign("sysmsh", "export:meshes"));
    this->SetAssign(Assign("systex", "export:textures"));
    this->SetAssign(Assign("sysanim", "export:anims"));

    // Nebula2 backward compat assigns:
    this->SetAssign(Assign("meshes", "export:meshes"));
    this->SetAssign(Assign("anims", "export:anims"));
    this->SetAssign(Assign("textures", "export:textures"));

    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
void
AssignRegistry::SetAssign(const Assign& assign)
{
    this->critSect.Enter();

    n_assert(this->IsValid());
    if (this->HasAssign(assign.GetName()))
    {
        this->ClearAssign(assign.GetName());
    }
    this->assignTable.Add(assign);
    
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
bool
AssignRegistry::HasAssign(const String& assignName) const
{
    this->critSect.Enter();
    n_assert(assignName.IsValid());
    bool result = this->assignTable.Contains(assignName);
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
String
AssignRegistry::GetAssign(const String& assignName) const
{
    this->critSect.Enter();

    n_assert(assignName.IsValid());
    n_assert(this->HasAssign(assignName));
    String result = this->assignTable[assignName];
    
    this->critSect.Leave();
    return result;
}

//------------------------------------------------------------------------------
/**
*/
void
AssignRegistry::ClearAssign(const String& assignName)
{
    this->critSect.Enter();

    n_assert(assignName.IsValid());
    n_assert(this->HasAssign(assignName));
    this->assignTable.Erase(assignName);

    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
Array<Assign>
AssignRegistry::GetAllAssigns() const
{
    this->critSect.Enter();

    ArrayStack<KeyValuePair<String,String>, 1> content = this->assignTable.Content();
    Array<Assign> assigns(content.Size(), 0);
    IndexT i;
    for (i = 0; i < content.Size(); i++)
    {
        assigns.Append(Assign(content[i].Key(), content[i].Value()));
    }

    this->critSect.Leave();
    return assigns;
}

//------------------------------------------------------------------------------
/**
    Resolves all assigns from an URI returning an URI. It is allowed to
    "stack" assigns, which means, defining an assign as pointing to
    another assign.
*/
URI
AssignRegistry::ResolveAssigns(const URI& uri) const
{
    this->critSect.Enter();
    URI resolvedUri = this->ResolveAssignsInString(uri.AsString());
    this->critSect.Leave();
    return resolvedUri;
}

//------------------------------------------------------------------------------
/**
    Resolves all assigns from a URI. It is allowed to
    "stack" assigns, which means, defining an assign as pointing to
    another assign.s
*/
String
AssignRegistry::ResolveAssignsInString(const String& uriString) const
{
    this->critSect.Enter();
    String result = uriString;

    // check for assigns
    int colonIndex;
    while ((colonIndex = result.FindCharIndex(':', 0)) > 0)
    {
        // special case: ignore one-caracter "assigns" because they are 
        // really DOS drive letters
        if (colonIndex > 1)
        {
            String assignString = result.ExtractRange(0, colonIndex);
            
            // ignore unknown assigns, because these may be part of an URI
            if (this->HasAssign(assignString))
            {
                String postAssignString = result.ExtractRange(colonIndex + 1, result.Length() - (colonIndex + 1));
                String replace = this->GetAssign(assignString);
                if (!replace.IsEmpty())
                {
                    if (replace[replace.Length()-1] != ':'
                        && (replace[replace.Length()-1] != '/'
                        || replace[replace.Length()-2] != ':'))
                    {
                        replace.Append("/");
                    }
                    replace.Append(postAssignString);
                }
                result = replace;
            }
            else break;
        }
        else break;
    }
    result.ConvertBackslashes();
    result.TrimRight("/");
    this->critSect.Leave();
    return result;
}

} // namespace IO