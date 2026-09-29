
#include "io/filewatcher.h"
#include "io/assignregistry.h"
#include "util/win32/win32stringconverter.h"
#include "core/sysfunc.h"

//------------------------------------------------------------------------------
/**
*/
static void 
QueryDirectoryChanges(IO::EventHandlerData& data, const IndexT index)
{
    Memory::Clear(&data.data.overlaps[index], sizeof(data.data.overlaps[index]));
    bool res = ReadDirectoryChangesW(
        data.data.handles[index], data.data.buffers[index].Begin(), data.data.buffers[index].ByteSize(), false, data.data.notifyFilter, NULL, &data.data.overlaps[index], NULL
    );
    n_assert(res);
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

    static std::function<void(IndexT& i, const Util::String& folder, const Util::String& relativePath, EventHandlerData& data)> recursiveTraverse =
        [](IndexT& i, const Util::String& folder, const Util::String& relativePath, EventHandlerData& data)
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
        n_assert(p.dirHandle != INVALID_HANDLE_VALUE);
        //p.overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        p.notifyFilter = 0;
        p.notifyFilter |= data.flags.IsSet(IO::NameChanged) ? FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME : 0;
        p.notifyFilter |= data.flags.IsSet(IO::SizeChanged) ? FILE_NOTIFY_CHANGE_SIZE : 0;
        p.notifyFilter |= data.flags.IsSet(IO::Write) ? FILE_NOTIFY_CHANGE_LAST_WRITE : 0;
        p.notifyFilter |= data.flags.IsSet(IO::Access) ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0;
        p.notifyFilter |= data.flags.IsSet(IO::Creation) ? FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_FILE_NAME : 0;

        OVERLAPPED& overlap = p.overlaps.Emplace();
        p.relativePaths.Append(relativePath);
        Util::FixedArray<BYTE>& buffer = p.buffers.Emplace();
        buffer.Resize(16 * 1024);

        QueryDirectoryChanges(data, i);
        p.relativePathLookup.Add(handle, relativePath);

        Util::Array<Util::String> dirs = IO::ListDirectories(folder);
        for (const Util::String& dir : dirs)
        {
            i++;
            recursiveTraverse(i, folder + "/" + dir, relativePath + "/" + dir, data);
        }
    };

    IndexT index = 0;
    recursiveTraverse(index, local, "", data);
    
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

                Util::String fullRelative =
                    data.folder.AsString() + "/" +
                    Win32::Win32StringConverter::WideToUTF8((ushort*)ev->FileName, ev->FileNameLength / 2);
                fullRelative.SubstituteString("\\", "/");
                Util::String relativePath = fullRelative.ExtractToLastSlash();
                relativePath.TrimRight("/");
                Util::String file = fullRelative.ExtractFileName();
                switch (ev->Action)
                {
                case FILE_ACTION_ADDED: {
                    data.callback({Created, data.folder, relativePath, file});
                    break;
                }
                case FILE_ACTION_MODIFIED: {
                    data.callback({Modified, data.folder, relativePath, file});
                    break;
                }
                case FILE_ACTION_REMOVED: {
                    data.callback({Deleted, data.folder, relativePath, file});
                    break;
                }
                case FILE_ACTION_RENAMED_NEW_NAME: {
                    data.callback({NameChange, data.folder, relativePath, file});
                    break;
                }
                }
            }

            if (ev->NextEntryOffset == 0)
                break;
            ev = (FILE_NOTIFY_INFORMATION*)((char*)ev + ev->NextEntryOffset);
        } while (true);
        QueryDirectoryChanges(data, i);

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
    CancelIo(p.dirHandle);
    CloseHandle(p.dirHandle);
    CloseHandle(p.overlapped.hEvent);
}

void FileWatcherImpl::Init() {}
void FileWatcherImpl::Shutdown() {}
void FileWatcherImpl::WakeUp() {}
void FileWatcherImpl::WaitForEvents(double timeoutSecs)
{
    Core::SysFunc::Sleep(timeoutSecs);
}

} // namespace IO
