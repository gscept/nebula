//------------------------------------------------------------------------------
//  flatbufferinterface.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "flatbufferinterface.h"
#include "io/assignregistry.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/streamreader.h"
#include "io/uri.h"
#include "util/stringatom.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"

using namespace Flat;

// wrapper functions to allow flatbuffer to use nebula assign style paths
bool FlatFileExistFunction(const char* filename)
{
    return IO::IoServer::Instance()->FileExists(filename);
}

bool FlatLoadFileFunction(const char* filename, bool binary, std::string* target)
{
    Util::String buf;
    bool success = IO::IoServer::Instance()->ReadFile(filename, buf);
    n_assert(success);
    target->assign(buf.AsCharPtr(), buf.Length() + 1);
    return success;
}

namespace
{
struct FlatbufferState
{
    void Init();
    bool LoadFbs(IO::URI const& file);
    void LoadExported();
    void LoadFolder(Util::String const& folder);
    
    Util::Dictionary<Util::StringAtom, flatbuffers::Parser*> parsers;
    Util::Dictionary<Util::StringAtom, IO::URI> schemaFiles;
};

void FlatbufferState::Init()
{
    n_assert(IO::AssignRegistry::HasInstance());
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("bfbs", "data:flatbuffer"));
    // this is only for tool-time as it uses work
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("fbs", "root:work/data/flatbuffer"));
    flatbuffers::SetLoadFileFunction(&FlatLoadFileFunction);
    flatbuffers::SetFileExistsFunction(&FlatFileExistFunction);
}

void FlatbufferState::LoadFolder(Util::String const& folder)
{
    Util::Array<Util::String> files = IO::IoServer::Instance()->ListFiles(folder, "*.bfbs", true);

    for (int i = 0; i < files.Size(); i++)
    {
        LoadFbs(files[i]);
    }

    // Recurse all folders
    Util::Array<Util::String> dirs = IO::IoServer::Instance()->ListDirectories(folder, "*", true);
    for (auto const& dir : dirs)
    {
        this->LoadFolder(dir);
    }
}

//------------------------------------------------------------------------------
/**
*/
void FlatbufferState::LoadExported()
{
    this->LoadFolder("bfbs:");
}

//------------------------------------------------------------------------------
/**
*/
flatbuffers::Parser*
CreateParser(IO::URI const& file)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    if (stream->Open())
    {
        void * buf = stream->Map();
        flatbuffers::Parser* parser = new flatbuffers::Parser;
        if (parser->Deserialize((uint8_t*)buf, stream->GetSize()))
        {
            if (!parser->file_identifier_.empty())
            {
               return parser;
            }
        }
        delete(parser);
    }
    return nullptr;

}

//------------------------------------------------------------------------------
/**
*/
bool FlatbufferState::LoadFbs(IO::URI const& file)
{
    flatbuffers::Parser* parser = CreateParser(file);
    if (parser != nullptr)
    {
        FlatbufferState::parsers.Add(parser->file_identifier_.c_str(), parser);
        FlatbufferState::schemaFiles.Add(parser->file_identifier_.c_str(), file);
        return true;
    }
    return false;
}

FlatbufferState state;
}

//------------------------------------------------------------------------------
/**
*/
void FlatbufferInterface::Init()
{
    state.Init();
    state.LoadExported();
}

//------------------------------------------------------------------------------
/**
*/
bool FlatbufferInterface::LoadSchema(IO::URI const& file)
{
    return state.LoadFbs(file);
}

//------------------------------------------------------------------------------
/**
*/
bool FlatbufferInterface::CompileSchema(IO::URI const& file)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    if (stream->Open())
    {
        void* buf = stream->Map();
        IO::URI systemInclude = "tool:syswork/data/flatbuffer/";
        const char* includes[2];
        includes[0] = systemInclude.LocalPath().AsCharPtr();
        includes[1] = nullptr;
        flatbuffers::Parser* parser = new flatbuffers::Parser;
        parser->Parse((const char*)buf, includes, file.LocalPath().AsCharPtr());
        parser->Serialize();
        auto blorf = parser->builder_.GetSize();
        char* ff = reinterpret_cast<char*>(parser->builder_.GetBufferPointer());
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool FlatbufferInterface::HasSchema(Util::StringAtom ident)
{
    return state.parsers.Contains(ident);
}

//------------------------------------------------------------------------------
/**
*/
Util::String FlatbufferInterface::BufferToText(const uint8_t* buf, Util::StringAtom ident)
{
    std::string output;
    flatbuffers::GenerateText(*state.parsers[ident], buf, &output);
    return output.c_str();
}

//------------------------------------------------------------------------------
/**
*/
bool FlatbufferInterface::Compile(IO::URI const& source, IO::URI const& targetFolder, const char* ident)
{
    bool result = false;
    if (!state.schemaFiles.Contains(ident))
    {
        return false;
    }
    flatbuffers::Parser *parser = CreateParser(state.schemaFiles[ident]);
    Util::String bufferText;
    if (IO::IoServer::Instance()->ReadFile(source, bufferText))
    {
        Util::String filename = source.LocalPath().ExtractFileName();
        filename.StripFileExtension();
        if (!parser->Parse(bufferText.AsCharPtr()))
        {
            n_warning("Failed to parse flatbuffer %s\n%s\n", source.AsString().AsCharPtr(), parser->error_.c_str());
            n_printf("%s\n", parser->error_.c_str());
        }
        else
        {
            Util::String target = targetFolder.LocalPath();
            // make sure we have a final / for the output folder
            // FIXME could use own writer
            target += "/";
            if (IO::IoServer::Instance()->CreateDirectory(target))
                result = flatbuffers::GenerateBinary(*parser, target.AsCharPtr(), filename.AsCharPtr());
        }
    }
    delete parser;
    return result;
}
