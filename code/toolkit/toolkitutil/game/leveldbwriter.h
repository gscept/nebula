#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::LevelDbWriter
        
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "io/xmlreader.h"
#include "attributecontainer.h"
#include "math/bbox.h"
#include "levelparser.h"
#include "database.h"
namespace ToolkitUtil
{
class LevelDbWriter : public ToolkitUtil::LevelParser
{
	__DeclareClass(LevelDbWriter);
public:
	/// constructor
	LevelDbWriter();
	/// destructor
	virtual ~LevelDbWriter();

    ///
    void Open(const Ptr<Db::Database> & gameDb, const Ptr<Db::Database> & staticDb);
    ///
    void Close();
	///
	const Util::Array<Util::String>& GetReferences() const;
	///
	void SetReferenceMode(bool enable);
	///
	void ClearReferences();
protected:
    /// set level name
    virtual void SetName(const Util::String & name);
    /// parse layer information
    virtual void AddLayer(const Util::String & name, bool visible, bool autoload, bool locked);
    /// add entity
    virtual void AddEntity(const Util::String & category, const Attr::AttributeContainer & attrs);
    /// posteffect
    virtual void SetPosteffect(const Util::String & preset, const Math::matrix44 & globallightTransform);
    /// level dimensions
    virtual void SetDimensions(const Math::bbox & box);
    ///
	virtual void AddReference(const Util::String & name);
    /// commit per level info to db
    virtual void CommitLevel();

    Util::String levelname;
    Util::String postEffectPreset;
    Ptr<Db::Database> gameDb, staticDb;
    Util::Array<Util::String> layers;
    Math::bbox dimensions;
    Math::matrix44 globallightTransform;
    Util::Dictionary<Util::String,Ptr<Db::Table>> instanceTables;
    Util::Dictionary<Util::String,Ptr<Db::Dataset>> instanceDataset;
    Util::Dictionary<Util::String,Ptr<Db::ValueTable>> instanceValues;
	Util::Array<Util::String> references;
	bool inReference;
}; 


//------------------------------------------------------------------------------
/**
*/
inline
const Util::Array<Util::String>&
LevelDbWriter::GetReferences() const
{
	return this->references;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
LevelDbWriter::SetReferenceMode(bool enable)
{
	this->inReference = enable;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
LevelDbWriter::ClearReferences()
{
	this->references.Clear();
}
} // namespace ToolkitUtil
//------------------------------------------------------------------------------