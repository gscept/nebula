
#include "io/filewatcher.h"
#include "io/assignregistry.h"
#include "util/win32/win32stringconverter.h"
#include "core/sysfunc.h"

//------------------------------------------------------------------------------
/**
*/
static void 
QueryDirectoryChanges(IO::EventHandlerData& data)
{
    IO::FileWatcherPlatform& p = data.data;
    Memory::Clear(&p.overlapped, sizeof(p.overlapped));
    bool res = ReadDirectoryChangesW(p.dirHandle, p.buffer, sizeof(p.buffer), p.recursive, p.notifyFilter, NULL, &p.overlapped, NULL);
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

    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(local, widePath, sizeof(widePath));

    p.dirHandle = CreateFileW((LPCWSTR)widePath,
        GENERIC_READ | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
        NULL,
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS| FILE_FLAG_OVERLAPPED, 
        NULL);
    n_assert(p.dirHandle != INVALID_HANDLE_VALUE);
    //p.overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p.notifyFilter = 0;
    p.notifyFilter |= data.flags.IsSet(NameChanged) ? FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_FILE_NAME : 0;
    p.notifyFilter |= data.flags.IsSet(SizeChanged) ? FILE_NOTIFY_CHANGE_SIZE : 0;
    p.notifyFilter |= data.flags.IsSet(Write) ? FILE_NOTIFY_CHANGE_LAST_WRITE : 0;
    p.notifyFilter |= data.flags.IsSet(Access) ? FILE_NOTIFY_CHANGE_LAST_ACCESS : 0;
    p.notifyFilter |= data.flags.IsSet(Creation) ? FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_FILE_NAME : 0;
    QueryDirectoryChanges(data);
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcherImpl::Update(EventHandlerData& data)
{
    FileWatcherPlatform& p = data.data;
    DWORD bytes;
    bool res = GetOverlappedResult(p.dirHandle, &p.overlapped, &bytes, false);
    
    if (!res) return;

        
    FILE_NOTIFY_INFORMATION* ev = (FILE_NOTIFY_INFORMATION*)p.buffer;
    do
    {
        if (ev->FileNameLength > 0)
        {
            Util::String fullRelative = Win32::Win32StringConverter::WideToUTF8((ushort*)ev->FileName, ev->FileNameLength / 2);
            fullRelative.SubstituteString("\\", "/");
            Util::String relativePath = fullRelative.ExtractToLastSlash();
            relativePath.TrimRight("/");
            Util::String file = fullRelative.ExtractFileName();
            switch (ev->Action)
            {
            case FILE_ACTION_ADDED:
            {
                data.callback({ Created, data.folder, relativePath, file });
            }
            break;
            case FILE_ACTION_MODIFIED:
            {
                data.callback({ Modified, data.folder, relativePath, file });
            }
            break;
            case FILE_ACTION_REMOVED:
            {
                data.callback({ Deleted, data.folder, relativePath, file });
            }
            break;
            case FILE_ACTION_RENAMED_NEW_NAME:
            {
                data.callback({ NameChange, data.folder, relativePath, file });
            }

            }
        }

        if (ev->NextEntryOffset == 0)
            break;
        ev = (FILE_NOTIFY_INFORMATION*)((char*)ev + ev->NextEntryOffset);
    } while (true);
    QueryDirectoryChanges(data);
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

}
