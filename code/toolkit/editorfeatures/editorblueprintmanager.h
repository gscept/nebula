#pragma once
//------------------------------------------------------------------------------
/**
    @class LevelEditor2::EditorBlueprintManager

    Based on normal factory manager but allows handling of blueprint files
	and access to them

    (C) 2012-2016 Individual contributors, see AUTHORS file
*/

#include "basegamefeature/managers/factorymanager.h"
#include "idldocument/idlattribute.h"
#include "idldocument/idlproperty.h"
#include "attr/attributecontainer.h"
#include "toolkitutil/logger.h"

//------------------------------------------------------------------------------
namespace Toolkit
{
class EditorBlueprintManager : public Core::RefCounted
{
    __DeclareClass(EditorBlueprintManager);
    __DeclareSingleton(EditorBlueprintManager);
public:
	struct CategoryEntry
	{			
		bool isVirtual;
        bool isSpecial;
	};

	struct TemplateEntry
	{		
		Util::String id;
		Util::String category;		
		Attr::AttributeContainer attrs;
	};
	struct BluePrint
	{
		Util::String type;
		Util::String cppClass;
		Util::String description;
		Util::Array<Ptr<Tools::IDLProperty>> properties;
		Util::Dictionary<Util::String,bool> remoteTable;
	};
	/// constructor
	EditorBlueprintManager();
	/// destructor
	virtual ~EditorBlueprintManager();
	/// Open blueprintmanager, load projectinfo and all blueprints, nidl files and templates
	void Open();
	
	/// is the AttrID with the fourcc a system attribute that should not be shown to the user
	bool IsSystemAttribute(Util::FourCC);
	/// is the attribute known to the blueprintmanager
	bool HasAttributeByName(const Util::String & attr) const;
	/// find an IDLAttribute by its name
	const Ptr<Tools::IDLAttribute> & GetAttributeByName(const Util::String & attr) const;


	/// get number of known categories
	const IndexT GetNumCategories() const;
	/// get the name of category by index
	const Util::String & GetCategoryByIndex(IndexT i) const;
	
	/// get all known templates for given category, can be empty
	const Util::Array<TemplateEntry> & GetTemplates(const Util::String & category) const;
	/// get all attributes for a category/template
	const Attr::AttributeContainer & GetTemplate(const Util::String & category, const Util::String & etemplate) const;
	/// get all attributes used by a given category
	Attr::AttributeContainer GetCategoryAttributes(const Util::String & category);	
	/// get all properties used by a category
	const Util::Array<Util::String> & GetCategoryProperties(const Util::String & category) const;

	/// add a template
	void AddTemplate(const Util::String & id, const Util::String & category, const Attr::AttributeContainer & container);
	/// save category templates
	void SaveCategoryTemplates(const Util::String & category);
	/// does template exist
	bool HasTemplate(const Util::String & id, const Util::String & category);

	/// is category known to manager, aka defined in any of the used blueprint files
	const bool HasCategory(const Util::String&templ) const;
	/// gets a category blueprint object
	const BluePrint& GetCategory(const Util::String & category); 
	/// adds or updates a category
	void SetCategory(const Util::String & category, const Util::Array<Util::String> properties );
	/// remove a category
	void RemoveCategory(const Util::String & category);

	/// property is known to system
	const bool HasProperty(const Util::String & propertyName) const;
	/// get property by name
	const Ptr<Tools::IDLProperty> & GetProperty(const Util::String & propertyName);

	/// get all known properties
	Util::Array<Ptr<Tools::IDLProperty>>  GetAllProperties();
	/// get all known attributes
	Util::Array<Ptr<Tools::IDLAttribute>> GetAllAttributes();

	/// get properties containing attribute
	Util::Array<Util::String> GetAttributeProperties(const Attr::AttrId& id);

	/// set logger object
	void SetLogger(ToolkitUtil::Logger * log);

	/// save blueprint
	void SaveBlueprint(const Util::String & path);
	
	/// parse project info file for nidls
	void ParseProjectInfo(const Util::String & path);
	/// add nidl file to attribute/property database
	void AddNIDLFile(const IO::URI & filename);
	/// parse specific blueprint file
    /// will not overwrite already known categories
	void ParseBlueprint(const IO::URI & filename);
	/// parse category templates from folder
	void ParseTemplates(const Util::String & folder);
	/// set folder where to save templates in
	void SetTemplateTargetFolder(const Util::String & folder);
	/// adds empty template tables for all categories
	void CreateMissingTemplates();
	/// update attributes used by a category
	void UpdateCategoryAttributes(const Util::String & category);	
	/// update properties owning an attribute
	void UpdateAttributeProperties();

    /// create an instance of an attribute from an idl attribute
    static Attr::Attribute AttrFromIDL(const Ptr<Tools::IDLAttribute> & attr);
    ///
    static void CreateColumn(const Ptr<Db::Table>& table, Db::Column::Type type, Attr::AttrId attributeId);
    /// create both static and instance databases
    bool CreateDatabases(const Util::String & folder);    
protected:

    /// 
    void WriteAttributes(const Ptr<Db::Database> & db);
    /// 
    void CreateCategoryTables(const Ptr<Db::Database> & staticDb, const Ptr<Db::Database> & instanceDb);
    ///
    void WriteTemplates(const Ptr<Db::Table> & table, const Util::String & catgory);
    ///
    Ptr<Db::Table> CreateTable(const Ptr<Db::Database>& db, const Util::String& tableName);    
    ///
    void AddAttributeColumns(Ptr<Db::Table> table, const Attr::AttributeContainer& attrs);
    ///
    void CreateLevelTables(const Ptr<Db::Database> & db, const Util::String & tableName);
    /// 
    Ptr<Db::Database> CreateDatabase(const IO::URI & filename);
    /// FIXME: this will generate tables for scriptfeature, doesnt do anything meaningful with it
    void CreateScriptTables(const Ptr<Db::Database> & db);
    ///
    void ExportGlobals(const Ptr<Db::Database> & gameDb);
    /// create empty world
    void CreateEmptyLevel(const Ptr<Db::Database> & staticDb, const Ptr<Db::Database> & gameDb);

	Util::Dictionary<Util::String, Ptr<Tools::IDLProperty>> properties;
	Util::Dictionary<Util::String, BluePrint> bluePrints;
	Util::Dictionary<Util::FourCC, bool> systemAttributeDictionary;
	Util::Dictionary<Util::String, Util::Array<TemplateEntry>> templates;
	Util::Dictionary<Util::String, Util::String> templateFiles;
	Util::Dictionary<Util::String, Attr::AttributeContainer> categoryAttributes;
	Util::Dictionary<Util::String, Ptr<Tools::IDLAttribute>> attributes;
	Util::Dictionary<Attr::AttrId, Util::Array<Util::String>> attributeProperties;
	Util::Dictionary<Util::String, Util::Array<Util::String>> categoryProperties;
	Util::Dictionary<Util::String, CategoryEntry> categoryFlags;

	ToolkitUtil::Logger * logger;
	IO::URI blueprintPath;
	Util::String templateDir;	
};

//------------------------------------------------------------------------------
/**
*/
inline 
const IndexT
EditorBlueprintManager::GetNumCategories() const
{
	return bluePrints.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Util::String &
EditorBlueprintManager::GetCategoryByIndex(IndexT i) const
{
	return bluePrints.KeyAtIndex(i);
}
//------------------------------------------------------------------------------
/**
*/
inline
const Util::Array<Toolkit::EditorBlueprintManager::TemplateEntry> & 
EditorBlueprintManager::GetTemplates(const Util::String & category) const
{
	return this->templates[category];
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
EditorBlueprintManager::HasAttributeByName(const Util::String & attr) const
{
	return this->attributes.Contains(attr);
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Ptr<Tools::IDLAttribute> &
EditorBlueprintManager::GetAttributeByName(const Util::String & attr) const
{
	return this->attributes[attr];
}
//------------------------------------------------------------------------------
/**
*/
inline 
const bool
EditorBlueprintManager::HasCategory(const Util::String & category) const
{
	return this->templates.Contains(category);
}

//------------------------------------------------------------------------------
/**
*/
inline 
const EditorBlueprintManager::BluePrint& EditorBlueprintManager::GetCategory(const Util::String & category)
{
	return this->bluePrints[category];
}

//------------------------------------------------------------------------------
/**
*/
inline
const bool
EditorBlueprintManager::HasProperty(const Util::String & propertyName) const
{
	return this->properties.Contains(propertyName);
}

//------------------------------------------------------------------------------
/**
*/
inline
const Ptr<Tools::IDLProperty> &
EditorBlueprintManager::GetProperty(const Util::String & propertyName)
{
	return this->properties[propertyName];
}

//------------------------------------------------------------------------------
/**
*/
inline 
Util::Array<Ptr<Tools::IDLProperty>>
EditorBlueprintManager::GetAllProperties()
{
	return this->properties.ValuesAsArray();
}

//------------------------------------------------------------------------------
/**
*/
inline 
const Util::Array<Util::String> &
EditorBlueprintManager::GetCategoryProperties(const Util::String & category) const
{
	return this->categoryProperties[category];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
EditorBlueprintManager::SetLogger(ToolkitUtil::Logger * log)
{
	this->logger = log;
}
}	