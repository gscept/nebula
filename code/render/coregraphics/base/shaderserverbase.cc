//------------------------------------------------------------------------------
//  shaderserverbase.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "resources/resourceserver.h"
#include "coregraphics/base/shaderserverbase.h"
#include "coregraphics/graphicsdevice.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include "io/filewatcher.h"
#include "io/memorystream.h"
#include "system/process.h"
#include "system/nebulasettings.h"

namespace Base
{
__ImplementClass(Base::ShaderServerBase, 'SSRV', Core::RefCounted);
__ImplementSingleton(Base::ShaderServerBase);

using namespace CoreGraphics;
using namespace IO;
using namespace Util;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
ShaderServerBase::ShaderServerBase() :
    curShaderFeatureBits(0),
    objectIdShaderVar(Ids::InvalidId32),
    isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ShaderServerBase::~ShaderServerBase()
{
    n_assert(!this->IsOpen());
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
RecursiveLoadShaders(ShaderServerBase* shaderServer, const Util::String& path)
{
    Util::Array<Util::String> files = IoServer::Instance()->ListFiles(path, "*.gplb");

    for (IndexT i = 0; i < files.Size(); i++)
    {
        ResourceName resId = path + files[i];

        // load shader
        shaderServer->LoadShader(resId);
    }

    Util::Array<Util::String> directories = IoServer::Instance()->ListDirectories(path, "*", false, false);
    for (IndexT i = 0; i < directories.Size(); i++)
    {
        RecursiveLoadShaders(shaderServer, path + directories[i] + "/");
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderServerBase::Open()
{
    n_assert(!this->isOpen);
    n_assert(this->shaders.IsEmpty());

    RecursiveLoadShaders(this, "shd:");

//#ifndef __linux__
//    auto reloadFileFunc = [this](IO::WatchEvent const& event)
//    {
//        if (event.type == WatchEventType::Modified || event.type == WatchEventType::NameChange &&
//            !event.file.EndsWithString("TMP") &&
//            !event.file.EndsWithString("~"))
//        {
//            // remove file extension
//            Util::String out = event.file;
//            out.StripFileExtension();
//
//            // find argument in export folder
//            Ptr<IO::Stream> file = IO::IoServer::Instance()->CreateStream(Util::String::Sprintf("bin:shaders/%s.txt", out.AsCharPtr()));
//            if (file->Open())
//            {
//                System::Process process;
//
//                void* buf = file->Map();
//                SizeT size = file->GetSize();
//
//                // run process
//                Util::String cmd;
//                cmd.Set((const char*)buf, size);
//                process.SetWorkingDirectory(file->GetURI().LocalPath().ExtractDirName());
//                process.SetExecutable(cmd);
//                process.SetNoConsoleWindow(false);
//                Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();
//                process.SetStderrCaptureStream(stream);
//
//                // launch process
//                bool res = process.LaunchWait();
//                if (res)
//                {
//                    Ptr<TextReader> reader = TextReader::Create();
//                    reader->SetStream(stream);
//                    reader->Open();
//
//                    // write output from compilation
//                    while (!reader->Eof())
//                    {
//                        Core::SysFunc::DebugOut(reader->ReadLine().AsCharPtr());
//                    }
//
//                    // close reader
//                    reader->Close();
//
//                    // Get exported file name, this is what we need to reload.
//                    IndexT oIndex = cmd.FindStringIndex("-o");
//                    if (oIndex != InvalidIndex)
//                    {
//                        Util::String exportedFilePath;
//                        char const* c = cmd.AsCharPtr() + oIndex + 3; // skip "-o "
//                        while (*c != ' ')
//                        {
//                            exportedFilePath.AppendChar(*c);
//                            c += 1;
//                        }
//
//                        // reload shader
//                        this->pendingShaderReloads.Enqueue(exportedFilePath);
//                    }
//                }
//            }
//        }
//    };
//
//    // create file watcher
//    if (IO::IoServer::Instance()->DirectoryExists("home:work/shaders/vk"))
//    {
//        FileWatcher::Instance()->Watch("home:work/shaders/vk", true, IO::WatchFlags(NameChanged | SizeChanged | Write), reloadFileFunc);
//    }
//
//#ifndef PUBLIC_BUILD
//    if (System::NebulaSettings::Exists("gscept", "ToolkitShared", "path"))
//    {
//        IO::URI shaderPath = System::NebulaSettings::ReadString("gscept", "ToolkitShared", "path");
//        shaderPath.AppendLocalPath("syswork/shaders/vk");
//        if (IO::IoServer::Instance()->DirectoryExists(shaderPath))
//        {
//            FileWatcher::Instance()->Watch(shaderPath.AsString(), true, IO::WatchFlags(NameChanged | SizeChanged | Write), reloadFileFunc);
//        }
//    }
//#endif
//#endif

    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderServerBase::Close()
{
    n_assert(this->isOpen);
    #ifndef __linux__
    // unwatch 
    //if (IO::IoServer::Instance()->DirectoryExists("home:work/shaders/vk"))
    //{
    //    IO::FileWatcher::Instance()->Unwatch("home:work/shaders/vk");
    //}
    #endif

    // unload all currently loaded shaders
    IndexT i;
    for (i = 0; i < this->shaders.Size(); i++)
    {
        Resources::DiscardResource(this->shaders.ValueAtIndex(i));
    }
    this->shaders.Clear();

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderServerBase::LoadShader(const Resources::ResourceName& shdName)
{
	n_assert(shdName.IsValid());
	Resources::ResourceId sid = Resources::CreateResource(shdName, "shaders"_atm, nullptr,
		[shdName](const ResourceId id)
	{
		n_error("Failed to load shader '%s'!\n", shdName.Value());
	}, true);
	
	this->shaders.Add(shdName, sid);
}

//------------------------------------------------------------------------------
/**
*/
void 
ShaderServerBase::BeforeFrame()
{
    if (this->pendingShaderReloads.Size() > 0)
    {
        // wait for all graphics commands to finish first
        CoreGraphics::WaitAndClearPendingCommands();

        Util::Array<Resources::ResourceName> shaders;
        shaders.Reserve(4);
        this->pendingShaderReloads.DequeueAll(shaders);

        // reload shaders
        IndexT i;
        for (i = 0; i < shaders.Size(); i++)
            Resources::ReloadResource(shaders[i]);
    }
}

} // namespace Base
