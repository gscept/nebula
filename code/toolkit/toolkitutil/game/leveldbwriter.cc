//------------------------------------------------------------------------------
//  leveldbwriter.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "leveldbwriter.h"
#include "dataset.h"
#include "valuetable.h"
#include "basegamefeature/basegameattr/basegameattributes.h"
#include "graphicsfeature/graphicsattr/graphicsattributes.h"
#include "editorblueprintmanager.h"

using namespace Db;
using namespace Util;
using namespace Attr;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::LevelDbWriter, 'LDBW', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
LevelDbWriter::LevelDbWriter():
	inReference(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
LevelDbWriter::~LevelDbWriter()
{
	// empty
}


//------------------------------------------------------------------------------
/**
*/
void 
LevelDbWriter::SetName(const Util::String & name)
{
	if (!this->inReference)
	{
		this->levelname = name;
	}    
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelDbWriter::AddLayer(const Util::String & name, bool visible, bool autoload, bool locked)
{
    if(autoload && !this->inReference)
    {
        this->layers.Append(name);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelDbWriter::AddEntity(const Util::String & category, const Attr::AttributeContainer & attrs)
{
    Ptr<Toolkit::EditorBlueprintManager> bm = Toolkit::EditorBlueprintManager::Instance();
    if(!bm->HasCategory(category))
    {
		// we dont warn about leveleditor builtins
		if (category[0] != '_')
		{
			n_printf("Ignoring object with category %s\n", category.AsCharPtr());
		}        
        return;
    }   

    Ptr<Db::ValueTable> valueTable;
    Util::String instanceName = "_Instance_" + category;
    if(!this->instanceValues.Contains(instanceName))
    {
        Ptr<Table> table = this->gameDb->GetTableByName(instanceName); 
        this->instanceTables.Add(instanceName, table);
        Ptr<Db::Dataset> dataset;        

        dataset = table->CreateDataset();
        dataset->AddAllTableColumns();	
        dataset->PerformQuery();
        this->instanceDataset.Add(instanceName, dataset);
        valueTable = dataset->Values();
        this->instanceValues.Add(instanceName, valueTable);
    }
    else
    {
        valueTable = this->instanceValues[instanceName];
    }
    
    IndexT row = valueTable->AddRow();
    Dictionary<AttrId,Attribute> attrDic = attrs.GetAttrs();
    for(IndexT i = 0 ; i <attrDic.Size() ; i++)
    {
        if(valueTable->HasColumn(attrDic.KeyAtIndex(i)))
        {
			if (attrDic.KeyAtIndex(i) == Attr::Guid)
			{
				Util::Guid newguid;
				newguid.Generate();
				valueTable->SetAttr(Attr::Attribute(Attr::Guid, newguid), row);
			}
			// override level field
			else if (attrDic.KeyAtIndex(i) == Attr::_Level)
			{
				valueTable->SetAttr(Attr::Attribute(Attr::_Level, this->levelname), row);
			}
			else
			{
				valueTable->SetAttr(attrDic.ValueAtIndex(i), row);
			}   
			
        }
        else
        {
            Util::String name = "unknown";
            if(attrDic.Contains(Attr::Id))
            {
                name = attrDic[Attr::Id].GetString();
            }
            n_warning("Unregistered Attribute %s in object %s in category %s\n", attrDic.KeyAtIndex(i).GetName().AsCharPtr(), name.AsCharPtr(), category.AsCharPtr());
        }
        
    }        
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelDbWriter::SetPosteffect(const Util::String & preset, const Math::matrix44 & globallightTransform)
{
	if (!this->inReference)
	{
		this->postEffectPreset = preset;
		this->globallightTransform = globallightTransform;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelDbWriter::SetDimensions(const Math::bbox & box)
{
	if (!this->inReference)	
	{
		this->dimensions = box;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelDbWriter::Close()
{
    for(IndexT i = 0 ; i < this->instanceDataset.Size() ; i++)
    {
        this->instanceDataset.ValueAtIndex(i)->CommitChanges();
        this->instanceTables.ValueAtIndex(i)->CommitChanges();
    }
    this->instanceValues.Clear();
    this->instanceDataset.Clear();
    this->instanceTables.Clear();
	this->references.Clear();
	this->inReference = false;
    this->gameDb = 0;
    this->staticDb = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelDbWriter::Open(const Ptr<Db::Database> & gameDb, const Ptr<Db::Database> & staticDb)
{
    this->gameDb = gameDb;
    this->staticDb = staticDb;
	this->inReference = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
LevelDbWriter::CommitLevel()
{
	if (this->inReference)
	{
		return;
	}	

    Ptr<Table> table = this->staticDb->GetTableByName("_Template_Levels");
    Ptr<Db::Dataset> dataset;
    Ptr<Db::ValueTable> valueTable;

    dataset = table->CreateDataset();
    dataset->AddAllTableColumns();	

    dataset->PerformQuery();
    valueTable = dataset->Values();

    Attribute id(Attr::Id,this->levelname);
    Util::Array<IndexT> idxs = valueTable->FindRowIndicesByAttr(id,true);
    IndexT row;
    if(idxs.IsEmpty())
    {
        row = valueTable->AddRow();
    }
    else
    {
        row = idxs[0];
    }
    valueTable->SetString(Attr::Id, row, this->levelname);
    valueTable->SetString(Attr::Name, row, this->levelname);
    dataset->CommitChanges();

    table = this->gameDb->GetTableByName("_Instance_Levels");

    dataset = table->CreateDataset();
    dataset->AddAllTableColumns();	

    dataset->PerformQuery();
    valueTable = dataset->Values();

    idxs = valueTable->FindRowIndicesByAttr(id,true);

    if(idxs.IsEmpty())
    {
        row = valueTable->AddRow();
    }
    else
    {
        row = idxs[0];
    }
    valueTable->SetString(Attr::Id, row, this->levelname);
    valueTable->SetString(Attr::Name, row, this->levelname);
    valueTable->SetFloat4(Attr::WorldCenter, row, this->dimensions.center());
    valueTable->SetFloat4(Attr::WorldExtents, row, this->dimensions.extents());
    valueTable->SetString(Attr::PostEffectPreset, row, this->postEffectPreset);
    valueTable->SetMatrix44(Attr::GlobalLightTransform, row, this->globallightTransform);
    valueTable->SetString(Attr::_Layers, row, String::Concatenate(this->layers,";"));
    dataset->CommitChanges();
}

//------------------------------------------------------------------------------
/**
*/
void
LevelDbWriter::AddReference(const Util::String & name)
{
	this->references.Append(name);
}

} // namespace ToolkitUtil