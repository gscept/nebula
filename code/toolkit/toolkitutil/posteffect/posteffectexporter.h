#pragma once
//------------------------------------------------------------------------------
/**
@class ToolkitUtil::PostEffectExporter

Exports posteffect presets 

(C) 2015-2016 Individual contributors, see AUTHORS file
*/
#include "base/exporterbase.h"
#include "db/dbfactory.h"
#include "db/database.h"
#include "logger.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class PostEffectExporter : public Base::ExporterBase
{
	__DeclareClass(PostEffectExporter);
public:
	/// constructor
	PostEffectExporter();
	/// destructor
	virtual ~PostEffectExporter();

	/// opens the exporter
	void Open();
	/// closes the exporter
	void Close();
	/// sets the database factory
	void SetDb(const Ptr<Db::Database>& staticDb);

	/// exports all presets
	void ExportAll();	
	/// set pointer to a valid logger object
	void SetLogger(Logger* logger);
private:
	/// create table and columns in case they dont exist
	void SetupTables();
	/// makes sure default preset file exists
	void CheckDefaultPreset();
	
	Ptr<Db::Database> staticDb;
	Logger* logger;	
};

//------------------------------------------------------------------------------
/**
*/
inline void
PostEffectExporter::SetDb(const Ptr<Db::Database>& staticDb)
{	
	this->staticDb = staticDb;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PostEffectExporter::SetLogger(Logger* l)
{
	this->logger = l;
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------