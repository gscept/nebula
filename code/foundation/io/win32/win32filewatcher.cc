#include "foundation/stdneb.h"
#include "io/filewatcher.h"
#include "io/assignregistry.h"
#include "memory/memory.h"
#include "util/win32/win32stringconverter.h"

//------------------------------------------------------------------------------
/**
*/
static void 
QueryDirectoryChanges(IO::EventHandlerData& data)
{
    IO::FileWatcherPlatform & p = data.data;
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
    Util::String local = IO::AssignRegistry::Instance()->ResolveAssigns(data.folder).LocalPath();
    FileWatcherPlatform & p = data.data;

    ushort widePath[1024];
    Win32::Win32StringConverter::UTF8ToWide(local, widePath, sizeof(widePath));

    p.dirHandle = CreateFileW((LPCWSTR)widePath,
        FILE_LIST_DIRECTORY, 
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 
        NULL,
        OPEN_EXISTING, 
        FILE_FLAG_BACKUP_SEMANTICS| FILE_FLAG_OVERLAPPED, 
        NULL);
    n_assert(p.dirHandle != INVALID_HANDLE_VALUE);
    p.overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    p.notifyFilter = FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_FILE_NAME;
    QueryDirectoryChanges(data);
}

//------------------------------------------------------------------------------
/**
*/
void
FileWatcherImpl::Update(EventHandlerData& data)
{
    DWORD bytes;
    FileWatcherPlatform & p = data.data;

    bool res = false;
    
    res = GetOverlappedResult(p.dirHandle, &p.overlapped, &bytes, false);

    if (res)
    {
        FILE_NOTIFY_INFORMATION* ev = (FILE_NOTIFY_INFORMATION*)p.buffer;
        do
        {
            switch (ev->Action)
            {
            case FILE_ACTION_ADDED:
            {
                Util::String file = Win32::Win32StringConverter::WideToUTF8((ushort*)ev->FileName);
                data.callback({ Created, data.folder, file });
            }
            break;
            case FILE_ACTION_MODIFIED:
            {
                Util::String file = Win32::Win32StringConverter::WideToUTF8((ushort*)ev->FileName);
                data.callback({ Modified, data.folder, file });
            }
            break;
            case FILE_ACTION_REMOVED:
            {
                Util::String file = Win32::Win32StringConverter::WideToUTF8((ushort*)ev->FileName);
                data.callback({ Deleted, data.folder, file });
            }
            break;
                    
            }
            if (ev->NextEntryOffset == 0)
                break;
            ev = (FILE_NOTIFY_INFORMATION*)((char*)ev + ev->NextEntryOffset);
        } while (true);

    }    
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

}
