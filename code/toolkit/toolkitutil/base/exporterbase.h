#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::ExporterBase
    
    Implements a base class for an exporter
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "io/uri.h"
#include "toolkitutil/platform.h"
#include "net/socket/socket.h"
#include "io/console.h"

typedef void (*ExporterProgressCallback) (float progress, const Util::String& status);
typedef void (*ExporterMinMaxCallback) (int min, int max);
//------------------------------------------------------------------------------
namespace Base
{
class ExporterBase : public Core::RefCounted
{
	__DeclareClass(ExporterBase);

public:
	enum ExportFlag
	{
		All,
		Dir,
		File
	};

	/// constructor
	ExporterBase();
	/// destructor
	virtual ~ExporterBase();

	/// opens the exporter
	virtual void Open();
	/// closes the exporter
	virtual void Close();
	/// returns true if exporter is open
	bool IsOpen() const;

	/// exports a single file
	virtual void ExportFile(const IO::URI& file);
	/// exports a single directory
	virtual void ExportDir(const Util::String& category);
	/// exports all files
	virtual void ExportAll();

	/// sets error flag
	void SetHasErrors(bool flag);
	/// returns error flag
	const bool HasErrors() const;

	/// sets the category
	void SetCategory(const Util::String& category);
	/// sets the file name
	void SetFile(const Util::String& file);
	/// sets the platform
	void SetPlatform(ToolkitUtil::Platform::Code platform);
	/// sets the parse flag for the parser
	void SetExportFlag(ExportFlag parseFlag);
	/// sets if the parser should force export
	void SetForce(bool force);
	/// sets if the parser should be used as a remote tool
	void SetRemote(bool remote);
	/// sets the exporter callback (only used if remote is false)
	void SetProgressCallback(ExporterProgressCallback callback);
	/// sets the min-max callback (only used if remote is false)
	void SetMinMaxCallback(ExporterMinMaxCallback callback);

	/// helper function for reporting progress
	void Progress(float progress, const Util::String& status);
	/// helper function for sending the progress min and max values
	void SetProgressMinMax(int min, int max);

	/// counts number of files to export
	int CountExports(const Util::String& dir, const Util::String& ext);

	/// sets progress precision
	void SetProgressPrecision(int precision);

protected:

	/// reports an error, depending on what read state we are in, the error will be fatal or a warning
	void ReportError(const char* error, ...);
	/// checks whether or not a file needs to be updated 
	bool NeedsConversion(const Util::String& src, const Util::String& dst);

	Util::String category;
	Util::String file;

	int precision;
	Ptr<Net::Socket> socket;
	ToolkitUtil::Platform::Code platform;
	ExportFlag exportFlag;
	ExporterProgressCallback progressCallback;
	ExporterMinMaxCallback	minMaxCallback;

	bool hasErrors;

	bool force;
	bool isOpen;
	bool remote;


}; 

//------------------------------------------------------------------------------
/**
*/
inline bool 
ExporterBase::IsOpen() const
{
	return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetHasErrors( bool flag )
{
	this->hasErrors = flag;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool 
ExporterBase::HasErrors() const
{
	return this->hasErrors;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetProgressPrecision( int precision )
{
	this->precision = precision;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetCategory( const Util::String& category )
{
	this->category = category;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetFile( const Util::String& file )
{
	this->file = file;
	this->file.StripFileExtension();
}


//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetPlatform( ToolkitUtil::Platform::Code platform )
{
	this->platform = platform;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetExportFlag( ExportFlag exportFlag )
{
	this->exportFlag = exportFlag;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetForce( bool force )
{
	this->force = force;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetRemote( bool remote )
{
	this->remote = remote;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetProgressCallback( ExporterProgressCallback callback )
{
	this->progressCallback = callback;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::SetMinMaxCallback( ExporterMinMaxCallback callback )
{
	this->minMaxCallback = callback;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
ExporterBase::ReportError( const char* error, ... )
{
	va_list argList;
	va_start(argList, error);
	if (this->exportFlag == ExporterBase::File)
	{
		IO::Console::Instance()->Error(error, argList);
	}
	else
	{
		IO::Console::Instance()->Warning(error, argList);
	}
	va_end(argList);
}


} // namespace ToolkitUtil
//------------------------------------------------------------------------------