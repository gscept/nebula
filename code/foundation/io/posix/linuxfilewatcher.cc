//------------------------------------------------------------------------------
//  linuxfilewatcher.cc
//  (C) 2019-2020 Individual contributors, see AUTHORS file
//---------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/filewatcher.h"
#include "io/assignregistry.h"
#include "io/fswrapper.h"

#include <errno.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <unistd.h>

namespace
{
using namespace IO;
using namespace Util;

static uint32_t
CreateNotifyMask(Util::BitField<8> flags)
{
    uint32_t mask = 0;
    if (flags.IsSet(NameChanged))
        mask |= IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE_SELF | IN_MOVE_SELF;
    if (flags.IsSet(SizeChanged))
        mask |= IN_MODIFY | IN_ATTRIB;
    if (flags.IsSet(Write))
        mask |= IN_MODIFY | IN_CLOSE_WRITE;
    if (flags.IsSet(Access))
        mask |= IN_ACCESS;
    if (flags.IsSet(Creation))
        mask |= IN_CREATE | IN_MOVED_TO;

    // Keep watch state coherent when directories are removed/moved.
    mask |= IN_DELETE_SELF | IN_MOVE_SELF | IN_IGNORED;
    return mask;
}

static void
AddWatchRecursive(FileWatcherPlatform& p, const String& absolutePath, const String& relativePath, uint32_t mask)
{
    int wd = inotify_add_watch(p.inotifyFd, absolutePath.AsCharPtr(), mask);
    if (wd < 0)
        return;

    if (!p.wdToRelativePath.Contains(wd))
        p.wdToRelativePath.Add(wd, relativePath);
    else
        p.wdToRelativePath[wd] = relativePath;

    if (!p.recursive)
        return;

    Util::Array<String> subDirs = IO::FSWrapper::ListDirectories(absolutePath, "*");
    for (IndexT i = 0; i < subDirs.Size(); i++)
    {
        const String subDir = subDirs[i];
        const String subAbsPath = String::AppendPath(absolutePath, subDir);
        const String subRelPath = String::AppendPath(relativePath, subDir);
        AddWatchRecursive(p, subAbsPath, subRelPath, mask);
    }
}
}

namespace IO
{

int FileWatcherImpl::epollFd = -1;
int FileWatcherImpl::wakeupFd = -1;

void
FileWatcherImpl::Init()
{
    epollFd = epoll_create1(EPOLL_CLOEXEC);
    n_assert(epollFd >= 0);
    wakeupFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    n_assert(wakeupFd >= 0);
    epoll_event ev = {};
    ev.events = EPOLLIN;
    ev.data.fd = wakeupFd;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, wakeupFd, &ev);
}

void
FileWatcherImpl::Shutdown()
{
    if (epollFd >= 0)
    {
        close(epollFd);
        epollFd = -1;
    }
    if (wakeupFd >= 0)
    {
        close(wakeupFd);
        wakeupFd = -1;
    }
}

void
FileWatcherImpl::WaitForEvents(double timeoutSecs)
{
    epoll_event events[64];
    int timeoutMs = (int)(timeoutSecs * 1000.0);
    int n = epoll_wait(epollFd, events, 64, timeoutMs);
    for (int i = 0; i < n; i++)
    {
        if (events[i].data.fd == wakeupFd)
        {
            uint64_t val;
            (void)read(wakeupFd, &val, sizeof(val));
            break;
        }
    }
}

void
FileWatcherImpl::WakeUp()
{
    if (wakeupFd >= 0)
    {
        uint64_t val = 1;
        (void)write(wakeupFd, &val, sizeof(val));
    }
}

 void 
 FileWatcherImpl::CreateWatcher(EventHandlerData& data)
 {
     FileWatcherPlatform& p = data.data;
     p.rootPath = IO::AssignRegistry::Instance()->ResolveAssigns(data.folder.AsString()).LocalPath();
     p.wdToRelativePath.Clear();

     p.inotifyFd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
     n_assert(p.inotifyFd >= 0);

     if (epollFd >= 0)
     {
         epoll_event epollEv = {};
         epollEv.events = EPOLLIN;
         epollEv.data.fd = p.inotifyFd;
         epoll_ctl(epollFd, EPOLL_CTL_ADD, p.inotifyFd, &epollEv);
     }

     const uint32_t mask = CreateNotifyMask(data.flags);
     AddWatchRecursive(p, p.rootPath, "", mask);
 }

void 
FileWatcherImpl::DestroyWatcher(EventHandlerData& data)
{
    FileWatcherPlatform& p = data.data;
    if (p.inotifyFd >= 0)
    {
        Util::Array<int> watchIds = p.wdToRelativePath.KeysAsArray();
        for (IndexT i = 0; i < watchIds.Size(); i++)
        {
            inotify_rm_watch(p.inotifyFd, watchIds[i]);
        }
        if (epollFd >= 0)
            epoll_ctl(epollFd, EPOLL_CTL_DEL, p.inotifyFd, nullptr);
        close(p.inotifyFd);
        p.inotifyFd = -1;
    }
    p.wdToRelativePath.Clear();
    p.rootPath.Clear();
}

void 
FileWatcherImpl::Update(EventHandlerData& data)
{
    FileWatcherPlatform& p = data.data;
    if (p.inotifyFd < 0)
        return;

    char buffer[16 * 1024];
    ssize_t bytes = read(p.inotifyFd, buffer, sizeof(buffer));
    if (bytes <= 0)
    {
        if (bytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            n_warning("LinuxFileWatcher: failed reading inotify events for '%s'", p.rootPath.AsCharPtr());
        }
        return;
    }

    const uint32_t mask = CreateNotifyMask(data.flags);
    size_t offset = 0;
    while (offset < (size_t)bytes)
    {
        const inotify_event* ev = (const inotify_event*)(buffer + offset);

        String relDir;
        if (p.wdToRelativePath.Contains(ev->wd))
            relDir = p.wdToRelativePath[ev->wd];

        String relativePath = relDir;
        String file;
        if (ev->len > 0 && ev->name[0] != 0)
            file = String(ev->name);

        if ((ev->mask & IN_ISDIR) && p.recursive && (ev->mask & (IN_CREATE | IN_MOVED_TO)) && !file.IsEmpty())
        {
            const String dirRelPath = relativePath.IsEmpty() ? file : String::AppendPath(relativePath, file);
            const String absPath = String::AppendPath(p.rootPath, dirRelPath);
            AddWatchRecursive(p, absPath, dirRelPath, mask);
        }

        if (ev->mask & IN_IGNORED)
        {
            if (p.wdToRelativePath.Contains(ev->wd))
                p.wdToRelativePath.Erase(ev->wd);
            offset += sizeof(inotify_event) + ev->len;
            continue;
        }

        if (ev->mask & IN_DELETE)
        {
            data.callback({ Deleted, data.folder, relativePath, file });
        }
        else if (ev->mask & (IN_MOVED_FROM | IN_MOVED_TO))
        {
            data.callback({ NameChange, data.folder, relativePath, file });
        }
        else if (ev->mask & IN_CREATE)
        {
            data.callback({ Created, data.folder, relativePath, file });
        }
        else if (ev->mask & (IN_MODIFY | IN_CLOSE_WRITE | IN_ATTRIB))
        {
            data.callback({ Modified, data.folder, relativePath, file });
        }

        if (ev->mask & (IN_DELETE_SELF | IN_MOVE_SELF))
        {
            if (p.wdToRelativePath.Contains(ev->wd))
                p.wdToRelativePath.Erase(ev->wd);
        }

        offset += sizeof(inotify_event) + ev->len;
    }
}
}