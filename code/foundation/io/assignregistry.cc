//------------------------------------------------------------------------------
//  assignregistry.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "io/assignregistry.h"
#include "io/fswrapper.h"
#include "io/ioserver.h"
#include "core/coreserver.h"
#include "system/nebulasettings.h"
#include "db/sqlite3/sqlite3factory.h"
#include "db/dataset.h"

namespace Attr
{
DefineAttrInt(URNHash, 'FURN', Attr::ReadWrite);
DefineAttrInt(WorkHash, 'FWRK', Attr::ReadWrite);
DefineAttrString(Export, 'FEXP', Attr::ReadWrite);
DefineAttrString(Work, 'FWOR', Attr::ReadWrite);
}


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

    // Read the resource table and populate URN -> URI lookup map
    IO::URI resTableUri("export:resource_mappings.sqlite");

    if (!Db::Sqlite3Factory::HasInstance())
    {
        this->dbFactory = Db::Sqlite3Factory::Create();
    }
    this->database = Db::DbFactory::Instance()->CreateDatabase();

    // Determine access mode
    IO::IoServer* ioServer = IO::IoServer::Instance();

    this->database->SetURI(resTableUri);
    this->database->SetAccessMode(Db::Database::ReadWriteCreate);
    this->database->SetIgnoreUnknownColumns(false);

    // Use memory database if we're not in editor as it can modify the database
#if WITH_NEBULA_EDITOR == 0
    this->database->SetInMemoryDatabase(true);
#endif

    // Try to open database
    if (!this->database->Open())
    {
        n_printf("Failed to open resource lookup database at %s\n", resTableUri.AsString().AsCharPtr());
    }
    else
    {
        if (!this->database->HasTable("Mappings"))
        {
            // Close database, it's invalid
            this->database = nullptr;
        }
        else
        {
            this->mappings = this->database->GetTableByName("Mappings");
        }
    }
    
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
    this->SetAssign(Assign("export", "root:export"));
    
    // setup content assigns
    this->SetAssign(Assign("msh",   "export:meshes"));
    this->SetAssign(Assign("ani",   "export:anims"));
    this->SetAssign(Assign("ske",   "export:skeletons"));
#if PUBLIC_BUILD
    this->SetAssign(Assign("data",  "export:data"));        
#else
    this->SetAssign(Assign("data", "root:data"));
#endif
    this->SetAssign(Assign("video", "export:video"));
    this->SetAssign(Assign("db",    "export:db"));
    this->SetAssign(Assign("audio", "export:audio"));
    this->SetAssign(Assign("shd",   "export:shaders"));
    this->SetAssign(Assign("tex",   "export:textures"));
    this->SetAssign(Assign("frame", "export:frame"));
    this->SetAssign(Assign("mdl",   "export:models"));
    this->SetAssign(Assign("phys",  "export:physics"));
    this->SetAssign(Assign("audio", "export:audio"));    
    this->SetAssign(Assign("mat",   "export:materials"));
    this->SetAssign(Assign("sur",   "export:surfaces"));
    this->SetAssign(Assign("par",   "export:particles"));
    this->SetAssign(Assign("scr",   "data:scripts"));
    this->SetAssign(Assign("gui",   "data:gui"));
    this->SetAssign(Assign("tbl", "export:data/tables"));
    
    // assign for nav meshes
    this->SetAssign(Assign("nav",   "data:navigation"));

    // setup special system assigns (for placeholders, etc...)
    this->SetAssign(Assign("sysmsh",    "msh:system"));
    this->SetAssign(Assign("systex",    "tex:system"));
    this->SetAssign(Assign("sysani",    "ani:system"));
    this->SetAssign(Assign("sysmdl",    "mdl:system"));
    this->SetAssign(Assign("syssur",    "sur:system"));
    this->SetAssign(Assign("sysphys",   "phys:system"));

    this->SetAssign(Assign("config", "data:config"));

    // tools assigns
    this->SetAssign(Assign("tool", Core::CoreServer::Instance()->GetToolDirectory().AsString()));    
    // project assign from registry
    if (System::NebulaSettings::Exists("gscept", "ToolkitShared", "workdir"))
    {
        this->SetAssign(Assign("proj", System::NebulaSettings::ReadString("gscept", "ToolkitShared", "workdir")));
    }
    else
    {
        this->SetAssign(Assign("proj", "root:"));
    }
    

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

    StackArray<KeyValuePair<String,String>, 1> content = this->assignTable.Content();
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

//------------------------------------------------------------------------------
/**
*/
void
AssignRegistry::PrintAll() const
{
    this->critSect.Enter();
    auto keys = this->assignTable.KeysAsArray();
    for (int i = 0; i < keys.Size(); i++)
    {
        Util::String assign = this->assignTable[keys[i]];
        Util::String path = IO::URI(this->assignTable[keys[i]]).LocalPath();
        n_printf("%-10s  =>   %-24s  =>  %s\n", (keys[i] + ":").AsCharPtr(), assign.AsCharPtr(), path.AsCharPtr());
    }
    this->critSect.Leave();
}

//------------------------------------------------------------------------------
/**
*/
IO::URI
AssignRegistry::ResolveWorkToExport(const IO::URI& uri) const
{
    Ptr<Db::Dataset> set = this->mappings->CreateDataset();
    uint32_t hash = uri.LocalPath().HashCode();
    set->Filter()->AddEqualCheck(Attr::Attribute(Attr::WorkHash, hash));
    set->PerformQuery();
    Ptr<Db::ValueTable> workValues = set->Values();
    n_assert(workValues->GetNumRows() <= 1);
    if (workValues->GetNumRows() == 1)
    {
        return IO::URI(workValues->GetString(Attr::Export, 0));
    }
    return IO::URI();
}

//------------------------------------------------------------------------------
/**
*/
IO::URI
AssignRegistry::ResolveURNToWork(const IO::URN& urn) const
{
    Ptr<Db::Dataset> set = this->mappings->CreateDataset();
    set->Filter()->AddEqualCheck(Attr::Attribute(Attr::URNHash, (urn.AsString()).HashCode()));
    set->PerformQuery();
    Ptr<Db::ValueTable> workValues = set->Values();
    n_assert(workValues->GetNumRows() <= 1);
    if (workValues->GetNumRows() == 1)
    {
        return IO::URI(workValues->GetString(Attr::Work, 0));
    }
    return IO::URI();
}

//------------------------------------------------------------------------------
/**
*/
IO::URI
AssignRegistry::ResolveURNToExport(const IO::URN& urn) const
{
    Ptr<Db::Dataset> set = this->mappings->CreateDataset();
    set->Filter()->AddEqualCheck(Attr::Attribute(Attr::URNHash, (urn.AsString()).HashCode()));
    set->PerformQuery();
    Ptr<Db::ValueTable> workValues = set->Values();
    n_assert(workValues->GetNumRows() <= 1);
    if (workValues->GetNumRows() == 1)
    {
        return IO::URI(workValues->GetString(Attr::Export, 0));
    }
    return IO::URI();
}

} // namespace IO
