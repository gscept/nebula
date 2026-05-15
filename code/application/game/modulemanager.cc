//------------------------------------------------------------------------------
//  modulemanager.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "game/modulemanager.h"

#include "game/featureunit.h"
#include "game/gameserver.h"
#include "io/fswrapper.h"
#include "io/ioserver.h"
#include "system/library.h"

#if __WIN32__
#include <windows.h>
#endif

namespace
{
// windows helper stuff for dealing with pdb locking/renaming etc. maybe move this to some other dedicated file later.
// heavily inspired by fungos/cr
#if __WIN32__

// RSDS signature in little-endian dword form.
static constexpr DWORD RsdsSignature = 'SDSR';

//------------------------------------------------------------------------------
/**
    Nebulas ioserver::copy copies the file bytewise, just do a direct copy here
*/
static bool
CopyFileNative(const Util::String& src, const Util::String& dst)
{
    return ::CopyFileA(src.AsCharPtr(), dst.AsCharPtr(), FALSE) == TRUE;
}

//------------------------------------------------------------------------------
/**
*/
static bool
RvaToFileOffset(const IMAGE_NT_HEADERS* ntHeaders, DWORD rva, DWORD& outOffset)
{
    const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(ntHeaders);
    for (WORD i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, section++)
    {
        const DWORD sectionStart = section->VirtualAddress;
        const DWORD sectionSize = section->Misc.VirtualSize > section->SizeOfRawData ? section->Misc.VirtualSize : section->SizeOfRawData;
        if (rva >= sectionStart && rva < sectionStart + sectionSize)
        {
            outOffset = section->PointerToRawData + (rva - sectionStart);
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
static bool
PatchPdbReferenceInCopiedDll(const Util::String& copiedDllPath, const Util::String& newPdbName, Util::String& outOriginalPdb)
{
    HANDLE file = ::CreateFileA(copiedDllPath.AsCharPtr(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    LARGE_INTEGER fileSize = {};
    if (::GetFileSizeEx(file, &fileSize) == FALSE || fileSize.QuadPart <= 0)
    {
        ::CloseHandle(file);
        return false;
    }

    HANDLE mapping = ::CreateFileMappingA(file, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    if (mapping == nullptr)
    {
        ::CloseHandle(file);
        return false;
    }

    byte* data = (byte*)::MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (data == nullptr)
    {
        ::CloseHandle(mapping);
        ::CloseHandle(file);
        return false;
    }

    const size_t size = (size_t)fileSize.QuadPart;
    bool patched = false;

    if (size >= sizeof(IMAGE_DOS_HEADER))
    {
        const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)data;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew > 0 && (size_t)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) <= size)
        {
            const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)(data + dos->e_lfanew);
            if (nt->Signature == IMAGE_NT_SIGNATURE)
            {
                DWORD debugRva = 0;
                DWORD debugSize = 0;
                if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                {
                    const IMAGE_OPTIONAL_HEADER64* opt64 = (const IMAGE_OPTIONAL_HEADER64*)&nt->OptionalHeader;
                    debugRva = opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
                    debugSize = opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
                }
                else if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
                {
                    const IMAGE_OPTIONAL_HEADER32* opt32 = (const IMAGE_OPTIONAL_HEADER32*)&nt->OptionalHeader;
                    debugRva = opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
                    debugSize = opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
                }

                if (debugRva != 0 && debugSize >= sizeof(IMAGE_DEBUG_DIRECTORY))
                {
                    DWORD debugOffset = 0;
                    if (RvaToFileOffset(nt, debugRva, debugOffset) && (size_t)debugOffset + debugSize <= size)
                    {
                        const size_t numEntries = debugSize / sizeof(IMAGE_DEBUG_DIRECTORY);
                        IMAGE_DEBUG_DIRECTORY* entries = (IMAGE_DEBUG_DIRECTORY*)(data + debugOffset);

                        for (size_t i = 0; i < numEntries && !patched; i++)
                        {
                            IMAGE_DEBUG_DIRECTORY& entry = entries[i];
                            if (entry.Type != IMAGE_DEBUG_TYPE_CODEVIEW || entry.PointerToRawData == 0 || entry.SizeOfData < (sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD) + 1))
                                continue;

                            if ((size_t)entry.PointerToRawData + entry.SizeOfData > size)
                                continue;

                            byte* cvData = data + entry.PointerToRawData;
                            DWORD signature = *(DWORD*)cvData;
                            if (signature != RsdsSignature)
                                continue;

                            char* pdb = (char*)(cvData + sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD));
                            const size_t maxLen = entry.SizeOfData - (sizeof(DWORD) + sizeof(GUID) + sizeof(DWORD));

                            size_t oldLen = 0;
                            while (oldLen < maxLen && pdb[oldLen] != '\0')
                                oldLen++;

                            if (oldLen == maxLen)
                                continue;

                            if (newPdbName.Length() > oldLen)
                                continue;

                            outOriginalPdb = pdb;
                            Memory::Copy(newPdbName.AsCharPtr(), pdb, newPdbName.Length());
                            pdb[newPdbName.Length()] = '\0';
                            patched = true;
                        }
                    }
                }
            }
        }
    }

    ::UnmapViewOfFile(data);
    ::CloseHandle(mapping);
    ::CloseHandle(file);
    return patched;
}

//------------------------------------------------------------------------------
/**
*/
static bool
CreateWindowsShadowCopyArtifacts(const Util::String& sourceDllPath, const Util::String& moduleName, uint serial, Util::String& outShadowDllPath, Util::String& outShadowPdbPath)
{
    IndexT slash = sourceDllPath.FindCharIndexReverse('/');
    IndexT backslash = sourceDllPath.FindCharIndexReverse('\\');
    IndexT separator = slash > backslash ? slash : backslash;

    Util::String folder = separator != InvalidIndex ? sourceDllPath.ExtractRange(0, separator + 1) : "";
    Util::String fileName = sourceDllPath.ExtractFileName();
    Util::String baseName = fileName;
    baseName.StripFileExtension();

    Util::String safeModule = moduleName.IsValid() ? moduleName : baseName;
    safeModule.SubstituteString(" ", "_");

    Util::String shadowFileName = Util::String::Sprintf("%s.reload.%u.dll", safeModule.AsCharPtr(), serial);
    outShadowDllPath = folder + shadowFileName;
    outShadowPdbPath = outShadowDllPath;
    outShadowPdbPath.ChangeFileExtension("pdb");

    IO::FSWrapper::DeleteFile(outShadowDllPath);
    IO::FSWrapper::DeleteFile(outShadowPdbPath);

    if (!CopyFileNative(sourceDllPath, outShadowDllPath))
        return false;

    Util::String originalPdbPath;
    const Util::String shadowPdbFileName = outShadowPdbPath.ExtractFileName();
    if (PatchPdbReferenceInCopiedDll(outShadowDllPath, shadowPdbFileName, originalPdbPath))
    {
        Util::String copySourcePdb = originalPdbPath;
        if (!IO::FSWrapper::FileExists(copySourcePdb))
        {
            copySourcePdb = sourceDllPath;
            copySourcePdb.ChangeFileExtension("pdb");
        }

        if (!copySourcePdb.IsValid() || !IO::FSWrapper::FileExists(copySourcePdb) || !CopyFileNative(copySourcePdb, outShadowPdbPath))
        {
            n_warning("ModuleManager: failed to copy shadow PDB '%s' for '%s'", outShadowPdbPath.AsCharPtr(), outShadowDllPath.AsCharPtr());
        }
    }
    else
    {
        n_warning("ModuleManager: failed to patch RSDS PDB reference in '%s'", outShadowDllPath.AsCharPtr());
    }

    return true;
}

//------------------------------------------------------------------------------
/**
*/
static void
DeleteShadowArtifacts(const Util::String& shadowDllPath, const Util::String& shadowPdbPath)
{
    if (shadowPdbPath.IsValid())
    {
        IO::FSWrapper::DeleteFile(shadowPdbPath);
    }
    if (shadowDllPath.IsValid())
    {
        IO::FSWrapper::DeleteFile(shadowDllPath);
    }
}

#endif
}

namespace Game
{
__ImplementClass(Game::ModuleManager, 'GMDM', Core::RefCounted);

struct ModuleManager::LoadedModule
{
    RuntimeModuleConfig config;
    Base::Library* library;
    Ptr<FeatureUnit> feature;
    Util::String loadedLibraryPath;
#if __WIN32__
    Util::String loadedPdbPath;
    bool usesShadowCopy = false;
#endif
};

//------------------------------------------------------------------------------
/**
*/
ModuleManager::ModuleManager()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ModuleManager::~ModuleManager()
{
    n_assert(this->loadedModules.IsEmpty());
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::LoadModules(const Util::Array<RuntimeModuleConfig>& moduleConfigs, GameServer* gameServer, bool strictMode)
{
    n_assert(gameServer != nullptr);
    bool success = true;
    for (IndexT i = 0; i < moduleConfigs.Size(); i++)
    {
        const RuntimeModuleConfig& moduleConfig = moduleConfigs[i];
        if (!moduleConfig.enabled)
            continue;

        if (!this->LoadModule(moduleConfig, gameServer, strictMode))
        {
            success = false;
            if (strictMode || moduleConfig.required)
                break;
        }
    }
    return success;
}

//------------------------------------------------------------------------------
/**
*/
void
ModuleManager::UnloadModules(GameServer* gameServer)
{
    if (gameServer == nullptr)
        return;

    for (IndexT i = this->loadedModules.Size() - 1; i >= 0; i--)
    {
        this->UnloadModule(this->loadedModules[i], gameServer);
    }
    this->loadedModules.Clear();
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::IsModuleLoaded(const Util::String& moduleName) const
{
    return this->FindLoadedModuleIndex(moduleName) != InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ModuleManager::GetNumLoadedModules() const
{
    return this->loadedModules.Size();
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::LoadModule(const RuntimeModuleConfig& moduleConfig, GameServer* gameServer, bool strictMode)
{
    if (moduleConfig.name.IsValid() && this->IsModuleLoaded(moduleConfig.name))
    {
        n_printf("ModuleManager: module '%s' is already loaded, skipping duplicate request\n", moduleConfig.name.AsCharPtr());
        return true;
    }

    Util::String libraryPath = this->ResolveLibraryPath(moduleConfig);
    if (!libraryPath.IsValid())
    {
        n_warning("ModuleManager: module '%s' has no valid path\n", moduleConfig.name.AsCharPtr());
        return !(strictMode || moduleConfig.required);
    }

    Util::String runtimeLoadPath = libraryPath;
#if __WIN32__
    Util::String runtimePdbPath;
    bool usesShadowCopy = false;
    if (CreateWindowsShadowCopyArtifacts(libraryPath, moduleConfig.name, this->windowsShadowCopySerial++, runtimeLoadPath, runtimePdbPath))
    {
        usesShadowCopy = true;
    }
    else
    {
        runtimeLoadPath = libraryPath;
        runtimePdbPath.Clear();
        n_warning("ModuleManager: failed to prepare shadow copy for '%s', falling back to canonical DLL", moduleConfig.name.AsCharPtr());
    }
#endif

    Base::Library* library = new System::Library();
    library->SetPath(IO::URI(runtimeLoadPath));
    if (!library->Load())
    {
#if __WIN32__
        if (usesShadowCopy)
        {
            DeleteShadowArtifacts(runtimeLoadPath, runtimePdbPath);
        }
#endif
        delete library;
        return !(strictMode || moduleConfig.required);
    }

    NebulaModuleGetDescriptorFn getDescriptor = reinterpret_cast<NebulaModuleGetDescriptorFn>(library->GetExport(NEBULA_MODULE_GET_DESCRIPTOR_EXPORT));
    NebulaModuleCreateFeatureFn createFeature = reinterpret_cast<NebulaModuleCreateFeatureFn>(library->GetExport(NEBULA_MODULE_CREATE_FEATURE_EXPORT));
    NebulaModuleDestroyFeatureFn destroyFeature = reinterpret_cast<NebulaModuleDestroyFeatureFn>(library->GetExport(NEBULA_MODULE_DESTROY_FEATURE_EXPORT));

    if (getDescriptor == nullptr || createFeature == nullptr)
    {
        n_warning("ModuleManager: module '%s' is missing required exports\n", moduleConfig.name.AsCharPtr());
        library->Close();
        delete library;
#if __WIN32__
        if (usesShadowCopy)
        {
            DeleteShadowArtifacts(runtimeLoadPath, runtimePdbPath);
        }
#endif
        return !(strictMode || moduleConfig.required);
    }

    NebulaModuleDescriptor desc = {};
    if (getDescriptor(&desc) == 0)
    {
        n_warning("ModuleManager: module '%s' descriptor callback failed\n", moduleConfig.name.AsCharPtr());
        library->Close();
        delete library;
#if __WIN32__
        if (usesShadowCopy)
        {
            DeleteShadowArtifacts(runtimeLoadPath, runtimePdbPath);
        }
#endif
        return !(strictMode || moduleConfig.required);
    }

    if (desc.abiVersion != NEBULA_MODULE_ABI_VERSION)
    {
        n_warning("ModuleManager: module '%s' has ABI %u, expected %u\n", moduleConfig.name.AsCharPtr(), desc.abiVersion, NEBULA_MODULE_ABI_VERSION);
        library->Close();
        delete library;
#if __WIN32__
        if (usesShadowCopy)
        {
            DeleteShadowArtifacts(runtimeLoadPath, runtimePdbPath);
        }
#endif
        return !(strictMode || moduleConfig.required);
    }

    FeatureUnit* featureRaw = reinterpret_cast<FeatureUnit*>(createFeature());
    if (featureRaw == nullptr)
    {
        n_warning("ModuleManager: module '%s' did not return a feature instance\n", moduleConfig.name.AsCharPtr());
        library->Close();
        delete library;
#if __WIN32__
        if (usesShadowCopy)
        {
            DeleteShadowArtifacts(runtimeLoadPath, runtimePdbPath);
        }
#endif
        return !(strictMode || moduleConfig.required);
    }

    Ptr<FeatureUnit> feature = featureRaw;
    gameServer->AttachGameFeature(feature);

    LoadedModule loaded;
    loaded.config = moduleConfig;
    loaded.library = library;
    loaded.feature = feature;
    loaded.loadedLibraryPath = runtimeLoadPath;
#if __WIN32__
    loaded.loadedPdbPath = runtimePdbPath;
    loaded.usesShadowCopy = usesShadowCopy;
#endif
    this->loadedModules.Append(loaded);

    const char* descName = desc.name != nullptr ? desc.name : "<unnamed>";
    const char* descVersion = desc.version != nullptr ? desc.version : "<unknown>";
    if (destroyFeature != nullptr)
    {
        n_warning("ModuleManager: module '%s' exports '%s', but runtime expects FeatureUnit instances to be managed via Nebula refcounting\n", descName, NEBULA_MODULE_DESTROY_FEATURE_EXPORT);
    }
    n_printf("ModuleManager: loaded module '%s' v%s from '%s'\n", descName, descVersion, runtimeLoadPath.AsCharPtr());
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::UnloadModule(LoadedModule& loaded, GameServer* gameServer)
{
    if (loaded.feature.isvalid())
    {
        gameServer->RemoveGameFeature(loaded.feature);

        // Keep the module resident if external references still hold the feature.
        // Unloading the library in this state could leave dangling vtables.
        if (loaded.feature->GetRefCount() > 1)
        {
            n_warning("ModuleManager: module '%s' still has external references on unload (%d), keeping shared library loaded\n", loaded.config.name.AsCharPtr(), loaded.feature->GetRefCount());
            return false;
        }

        loaded.feature = nullptr;
    }

    if (loaded.library != nullptr)
    {
        if (loaded.library->IsLoaded())
        {
            loaded.library->Close();
        }
        delete loaded.library;
        loaded.library = nullptr;
    }

#if __WIN32__
    if (loaded.usesShadowCopy)
    {
        DeleteShadowArtifacts(loaded.loadedLibraryPath, loaded.loadedPdbPath);
        loaded.loadedPdbPath.Clear();
        loaded.usesShadowCopy = false;
    }
#endif
    loaded.loadedLibraryPath.Clear();

    return true;
}

//------------------------------------------------------------------------------
/**
*/
Util::String
ModuleManager::ResolveLibraryPath(const RuntimeModuleConfig& moduleConfig) const
{
    if (moduleConfig.path.IsValid())
        return moduleConfig.path;

    if (!moduleConfig.name.IsValid())
        return "";

    Util::String path = moduleConfig.name;

    if (path.ContainsCharFromSet("/\\"))
        return path;

#if __WIN32__
    if (!path.EndsWithString(".dll"))
    {
        path.Append(".dll");
    }
#else
    if (!path.BeginsWithString("lib"))
    {
        path = Util::String::Sprintf("lib%s", path.AsCharPtr());
    }
    if (!path.EndsWithString(".so"))
    {
        path.Append(".so");
    }
#endif

    if (IO::FSWrapper::FileExists(path))
    {
        return IO::IoServer::NativePath(path);
    }

#if defined(NEBULA_BINARY_FOLDER)
    Util::String deployRoot = NEBULA_BINARY_FOLDER;
    deployRoot.ConvertBackslashes();
    Util::String deployCandidate = Util::String::AppendPath(deployRoot, path);
    if (IO::FSWrapper::FileExists(deployCandidate))
    {
        return deployCandidate;
    }
#endif

    return path;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
ModuleManager::FindLoadedModuleIndex(const Util::String& moduleName) const
{
    Util::String checkName = moduleName;
    checkName.ToLower();

    for (IndexT i = 0; i < this->loadedModules.Size(); i++)
    {
        Util::String loadedName = this->loadedModules[i].config.name;
        loadedName.ToLower();
        if (loadedName == checkName)
            return i;
    }

    return InvalidIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
ModuleManager::QueueModuleReload(const Util::String& moduleName)
{
    // Deduplicate: only queue a reload once per frame
    if (this->pendingReloads.FindIndex(moduleName) != InvalidIndex)
        return;

    n_printf("ModuleManager: reload of '%s' queued for next frame boundary\n", moduleName.AsCharPtr());
    this->pendingReloads.Append(moduleName);
}

//------------------------------------------------------------------------------
/**
*/
void
ModuleManager::ProcessPendingReloads(GameServer* gameServer)
{
    if (this->pendingReloads.IsEmpty())
        return;

    // Snapshot and clear the queue before processing so that reloads triggered
    // during the reload itself are deferred to the following frame.
    Util::Array<Util::String> toProcess = this->pendingReloads;
    this->pendingReloads.Clear();

    for (IndexT i = 0; i < toProcess.Size(); i++)
    {
        this->ReloadModuleByName(toProcess[i], gameServer);
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
ModuleManager::ReloadModuleByName(const Util::String& moduleName, GameServer* gameServer)
{
    IndexT idx = this->FindLoadedModuleIndex(moduleName);

    if (idx == InvalidIndex)
    {
        n_warning("ModuleManager: reload of '%s' failed: module is not loaded\n", moduleName.AsCharPtr());
        return false;
    }

    // Save config so we can re-load with the same settings
    RuntimeModuleConfig config = this->loadedModules[idx].config;

    n_printf("ModuleManager: reloading '%s'...\n", moduleName.AsCharPtr());

    if (!this->UnloadModule(this->loadedModules[idx], gameServer))
    {
        n_warning("ModuleManager: reload of '%s' blocked: module could not be safely unloaded\n", moduleName.AsCharPtr());
        return false;
    }

    this->loadedModules.EraseIndex(idx);

    if (!this->LoadModule(config, gameServer, false))
    {
        n_warning("ModuleManager: reload of '%s' failed during load\n", moduleName.AsCharPtr());
        return false;
    }

    n_printf("ModuleManager: reload of '%s' complete\n", moduleName.AsCharPtr());
    return true;
}

} // namespace Game
