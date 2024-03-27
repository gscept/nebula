//------------------------------------------------------------------------------
//  levelexporter.cc
//  (C) 2012-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "game/levelexporter.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
#include "game/world.h"
#include "basegamefeature/levelparser.h"

using namespace IO;
using namespace Util;
using namespace MemDb;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::LevelExporter, 'LEXP', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
LevelExporter::LevelExporter()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
LevelExporter::~LevelExporter()
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::Open()
{
    ExporterBase::Open();
}


//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::Close()
{
    ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::ExportAll()
{
    String levelDir = "proj:work/levels";
    Array<String> files = IoServer::Instance()->ListFiles(IO::URI(levelDir), "*.json");
    for (int fileIndex = 0; fileIndex < files.Size(); fileIndex++)
    {
        this->ExportFile(levelDir + "/" + files[fileIndex]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::ExportDir( const String& category )
{
    /// fall-through, we want to overload base class but we don't care about category
    this->ExportAll();
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelExporter::ExportFile( const URI& file )
{
    n_assert(this->isOpen);
   
    Ptr<Stream> levelStream = IoServer::Instance()->CreateStream(file);
    Ptr<JsonReader> jsonReader = JsonReader::Create();
    
    if(!levelStream->Open())
    {        
        this->logger->Error("Could not open level: %s\n",file.GetHostAndLocalPath().AsCharPtr());
        this->SetHasErrors(true);
        return;
    }
    if(!levelStream->GetSize())
    {
        this->logger->Error("Empty level file: %s\n", file.GetHostAndLocalPath().AsCharPtr());
        this->SetHasErrors(true);
        return;
    }
    jsonReader->SetStream(levelStream);
    if (!jsonReader->Open())
    {
        this->logger->Error("Could not open level: %s\n", file.GetHostAndLocalPath().AsCharPtr());
        this->SetHasErrors(true);
        return;
    }
    
    if (jsonReader->HasNode("/level"))
    {
        this->Progress(5, file.AsString());
        this->logger->Print("Exporting: %s\n", file.GetHostAndLocalPath().ExtractFileName().AsCharPtr());
       
        Game::World* world = new Game::World('TEMP');
        this->ExportLevel(jsonReader, world, file);
        delete world;
    }
    else
    {
        n_printf("Level: %s is either corrupt or not a level", file.GetHostAndLocalPath().AsCharPtr());
    }

    // cleanup readers
    levelStream->Close();
    jsonReader->Close();
}

//------------------------------------------------------------------------------
/**
*/
bool 
LevelExporter::ExportLevel(const Ptr<IO::JsonReader>& reader, Game::World* world, const IO::URI& outputFile)
{
    Ptr<BaseGameFeature::LevelParser> parser = BaseGameFeature::LevelParser::Create();
    parser->SetWorld(world);
    if (parser->LoadJsonLevel(reader))
    {
        world->ExportLevel(outputFile.GetHostAndLocalPath());
        return true;
    }
    return false;
}

} // namespace ToolkitUtil