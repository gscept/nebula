
#include "io/filewatcher.h"
#include "io/assignregistry.h"
#include "util/win32/win32stringconverter.h"
#include "core/sysfunc.h"

//------------------------------------------------------------------------------
/**
*/
static void 
QueryDirectoryChanges(HANDLE handle, OVERLAPPED* overlapped, void* buf, SizeT bufSize, DWORD filter)
{
    Memory::Clear(overlapped, sizeof(OVERLAPPED));
    bool res = ReadDirectoryChangesW(
        handle, buf, bufSize, false, filter, NULL, overlapped, NULL
    );
    n_assert(res);
}

//------------------------------------------------------------------------------
/**
*/
static void
AddWatchRecursive(const Util::String& folder, const Util::String& relativePath, HANDLE parent, IO::EventHandlerData& data)
{
    IO::FileWatcherPlatform& p = data.data;
    HANDLE& handle = p.handles.Emplace();
    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(folder, widePath, sizeof(widePath));
    handle = CreateFileW(
        (LPCWSTR)widePath,
        GENERIC_READ | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );
    n_assert(handle != INVALID_HANDLE_VALUE);
    //p.overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p.notifyFilter = 0;
    p.notifyFilter |= data.flags.IsSet(IO::NameChanged) ? FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME : 0;
    p.notifyFilter |= data.flags.IsSet(IO::SizeChanged) ? FILE_NOTIFY_CHANGE_SIZE : 0;
    p.notifyFilter |= data.flags.IsSet(IO::Write) ? FILE_NOTIFY_CHANGE_LAST_WRITE : 0;
    p.notifyFilter |= data.flags.IsSet(IO::Access) ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0;
    p.notifyFilter |= data.flags.IsSet(IO::Creation) ? FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_FILE_NAME : 0;

    OVERLAPPED& overlap = p.overlaps.Emplace();
    Util::FixedArray<BYTE>& buffer = p.buffers.Emplace();
    buffer.Resize(16 * 1024);

    QueryDirectoryChanges(handle, &overlap, buffer.Begin(), buffer.ByteSize(), p.notifyFilter);
    p.relativePathLookup.Add(handle, relativePath);

    Util::Array<Util::String> dirs = IO::ListDirectories(folder);
    for (const Util::String& dir : dirs)
    {
        AddWatchRecursive(Util::String::AppendPath(folder, dir), Util::String::AppendPath(relativePath, dir), handle, data);
    }
}

namespace IO
{

//------------------------------------------------------------------------------
/**
*/
void
FileWatcherImpl::CreateWatcher(EventHandlerData& data)
{
    Util::String local = IO::AssignRegistry::Instance()->ResolveAssigns(data.folder.AsString()).LocalPath();
    FileWatcherPlatform& p = data.data;

    AddWatchRecursive(local, "", nullptr, data);
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcherImpl::Update(EventHandlerData& data)
{
    FileWatcherPlatform& p = data.data;

    for (SizeT i = 0; i < p.handles.Size(); i++)
    {
        DWORD bytes;
        bool res = GetOverlappedResult(p.handles[i], &p.overlaps[i], &bytes, false);
        if (!res)
            continue;

        FILE_NOTIFY_INFORMATION* ev = (FILE_NOTIFY_INFORMATION*)p.buffers[i].Begin();
        do
        {
            if (ev->FileNameLength > 0)
            {
                Util::String relDir;
                IndexT relativeIndex = p.relativePathLookup.FindIndex(p.handles[i]);
                if (relativeIndex != InvalidIndex)
                    relDir = p.relativePathLookup.ValueAtIndex(relativeIndex);

                Util::String newFile = Win32::Win32StringConverter::WideToUTF8((ushort*)ev->FileName, ev->FileNameLength / 2);
                Util::String fullRelative =
                    data.folder.AsString().AppendPath(relDir).AppendPath(newFile);
                fullRelative.SubstituteString("\\", "/");
                Util::String relativePath = fullRelative.ExtractToLastSlash();
                relativePath.TrimRight("/");
                Util::String file = fullRelative.ExtractFileName();
                switch (ev->Action)
                {
                case FILE_ACTION_ADDED: {
                    data.callback({Created, data.folder, relDir, file});
                    ushort buf[2048];
                    Win32::Win32StringConverter::UTF8ToWide(fullRelative.AsCharPtr(), buf, sizeof(buf));
                    DWORD attributes = GetFileAttributesW((LPCWSTR)buf);
                    n_assert(attributes != INVALID_FILE_ATTRIBUTES);
                    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
                        AddWatchRecursive(data.folder.AsString().AppendPath(newFile), Util::String::AppendPath(relDir, newFile), p.handles[i], data);
                    break;
                }
                case FILE_ACTION_MODIFIED: {
                    data.callback({Modified, data.folder, relDir, file});
                    break;
                }
                case FILE_ACTION_REMOVED: {
                    data.callback({Deleted, data.folder, relDir, file});
                    break;
                }
                case FILE_ACTION_RENAMED_NEW_NAME: {
                    data.callback({NameChange, data.folder, relDir, file});
                    break;
                }
                }
            }

            if (ev->NextEntryOffset == 0)
                break;
            ev = (FILE_NOTIFY_INFORMATION*)((char*)ev + ev->NextEntryOffset);
        } while (true);
        QueryDirectoryChanges(p.handles[i], &p.overlaps[i], p.buffers[i].begin(), p.buffers[i].ByteSize(), p.notifyFilter);

    }
    /*
    DWORD bytes;
    bool res = GetOverlappedResult(p.dirHandle, &p.overlapped, &bytes, false);
    
    if (!res) return;

        
    FILE_NOTIFY_INFORMATION* ev = (FILE_NOTIFY_INFORMATION*)p.buffer;
    do
    {
        if (ev->FileNameLength > 0)
        {
            Util::String relDir;
            if (p.relativePathLookup.Contains(ev))
                relDir = p.relativePathLookup[data.folder];

            Util::String fullRelative = data.folder.AsString() + "/" + Win32::Win32StringConverter::WideToUTF8((ushort*)ev->FileName, ev->FileNameLength / 2);
            fullRelative.SubstituteString("\\", "/");
            Util::String relativePath = fullRelative.ExtractToLastSlash();
            relativePath.TrimRight("/");
            Util::String file = fullRelative.ExtractFileName();
            switch (ev->Action)
            {
                case FILE_ACTION_ADDED:
                {
                    data.callback({ Created, data.folder, relativePath, file });
                    break;
                }
                case FILE_ACTION_MODIFIED:
                {
                    data.callback({ Modified, data.folder, relativePath, file });
                    break;
                }
                case FILE_ACTION_REMOVED:
                {
                    data.callback({ Deleted, data.folder, relativePath, file });
                    break;
                }
                case FILE_ACTION_RENAMED_NEW_NAME:
                {
                    data.callback({ NameChange, data.folder, relativePath, file });
                    break;
                }
            }
        }

        if (ev->NextEntryOffset == 0)
            break;
        ev = (FILE_NOTIFY_INFORMATION*)((char*)ev + ev->NextEntryOffset);
    } while (true);
    */
}

//------------------------------------------------------------------------------
/**
*/
void 
FileWatcherImpl::DestroyWatcher(EventHandlerData& data)
{
    FileWatcherPlatform & p = data.data;

    for (SizeT i = 0; i < p.handles.Size(); i++)
    {
        CancelIo(p.handles[i]);
        CloseHandle(p.handles[i]);
        CloseHandle(p.overlaps[i].hEvent);
    }
    data.data.handles.Clear();
    data.data.overlaps.Clear();
    data.data.relativePathLookup.Clear();
    data.data.buffers.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void FileWatcherImpl::Init() {}

//------------------------------------------------------------------------------
/**
*/
void FileWatcherImpl::Shutdown() {}

//------------------------------------------------------------------------------
/**
*/
void FileWatcherImpl::WakeUp() {}

//------------------------------------------------------------------------------
/**
*/
void FileWatcherImpl::WaitForEvents(double timeoutSecs)
{
    Core::SysFunc::Sleep(timeoutSecs);
}

} // namespace IO
