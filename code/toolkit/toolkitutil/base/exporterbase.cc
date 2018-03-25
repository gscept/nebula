//------------------------------------------------------------------------------
//  exporterbase.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "base/exporterbase.h"
#include "net/socket/ipaddress.h"
#include "io/ioserver.h"

using namespace ToolkitUtil;
using namespace Net;
#if __WIN32__
using namespace Win360;
#endif
using namespace IO;
using namespace Util;

namespace Base
{
__ImplementClass(Base::ExporterBase, 'EXBA', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ExporterBase::ExporterBase() : 
	hasErrors(false),
	isOpen(false),
	remote(true),
#if __WIN32__
	platform(Platform::Win32),
#elif __LINUX__
        platform(Platform::Linux),
#endif
	progressCallback(0),
	minMaxCallback(0),
	force(false)
{
	this->socket = Socket::Create();
	this->socket->SetAddress(IpAddress("localhost", 15302));
	bool opened = this->socket->Open(Socket::UDP);
	n_assert(opened);
	Socket::Result result = this->socket->Connect();
}

//------------------------------------------------------------------------------
/**
*/
ExporterBase::~ExporterBase()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ExporterBase::Open()
{
	n_assert(!this->isOpen);
	this->isOpen = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
ExporterBase::Close()
{
	n_assert(this->isOpen);
	this->isOpen = false;
	this->socket->Close();
}

//------------------------------------------------------------------------------
/**
*/
void 
ExporterBase::ExportFile( const IO::URI& file )
{
	// empty, implement in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void 
ExporterBase::ExportDir( const Util::String& category )
{
	// empty, implement in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void 
ExporterBase::ExportAll()
{
	// empty, implement in subclass!
}

//------------------------------------------------------------------------------
/**
*/
void 
ExporterBase::Progress( float progress, const Util::String& status )
{
	if (this->remote)
	{
		/// we need tag, progress, size of string, and string itself saved
		int size = sizeof(int) + sizeof(float) + sizeof(int) + status.Length()+1;
		char* package = n_new_array(char, size);
		char* chunk = package;

		*(int*)chunk = 'QPRO';
		chunk += 4;
		*(float*)chunk = progress;
		chunk += 4;
		*(int*)chunk = status.Length();
		chunk += 4;
		bool copyStatus = status.CopyToBuffer(chunk, status.Length()+1);
		n_assert(copyStatus);
		int sentBytes = 0;
		Socket::Result result = this->socket->Send(package, size, sentBytes);
		n_delete_array(package);
	}
	else if (this->progressCallback)
	{
		this->progressCallback(progress, status);
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ExporterBase::SetProgressMinMax( int min, int max )
{
	if (this->remote)
	{
		int size = sizeof(int) + sizeof(int) + sizeof(int);
		int* package = n_new_array(int, size);
		int* chunk = package;

		*chunk = 'QINT';
		chunk++;
		*chunk = min;
		chunk++;
		*chunk = max;

		int sentBytes = 0;
		Socket::Result result = this->socket->Send(package, size, sentBytes);
		n_delete_array(package);
	}
	else if (this->minMaxCallback)
	{
		this->minMaxCallback(min, max);
	}	
}

//------------------------------------------------------------------------------
/**
*/
int 
ExporterBase::CountExports( const Util::String& dir, const Util::String& ext)
{
	int count = 0;
	switch (this->exportFlag)
	{
	case All:
		{
			Array<String> dirs = IoServer::Instance()->ListDirectories(dir, "*");
			for (int catIndex = 0; catIndex < dirs.Size(); catIndex++)
			{
				String category = dir + "/" + dirs[catIndex];
				Array<String> files = IoServer::Instance()->ListFiles(category, "*." + ext);
				count += files.Size();
			}
			break;
		}
	case Dir:
		{
			Array<String> files = IoServer::Instance()->ListFiles(dir, "*." + ext);
			count += files.Size();
			break;
		}
	}
	return count;
}

//------------------------------------------------------------------------------
/**
*/
bool 
ExporterBase::NeedsConversion( const Util::String& src, const Util::String& dst )
{
	if (this->force)
	{
		return true;
	}

	IoServer* ioServer = IoServer::Instance();
	if (ioServer->FileExists(dst))
	{
		FileTime srcFileTime = ioServer->GetFileWriteTime(src);
		FileTime dstFileTime = ioServer->GetFileWriteTime(dst);
		if (dstFileTime > srcFileTime)
		{
			return false;
		}
	}

	// fallthrough
	return true;
}


} // namespace ToolkitUtil