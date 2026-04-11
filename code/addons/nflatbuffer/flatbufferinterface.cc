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
#include "idl_gen_binary.h"
#include "flatbuffers/code_generator.h"

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
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("bfbs", "export:data/flatbuffer"));
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
bool 
FlatbufferInterface::LoadSchema(IO::URI const& file)
{
    return state.LoadFbs(file);
}

//------------------------------------------------------------------------------
/**
*/
flatbuffers::Parser* 
FlatbufferInterface::CreateParserForJson(IO::URI const& file)
{
    Util::String ext = file.LocalPath().GetFileExtension();
    ext.ToUpper();
    n_assert(state.schemaFiles.Contains(ext.AsCharPtr()));
    return CreateParser(state.schemaFiles[ext]);
}

//------------------------------------------------------------------------------
/**
*/
Util::Blob
FlatbufferInterface::ParseJson(IO::URI const& file)
{
    Util::String contents;
    if (IO::IoServer::Instance()->ReadFile(file, contents))
    {
        flatbuffers::Parser* parser = CreateParserForJson(file);
        if (parser->ParseJson(contents.AsCharPtr(),file.LocalPath().AsCharPtr()))
        {
            auto builderSize = parser->builder_.GetSize();
            char* builderData = reinterpret_cast<char*>(parser->builder_.GetBufferPointer());
            Util::Blob blob(builderData, builderSize);
            delete parser;
            return blob;
        }
        delete parser;
    }
    return Util::Blob();
}

//------------------------------------------------------------------------------
/**
*/
Util::Blob
FlatbufferInterface::ParseJson(IO::URI const& file, const Util::String& schema)
{
    Util::String contents;
    if (IO::IoServer::Instance()->ReadFile(file, contents))
    {
        IndexT schemaIndex = state.schemaFiles.FindIndex(schema.AsCharPtr());
        n_assert(schemaIndex != InvalidIndex);
        flatbuffers::Parser* parser = CreateParser(state.schemaFiles.ValueAtIndex(schemaIndex));
        if (parser->ParseJson(contents.AsCharPtr(),file.LocalPath().AsCharPtr()))
        {
            auto builderSize = parser->builder_.GetSize();
            char* builderData = reinterpret_cast<char*>(parser->builder_.GetBufferPointer());
            Util::Blob blob(builderData, builderSize);
            delete parser;
            return blob;
        }
        delete parser;
    }
    return Util::Blob();
}

//------------------------------------------------------------------------------
/**
*/
bool FlatbufferInterface::CompileSchema(IO::URI const& file, IO::URI const& outFile)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    bool retval = false;
    if (stream->Open())
    {
        IO::Stream::Size fileSize = stream->GetSize();
        char* tempBuffer = (char*)Memory::Alloc(Memory::ScratchHeap, fileSize + 1);
        stream->Read(tempBuffer, fileSize);
        stream->Close();
        tempBuffer[fileSize] = '\0';
        IO::URI systemInclude = "tool:syswork/data/flatbuffer/";
        IO::URI projectInclude = "fbs:";
        const char* includes[3];
        includes[0] = systemInclude.LocalPath().AsCharPtr();
        includes[1] = projectInclude.LocalPath().AsCharPtr();
        includes[2] = nullptr;


        flatbuffers::Parser* parser = new flatbuffers::Parser;

        if (parser->Parse((const char*)tempBuffer, includes, file.LocalPath().AsCharPtr()))
        {
            Memory::Free(Memory::ScratchHeap, tempBuffer);
            parser->Serialize();
            auto builderSize = parser->builder_.GetSize();
            char* builderData = reinterpret_cast<char*>(parser->builder_.GetBufferPointer());
            IO::IoServer::Instance()->EnsureDirectoriesForFile(outFile);
            Ptr<IO::Stream> outStream = IO::IoServer::Instance()->CreateStream(outFile);
            outStream->SetAccessMode(IO::Stream::WriteAccess);
            if (outStream->Open())
            {
                outStream->Write(builderData, builderSize);
                outStream->Close();
                retval = true;
            }
        }
        delete parser;
    }
    return retval;
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
            {
                std::unique_ptr<flatbuffers::RealFileSaver> file_saver(new flatbuffers::RealFileSaver());
                parser->opts.file_saver = file_saver.get();
                std::unique_ptr<flatbuffers::CodeGenerator> generator = flatbuffers::NewBinaryCodeGenerator();
                result = generator->GenerateCode(*parser, target.AsCharPtr(), filename.AsCharPtr());
                parser->opts.file_saver = nullptr;
            }
        }
    }
    delete parser;
    return result;
}
