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

#ifndef __linux__
    if (!IO::DirectoryExists(NEBULA_BUILD_FOLDER"/shader_dependencies"))
    {
        n_printf("Can't find shader dependencies folder. To use shader hot reloading, you need to have Nebula built from source.");
    }
    else
    {
        Util::Set<Util::String> watchDirs;
        Util::Array<Util::String> folders;
        folders.Append(NEBULA_BUILD_FOLDER"/shader_dependencies");
        while (folders.Size() > 0)
        {
            Util::String current = folders.PopFront();
            folders.AppendArray(IO::ListDirectories(current, "*", true));

            const Util::Array<Util::String> depFiles = IO::ListFiles(current, "*.dep", true);
            for (IndexT i = 0; i < depFiles.Size(); i++)
            {
                Util::String contents;
                n_assert2(IoServer::ReadFile(depFiles[i], contents), depFiles[i].AsCharPtr());

                Util::Array<Util::String> dependencies = contents.Tokenize(";");
                n_assert(dependencies.Size() > 0);

                Util::String source = dependencies[0];
                source.Trim(" \r\n\t");
                n_assert(source.IsValid());
                source = IoServer::NativePath(source);
                source.ConvertBackslashes();
#if __WIN32__
                source.ToLower();
#endif

                for (IndexT j = 0; j < dependencies.Size(); j++)
                {
                    Util::String dep = dependencies[j];
                    dep.Trim(" \r\n\t");
                    n_assert(dep.IsValid());

                    dep = IoServer::NativePath(dep);
                    dep.ConvertBackslashes();
#if __WIN32__
                    dep.ToLower();
#endif

                    Util::String watchDir = dep.ExtractDirName();
                    watchDir.TrimRight("/");
                    n_assert(watchDir.IsValid());
                    watchDirs.Add(watchDir);

                    this->shaderReloadMap.Emplace(dep).Add(source);
                }
            }
        }

        const Util::Array<Util::String>& dirs = watchDirs.KeysAsArray();
        for (IndexT i = 0; i < dirs.Size(); i++)
        {
            bool nested = false;
            for (IndexT j = 0; j < dirs.Size(); j++)
            {
                if (i == j)
                {
                    continue;
                }
                Util::String prefix = dirs[j];
                prefix.Append("/");
                if (dirs[i].BeginsWithString(prefix))
                {
                    nested = true;
                    break;
                }
            }
            if (!nested)
            {
                this->shaderWatchFolders.Append(dirs[i]);
            }
        }

        auto reloadFileFunc = [this](IO::WatchEvent const& event)
        {
            if ((event.type != WatchEventType::Modified && event.type != WatchEventType::NameChange) ||
                event.file.EndsWithString("TMP") ||
                event.file.EndsWithString("~"))
            {
                return;
            }

            Util::String changed = event.folder.AsString();
            if (!event.relativePath.IsEmpty())
            {
                changed.AppendPath(event.relativePath);
            }
            changed.AppendPath(event.file);
            changed.ConvertBackslashes();
#if __WIN32__
            changed.ToLower();
#endif

            IndexT mapIndex = this->shaderReloadMap.FindIndex(changed);
            if (mapIndex == InvalidIndex)
            {
                return;
            }

            const Util::Array<Util::String>& sources = this->shaderReloadMap.ValueAtIndex(mapIndex).KeysAsArray();
            for (IndexT i = 0; i < sources.Size(); i++)
            {
                Util::String out = sources[i].ExtractFileName();
                out.StripFileExtension();

                Ptr<IO::Stream> file = IO::IoServer::Instance()->CreateStream(Util::String::Sprintf("bin:shaders/%s.txt", out.AsCharPtr()));
                if (file->Open())
                {
                    void* buf = file->Map();
                    SizeT size = file->GetSize();

                    Util::String cmd;
                    cmd.Set((const char*)buf, size);

                    Ptr<IO::MemoryStream> stream = IO::MemoryStream::Create();

                    System::ProcessStartInfo processInfo;
                    processInfo.workingDir = file->GetURI().LocalPath().ExtractDirName();
                    processInfo.exePath = cmd;
                    processInfo.consoleWindow = false;
                    processInfo.outputStream = stream;

                    System::ProcessId process = System::StartProcess(processInfo);
                    uint exitCode = System::WaitForProcess(process);

                    Ptr<TextReader> reader = TextReader::Create();
                    reader->SetStream(stream);
                    reader->Open();

                    if (exitCode != 0)
                    {
                        n_printf("Process %s ended with exit code %d\n", cmd.AsCharPtr(), exitCode);
                    }

                    while (!reader->Eof())
                    {
                        Core::SysFunc::DebugOut(reader->ReadLine().AsCharPtr());
                    }

                    reader->Close();

                    IndexT oIndex = cmd.FindStringIndex("-o");
                    if (oIndex != InvalidIndex)
                    {
                        Util::String exportedFilePath;
                        char const* c = cmd.AsCharPtr() + oIndex + 3;
                        while (*c != ' ' && *c != '\0')
                        {
                            exportedFilePath.AppendChar(*c);
                            c += 1;
                        }

                        this->pendingShaderReloads.Enqueue(exportedFilePath);
                    }
                }
            }
        };

        for (IndexT i = 0; i < this->shaderWatchFolders.Size(); i++)
        {
            FileWatcher::Instance()->Watch(this->shaderWatchFolders[i], true, IO::WatchFlags(NameChanged | SizeChanged | Write), reloadFileFunc);
        }
    }
#endif

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
    for (IndexT i = 0; i < this->shaderWatchFolders.Size(); i++)
    {
        FileWatcher::Instance()->Unwatch(this->shaderWatchFolders[i]);
    }
    this->shaderWatchFolders.Clear();
    this->shaderReloadMap.Clear();
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
