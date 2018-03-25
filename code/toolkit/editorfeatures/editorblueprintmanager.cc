#include "stdneb.h"

#include "io/ioserver.h"
#include "io/xmlreader.h"
#include "util/guid.h"
#include "attr/attrid.h"
#include "editorblueprintmanager.h"
#include "attr/attributecontainer.h"
#include "managers/factorymanager.h"
#include "idldocument/idldocument.h"
#include "idldocument/idlattributelib.h"
#include "basegamefeature/basegameattr/basegameattributes.h"
#include "projectinfo.h"
#include "system/nebulasettings.h"
#include "io/xmlwriter.h"
#include "logger.h"
#include "game/templateexporter.h"
#include "db/dbfactory.h"
#include "game/leveldbwriter.h"
#include "graphicsfeature/graphicsattr/graphicsattributes.h"
#include "writer.h"
#include "posteffect/posteffectexporter.h"

using namespace Util;
using namespace Attr;
using namespace Tools;
using namespace Db;
using namespace IO;

namespace Toolkit
{
__ImplementClass(EditorBlueprintManager, 'EBPM', Core::RefCounted);
__ImplementSingleton(EditorBlueprintManager);

//------------------------------------------------------------------------------
/**
*/

EditorBlueprintManager::EditorBlueprintManager(): logger(NULL)
{
	__ConstructSingleton;		
}

//------------------------------------------------------------------------------
/**
*/
EditorBlueprintManager::~EditorBlueprintManager()
{
	__DestructSingleton;		
	this->properties.Clear();
	this->bluePrints.Clear();
	this->systemAttributeDictionary.Clear();
	this->categoryAttributes.Clear();
	this->attributes.Clear();
	this->attributeProperties.Clear();
	this->categoryProperties.Clear();
}


//------------------------------------------------------------------------------
/**
*/
void
EditorBlueprintManager::Open()
{
// 	String toolkitdir;
// 	if (System::NebulaSettings::Exists("gscept","ToolkitShared", "path"))
// 	{
// 		 toolkitdir = System::NebulaSettings::ReadString("gscept","ToolkitShared", "path");
// 		if (IO::IoServer::Instance()->FileExists(toolkitdir + "/projectinfo.xml"))
// 		{
// 			this->ParseProjectInfo(toolkitdir + "/projectinfo.xml");
// 		}		
// 		IO::AssignRegistry::Instance()->SetAssign(IO::Assign("toolkit",toolkitdir));
// 	}
//     
// 	String workdir;
// 	if (System::NebulaSettings::Exists("gscept","ToolkitShared", "workdir"))
// 	{
// 		workdir = System::NebulaSettings::ReadString("gscept","ToolkitShared", "workdir");
// 		if (IO::IoServer::Instance()->FileExists(workdir + "/projectinfo.xml"))
// 		{
// 			this->ParseProjectInfo(workdir + "/projectinfo.xml");
// 		}
// 	}
// //	if(this->useSDKDir)
// 	{
// 		workdir = toolkitdir;
// 	}
// 	String blueprintpath = workdir + "/data/tables/blueprints.xml";
// 	if (IO::IoServer::Instance()->FileExists(blueprintpath))
// 	{
// 		this->ParseBlueprint(blueprintpath);
// 	}
// 	this->UpdateAttributeProperties();
//  	this->blueprintPath = "proj:data/tables/blueprints.xml";
//  	templateDir = "proj:data/tables/db";
//  	this->ParseTemplates(templateDir);
//  	this->CreateMissingTemplates();
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::AddNIDLFile(const IO::URI & uri)
{
	Ptr<Tools::IDLDocument> doc = Tools::IDLDocument::Create();
	// no source file given?
	if (uri.IsEmpty())
	{
		return;
	}

	// parse the source file into a C++ object tree
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(uri);
	stream->SetAccessMode(IO::Stream::ReadAccess);
	if (stream->Open())
	{
		Ptr<IO::XmlReader> xmlReader = IO::XmlReader::Create();
		xmlReader->SetStream(stream);
		if (xmlReader->Open())
		{
			// check if it is a valid IDL file
			if (xmlReader->GetCurrentNodeName() == "Nebula3")
			{				
				if (doc->Parse(xmlReader))
				{
					const Util::Array<Ptr<Tools::IDLAttributeLib>>& libs = doc->GetAttributeLibs();
					IndexT i;
					for(i=0;i<libs.Size();i++)
					{
						const Util::Array<Ptr<Tools::IDLAttribute>>& attrs =  libs[i]->GetAttributes();
						IndexT a;
						for(a=0 ; a < attrs.Size();a++)
						{	
							if(!this->attributes.Contains(attrs[a]->GetName()))
							{								
								this->attributes.Add(attrs[a]->GetName(),attrs[a]);
							}
							this->systemAttributeDictionary.Add(FourCC(attrs[a]->GetFourCC()),attrs[a]->IsSystem());							

							if(!AttributeDefinitionBase::FindByName(attrs[a]->GetName()))
							{
								// attribute is unknown to nebula, add it to the dynamic attributes
								Util::String typestring = attrs[a]->GetType();
								typestring.ToLower();
								AttributeDefinitionBase::RegisterDynamicAttribute(attrs[a]->GetName(),attrs[a]->GetFourCC(),Attribute::StringToValueType(typestring),ReadWrite);
							}							
						}
					}
					const Util::Array<Ptr<Tools::IDLProperty>>& props = doc->GetProperties();
					for(i=0 ; i<props.Size();i++)
					{
						if(!this->properties.Contains(props[i]->GetName()))
						{
							this->properties.Add(props[i]->GetName(),props[i]);
						}
					}
				}
			}			
			xmlReader->Close();
		}		
		stream->Close();
	}
	else
	{
		this->logger->Error("Can't open NIDL file %s\n", uri.AsString().AsCharPtr());
	}
}


//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::ParseBlueprint(const IO::URI & filename)
{
	if (IO::IoServer::Instance()->FileExists(filename))
	{        
		Ptr<IO::XmlReader> xmlReader = IO::XmlReader::Create();
		Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(filename);
		stream->SetAccessMode(IO::Stream::ReadAccess);
		xmlReader->SetStream(stream);
		if (xmlReader->Open())
		{
			// make sure it's a BluePrints file
			if (xmlReader->GetCurrentNodeName() != "BluePrints")
			{
				this->logger->Error("FactoryManager::ParseBluePrints(): not a valid blueprints file!");
			}
			if (xmlReader->SetToFirstChild("Entity")) do
			{
				BluePrint bluePrint;
				bluePrint.type = xmlReader->GetString("type");
				bluePrint.cppClass = xmlReader->GetString("cppclass");
				if(xmlReader->HasAttr("desc"))
				{
					bluePrint.description = xmlReader->GetString("desc");
				}
				if (xmlReader->SetToFirstChild("Property")) do
				{				
					Util::String name = xmlReader->GetString("type");
					if(this->properties.Contains(name))
					{
						bluePrint.properties.Append(this->properties[name]);
					}
					if(xmlReader->HasAttr("masterOnly"))
					{
						bool remote = xmlReader->GetBool("masterOnly");
						bluePrint.remoteTable.Add(name,remote);
					}
				}
				while (xmlReader->SetToNextChild("Property"));
                if(!this->bluePrints.Contains(bluePrint.type))
                {
				    this->bluePrints.Add(bluePrint.type,bluePrint);
				    this->UpdateCategoryAttributes(bluePrint.type);				
                }
			}
			while (xmlReader->SetToNextChild("Entity"));
			xmlReader->Close();			
		}
	}	
}

//------------------------------------------------------------------------------
/**
*/
void
EditorBlueprintManager::UpdateCategoryAttributes(const Util::String & category)
{
	const BluePrint& bluePrint = this->bluePrints[category];
	/// create an attribute container with all the attributes used by this entity type based
	/// on its properties
	AttributeContainer cont;
	IndexT i;
	/// make sure Id and Name exist
	cont.AddAttr(Attribute(Attr::Id,""));
	cont.AddAttr(Attribute(Attr::Name,""));

	/// create container for property list
	Array<String> propArray;

	for(i=0;i<bluePrint.properties.Size();i++)
	{
		Ptr<IDLProperty> curprop = bluePrint.properties[i];
		propArray.Append(curprop->GetName());
		bool done = false;
		do 
		{						
			const Util::Array<Util::String>& propattrs = curprop->GetAttributes();
			IndexT j;
			for(j=0;j<propattrs.Size();j++)
			{
				if(!cont.HasAttr(AttrId(propattrs[j])))
				{
					/// FUGLY WTH

					Ptr<IDLAttribute> idlattr = this->attributes[propattrs[j]];
                    Attribute newAttr = EditorBlueprintManager::AttrFromIDL(idlattr);
                    cont.AddAttr(newAttr);					                   
				}
			}
			if(curprop->GetParentClass().IsEmpty() || curprop->GetParentClass() == "Game::Property" || !this->properties.Contains(curprop->GetParentClass()))
			{
				done = true;
			}
			else
			{
				curprop = this->properties[curprop->GetParentClass()];
			}

		} while (!done);

	}
	IndexT old = this->categoryAttributes.FindIndex(category);
	if(old != InvalidIndex )
	{
		this->categoryAttributes.EraseAtIndex(old);
	}
	this->categoryAttributes.Add(bluePrint.type,cont);	

	old = this->categoryProperties.FindIndex(category);
	if(old != InvalidIndex )
	{
		this->categoryProperties.EraseAtIndex(old);
	}
	this->categoryProperties.Add(bluePrint.type,propArray);	

}

//------------------------------------------------------------------------------
/**
*/
void
EditorBlueprintManager::UpdateAttributeProperties()
{

	// slow 
	const Array<String>& attrs = this->attributes.KeysAsArray();
	const Array<Ptr<IDLProperty>> proparray = properties.ValuesAsArray();		

	for(IndexT i = 0 ; i < attrs.Size();i++)
	{
		String id = attrs[i];
		Array<String> props;
		for(IndexT j = 0; j < proparray.Size(); j++)
		{
			bool done = false;
			Ptr<IDLProperty> curprop = proparray[j];
			do
			{
				IndexT found = curprop->GetAttributes().FindIndex(attrs[i]);
				if(found != InvalidIndex)
				{
					if(props.FindIndex(proparray[j]->GetName()) == InvalidIndex)
					{
						props.Append(proparray[j]->GetName());
					}					
				}
				if(curprop->GetParentClass().IsEmpty() || curprop->GetParentClass() == "Game::Property" || !this->properties.Contains(curprop->GetParentClass()))
				{
					done = true;
				}
				else
				{
					curprop = this->properties[curprop->GetParentClass()];
				}
			}
			while(!done);					
		}
		this->attributeProperties.Add(Attr::AttrId(attrs[i]),props);
	}	
}

//------------------------------------------------------------------------------
/**
*/
Attr::AttributeContainer
EditorBlueprintManager::GetCategoryAttributes(const Util::String & _template)
{
	n_assert2(this->categoryAttributes.Contains(_template), "unknown category");
	return this->categoryAttributes[_template];
}



//------------------------------------------------------------------------------
/**
	Creates empty template entries for all categories so that new templates
	can be added
*/
void 
EditorBlueprintManager::CreateMissingTemplates()
{
	Util::Array<Util::String> cats = this->bluePrints.KeysAsArray();
	IndexT i;
	for(i=0 ; i< cats.Size();i++)
	{
		if(!this->templates.Contains(cats[i]))
		{
			Util::Array<TemplateEntry> emptyTemplates;
			this->templates.Add(cats[i],emptyTemplates);
		}
        if(!this->categoryFlags.Contains(cats[i]))
        {
            CategoryEntry entry = { false, false };
            this->categoryFlags.Add(cats[i], entry);
        }
	}
}


//------------------------------------------------------------------------------
/**
	parse all template files in projects data folder
*/
void 
EditorBlueprintManager::ParseTemplates(const Util::String & folder)
{
	Array<String> templates = IO::IoServer::Instance()->ListFiles(folder, "*.xml", true);
	
	IndexT i;
	for(i = 0 ; i<templates.Size() ; i++)
	{
		if (IO::IoServer::Instance()->FileExists(templates[i]))
		{
			Ptr<IO::XmlReader> xmlReader = IO::XmlReader::Create();
			Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(templates[i]);
			stream->SetAccessMode(IO::Stream::ReadAccess);
			xmlReader->SetStream(stream);
            if(stream->Open())
            {
                if(stream->GetSize() == 0)
                {
                    this->logger->Warning("Empty template file: %s\n", templates[i].AsCharPtr());
                    stream->Close();
                    continue;
                }
                stream->Close();
            }
			if (xmlReader->Open())
			{
				String category = xmlReader->GetCurrentNodeName();
				CategoryEntry entry = { xmlReader->GetOptBool("IsVirtualCategory", false), xmlReader->GetOptBool("IsSpecialCategory", false) };
               
                if(entry.isVirtual || entry.isSpecial)
                {
                    AttributeContainer cont;
                    /// make sure Id and Name exist
                    cont.AddAttr(Attribute(Attr::Id,""));
                    cont.AddAttr(Attribute(Attr::Name,""));
                    if(xmlReader->SetToFirstChild("Attributes"))
                    {                    
                        Array<String> attributes = xmlReader->GetAttrs();
                        for(IndexT j = 0;j<attributes.Size();j++)
                        {
                            if(this->HasAttributeByName(attributes[j]))
                            {
                                Ptr<IDLAttribute> at = this->GetAttributeByName(attributes[j]);
                                cont.SetAttr(EditorBlueprintManager::AttrFromIDL(at));                        
                            }
                            else
                            {
                                this->logger->Warning("Unknown attribute %s\n", attributes[j].AsCharPtr());
                            }
                        }
                        if(!this->categoryAttributes.Contains(category) && !cont.GetAttrs().IsEmpty())
                        {
                            this->categoryAttributes.Add(category, cont);                        
                        }
                        xmlReader->SetToParent();
                    }                    
                }

                if((!this->categoryFlags.Contains(category) && (this->bluePrints.Contains(category)) || entry.isVirtual || entry.isSpecial))
                {
                    if(!this->categoryFlags.Contains(category)) this->categoryFlags.Add(category, entry);
                    if(!this->templateFiles.Contains(category))
                    {
                        this->templateFiles.Add(category, templates[i]);
                    }
                    else
                    {
                        this->templateFiles[category] = templates[i];
                    }
                    Util::Array<TemplateEntry> newTemplates;

                    if (xmlReader->SetToFirstChild("Item")) do
                    {
                        AttributeContainer cont = this->GetCategoryAttributes(category);
                        Array<String> attrs = xmlReader->GetAttrs();
                        IndexT j;
                        for(j=0;j<attrs.Size();j++)
                        {
                            Util::String attval = xmlReader->GetString(attrs[j].AsCharPtr());
                            if (Attr::AttrId::IsValidName(attrs[j]))
                            {
                                AttrId newId(attrs[j]);
                                Attribute newAttr(newId);
                                newAttr.SetValueFromString(attval);
                                cont.SetAttr(newAttr);
                            }
                            else
                            {
                                this->logger->Warning("Unknown attribute %s in template %s\n", attrs[j].AsCharPtr(), templates[i].AsCharPtr());
                            }
                        }
                        TemplateEntry newTemp;						
                        newTemp.attrs = cont;
                        newTemp.id = cont.GetString(Attr::Id);
                        newTemp.category = category;
                        newTemplates.Append(newTemp);
                    }
                    while (xmlReader->SetToNextChild("Item"));
                    if(!this->templates.Contains(category))
                    {
                        this->templates.Add(category,newTemplates);                    
                    }                
                    else
                    {
                        this->templates[category].AppendArray(newTemplates);
                    }
                }
				xmlReader->Close();			
			}
		}

	}
}


//------------------------------------------------------------------------------
/**
	
*/
const Attr::AttributeContainer  &  
EditorBlueprintManager::GetTemplate(const Util::String & category, const Util::String & etemplate) const
{
	const Util::Array<TemplateEntry> & templs = this->templates[category];
	/// FIXME use a dictionary
	for(IndexT i =0;i<templs.Size();i++)
	{
		if(templs[i].id == etemplate)
		{
			return templs[i].attrs;
		}
	}
	this->logger->Error("template not found in category");
	return templs[0].attrs;
}

//------------------------------------------------------------------------------
/**
*/
bool 
EditorBlueprintManager::IsSystemAttribute(Util::FourCC fourCC)
{
	if (!this->systemAttributeDictionary.Contains(fourCC))
	{
		return true;
	}

	return  this->systemAttributeDictionary[fourCC];
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::ParseProjectInfo(const Util::String & path)
{
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(IO::URI(path));
	Ptr<IO::XmlReader> xmlReader = IO::XmlReader::Create();
	xmlReader->SetStream(stream);
	if (xmlReader->Open())
	{
		// check if it's a valid project info file
		if (!xmlReader->HasNode("/Nebula3/Project"))
		{
			return;
		}
		xmlReader->SetToNode("/Nebula3/Project");		
		// parse nidl entries if any
		if (xmlReader->SetToFirstChild("NIDL"))
		{
			// load files
			if (xmlReader->SetToFirstChild("File")) do
			{
				Util::String nidlfile = path.ExtractDirName() + "/" + xmlReader->GetString("name");

				this->AddNIDLFile(IO::URI(nidlfile));				
			}
			while(xmlReader->SetToNextChild("File"));
		}
		xmlReader->Close();
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::SetCategory(const Util::String & category, const Util::Array<Util::String> newproperties )
{
	
	IndexT old = this->bluePrints.FindIndex(category);
	if(old != InvalidIndex)
	{
		this->bluePrints.EraseAtIndex(old);
	}
	else
	{
		Util::Array<TemplateEntry> emptyTemplate;
		this->templates.Add(category,emptyTemplate);
	}

	BluePrint newPrint;
	newPrint.type = category;
	newPrint.cppClass = "Entity";
	for(IndexT i = 0; i < newproperties.Size();i++)
	{
		newPrint.properties.Append(this->properties[newproperties[i]]);
	}
	this->bluePrints.Add(category,newPrint);
	this->UpdateCategoryAttributes(category);
	this->CreateMissingTemplates();
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::RemoveCategory(const Util::String & category)
{
	IndexT old = this->bluePrints.FindIndex(category);
	if(old != InvalidIndex)
	{
		this->bluePrints.EraseAtIndex(old);
	}
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<IDLAttribute>>
EditorBlueprintManager::GetAllAttributes()
{
	return this->attributes.ValuesAsArray();
}
//------------------------------------------------------------------------------
/** EditorBlueprintManager::SaveBlueprint
*/
void
EditorBlueprintManager::SaveBlueprint(const Util::String & path)
{	
	Ptr<IO::XmlWriter> xmlWriter = IO::XmlWriter::Create();
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(path);
	stream->SetAccessMode(IO::Stream::WriteAccess);
	xmlWriter->SetStream(stream);
	if (xmlWriter->Open())
	{
		xmlWriter->BeginNode("BluePrints");
		Util::Array<BluePrint> prints = this->bluePrints.ValuesAsArray();
		IndexT i;
		for(i=0 ; i<prints.Size();i++)
		{
			xmlWriter->BeginNode("Entity");
			xmlWriter->SetString("cppclass","Entity");
			xmlWriter->SetString("type",prints[i].type);
			if(!prints[i].description.IsEmpty())
			{
				xmlWriter->SetString("desc",prints[i].description);
			}
			IndexT j;
			for(j=0;j<prints[i].properties.Size();j++)
			{
				xmlWriter->BeginNode("Property");
				xmlWriter->SetString("type",prints[i].properties[j]->GetName());
				
				if(prints[i].remoteTable.Contains(prints[i].properties[j]->GetName()))
				{
					xmlWriter->SetBool("masterOnly",prints[i].remoteTable[prints[i].properties[j]->GetName()]);
				}
				xmlWriter->EndNode();
			}
			xmlWriter->EndNode();
		}
		xmlWriter->EndNode();
		xmlWriter->Close();	
	}
}

//------------------------------------------------------------------------------
/** EditorBluePrintManager::GetAttributeProperties
*/
Util::Array<Util::String> 
EditorBlueprintManager::GetAttributeProperties(const Attr::AttrId& id)
{
	return this->attributeProperties[id];
}

//------------------------------------------------------------------------------
/**
*/
void
EditorBlueprintManager::AddTemplate(const Util::String & id, const Util::String & category, const Attr::AttributeContainer & container)
{
	TemplateEntry newTemp;
	newTemp.attrs = container;
	newTemp.id = id;
	newTemp.category = category;	
	this->templates[category].Append(newTemp);
}

//------------------------------------------------------------------------------
/**
*/
void
EditorBlueprintManager::SaveCategoryTemplates(const Util::String & category)
{
	n_assert(this->templates.Contains(category));
	Util::String filename;
	if (!this->templateFiles.Contains(category))
	{
		filename = this->templateDir + "/" + category + ".xml";
		this->templateFiles.Add(category, filename);
	}
	else
	{
		filename = this->templateFiles[category];
	}

	Ptr<IO::XmlWriter> xmlWriter = IO::XmlWriter::Create();
	Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(filename);
	stream->SetAccessMode(IO::Stream::WriteAccess);
	xmlWriter->SetStream(stream);
	if (xmlWriter->Open())
	{
		xmlWriter->BeginNode(category);
		const Util::Array<TemplateEntry> & entries = this->templates[category];
		for (int i = 0; i < entries.Size(); i++)
		{
			xmlWriter->BeginNode("Item");			
			const Util::Dictionary<AttrId, Attribute>& entryDic = entries[i].attrs.GetAttrs();
			for (int j = 0; j < entryDic.Size(); j++)
			{
				// skip transform
				if (entryDic.KeyAtIndex(j) != Attr::Transform)
				{
					xmlWriter->SetString(entryDic.KeyAtIndex(j).GetName(), entryDic.ValueAtIndex(j).ValueAsString());
				}				
			}
			xmlWriter->SetString("Id", entries[i].id);
			xmlWriter->EndNode();
		}
		xmlWriter->EndNode();
		xmlWriter->Close();
	}	
#if 0
    //FIXME update this
	ToolkitUtil::Logger logger;
	Ptr<ToolkitUtil::TemplateExporter> exporter = ToolkitUtil::TemplateExporter::Create();
	exporter->SetLogger(&logger);
	exporter->SetDbFactory(Db::DbFactory::Instance());
	exporter->Open();
	exporter->ExportFile(IO::URI(filename));
	exporter->Close();
#endif
}

//------------------------------------------------------------------------------
/**
*/
bool
EditorBlueprintManager::HasTemplate(const Util::String & id, const Util::String & category)
{
	const Util::Array<TemplateEntry> & entries = this->templates[category];
	for (int i = 0; i < entries.Size(); i++)
	{
		if (entries[i].id == id)
		{
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
Attr::Attribute 
EditorBlueprintManager::AttrFromIDL( const Ptr<Tools::IDLAttribute> & attr )
{    
    AttrId newid(attr->GetName());
    switch(newid.GetValueType())
    {
    case IntType:	
        {
            Attribute newAttr(newid,0);
            if(attr->HasDefault())
            {
                newAttr.SetValueFromString(attr->GetDefault());
            }
            return newAttr;
        }									
        break;
    case FloatType:
        {
            Attribute newAttr(newid,0.0f);
            if(attr->HasDefault())
            {
                newAttr.SetValueFromString(attr->GetDefault());
            }
            return newAttr;
        }									
        break;
    case BoolType:
        {
            Attribute newAttr(newid,false);
            if(attr->HasDefault())
            {
                newAttr.SetValueFromString(attr->GetDefault());
            }
            return newAttr;
        }									
        break;
    case Float4Type:
        {
            Attribute newAttr(newid,Math::float4(0,0,0,0));
            if(attr->HasDefault())
            {
                newAttr.SetValueFromString(attr->GetDefaultRaw());
            }
            return newAttr;
        }									
        break;
    case StringType:
        {
            Attribute newAttr(newid,Util::String(""));
            if(attr->HasDefault())
            {
                newAttr.SetValueFromString(attr->GetDefaultRaw());
            }
            return newAttr;
        }									
        break;
    case Matrix44Type:
        {
            Attribute newAttr(newid,Math::matrix44::identity());
            if(attr->HasDefault())
            {
                newAttr.SetValueFromString(attr->GetDefault());
            }
           return newAttr;
        }									
        break;
    case GuidType:
        {
            Attribute newAttr(newid,Util::Guid());
            if(attr->HasDefault())
            {
                newAttr.SetValueFromString(attr->GetDefault());
            }
            return newAttr;
        }									
        break;
    default:
        {
            Attribute newAttr(newid);
            return newAttr;
        }
    }
}


//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::CreateCategoryTables( const Ptr<Db::Database> & staticDb, const Ptr<Db::Database> & instanceDb )
{
    // create global categories table object
    Ptr<Table> categoryTable = this->CreateTable(staticDb, "_Categories");
    this->CreateColumn(categoryTable, Column::Primary, Attr::CategoryName);
    this->CreateColumn(categoryTable, Column::Default, Attr::IsVirtualCategory);
    this->CreateColumn(categoryTable, Column::Default, Attr::IsSpecialCategory);
    this->CreateColumn(categoryTable, Column::Default, Attr::CategoryTemplateTable);
    this->CreateColumn(categoryTable, Column::Default, Attr::CategoryInstanceTable);
    staticDb->AddTable(categoryTable);
	
    Ptr<Dataset> categoryDataset;
    Ptr<ValueTable> categoryValues;

    categoryDataset = categoryTable->CreateDataset();
    categoryDataset->AddAllTableColumns();
    categoryValues = categoryDataset->Values();

	IndexT size = this->categoryFlags.Size();

	{
		// create level entry
		IndexT row = categoryValues->AddRow();
		categoryValues->SetString(Attr::CategoryName, row, "Levels");
		categoryValues->SetBool(Attr::IsVirtualCategory, row, true);
		categoryValues->SetBool(Attr::IsSpecialCategory, row, false);		
		categoryValues->SetString(Attr::CategoryTemplateTable, row, "_Template_Levels");
	}

    for(IndexT i = 0 ; i < size; i++)
    {
        const KeyValuePair<String, CategoryEntry> & entry = this->categoryFlags.KeyValuePairAtIndex(i);
        const String & category = entry.Key();
        // add to categories table first
        IndexT row = categoryValues->AddRow();
        categoryValues->SetString(Attr::CategoryName, row, category);
        categoryValues->SetBool(Attr::IsVirtualCategory, row, entry.Value().isVirtual);
        categoryValues->SetBool(Attr::IsSpecialCategory, row, entry.Value().isSpecial);
        if(!entry.Value().isVirtual)
        {
            categoryValues->SetString(Attr::CategoryInstanceTable, row, "_Instance_" + category);
        }
        if(!entry.Value().isSpecial)
        {
            categoryValues->SetString(Attr::CategoryTemplateTable, row, "_Template_" + category);
        }        

        const Attr::AttributeContainer & attrs = this->categoryAttributes[category];
        // create template tables and populate columns
        Ptr<Table> templateTable = this->CreateTable(staticDb, "_Template_" + category);
        // for templates Id is the primary column
        this->CreateColumn(templateTable, Column::Primary, Attr::Id);
        this->AddAttributeColumns(templateTable, attrs);
        staticDb->AddTable(templateTable);
        // FIXME, is this even required here
        templateTable->CommitChanges();

        // write templates to database
        this->WriteTemplates(templateTable, category);
        templateTable->CommitChanges();

        // create instance tables and populate columns
        Ptr<Table> instanceTable = this->CreateTable(instanceDb, "_Instance_" + category);
        // for instance tables Guid is the primary unique key
		this->CreateColumn(instanceTable, Column::Primary, Attr::Guid);
        
        // add other required columns
        this->CreateColumn(instanceTable, Column::Default, Attr::_Level);
        this->CreateColumn(instanceTable, Column::Default, Attr::_Layers);
		this->CreateColumn(instanceTable, Column::Default, Attr::_ID);
		this->CreateColumn(instanceTable, Column::Default, Attr::Id);
        this->CreateColumn(instanceTable, Column::Default, Attr::_LevelEntity);
        this->AddAttributeColumns(instanceTable, attrs);
        instanceDb->AddTable(instanceTable);
        instanceTable->CommitChanges();  
		Util::Array<Attr::AttrId> ids;
		ids.Append(Attr::Guid);
		instanceTable->CreateMultiColumnIndex(ids);
		ids.Clear();
		ids.Append(Attr::_Level);
		instanceTable->CreateMultiColumnIndex(ids);
		ids.Clear();
		ids.Append(Attr::Id);
		instanceTable->CreateMultiColumnIndex(ids);
		instanceTable->CommitChanges();
    }
    categoryDataset->CommitChanges();    
	Util::Array<Attr::AttrId> ids;
	ids.Append(Attr::CategoryName);
	categoryTable->CreateMultiColumnIndex(ids);
	categoryDataset->CommitChanges();
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::WriteAttributes( const Ptr<Db::Database> & db )
{
    Ptr<Db::Table> table;
    Ptr<Db::Dataset> dataset;
    Ptr<Db::ValueTable> valueTable;

    table = this->CreateTable(db, "_Attributes");
    this->CreateColumn(table, Column::Primary, Attr::AttrName);
    this->CreateColumn(table, Column::Default, Attr::AttrType);
    this->CreateColumn(table, Column::Default, Attr::AttrReadWrite);
    this->CreateColumn(table, Column::Default, Attr::AttrDynamic);
    db->AddTable(table);
    dataset = table->CreateDataset();
    dataset->AddAllTableColumns();	

    dataset->PerformQuery();
    valueTable = dataset->Values();

    IndexT index;
    String query;
    
    FixedArray<Attr::AttrId> attributes = Attr::AttrId::GetAllAttrIds();

    for (int attrIndex = 0; attrIndex < attributes.Size(); attrIndex++)
    {
        Attr::AttrId attribute = attributes[attrIndex];
        index = valueTable->FindRowIndexByAttr(Attr::Attribute(Attr::AttrName, attribute.GetName()));
        if (index == InvalidIndex)
        {
            index = valueTable->AddRow();
        }
        valueTable->SetString(Attr::AttrType, index, Attribute::ValueTypeToString(attribute.GetValueType()));
        valueTable->SetBool(Attr::AttrReadWrite, index, attribute.GetAccessMode() != ReadOnly);
        valueTable->SetBool(Attr::AttrDynamic, index, attribute.IsDynamic());
        valueTable->SetString(Attr::AttrName, index, attribute.GetName());		
    }
    dataset->CommitChanges();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Table> 
EditorBlueprintManager::CreateTable( const Ptr<Database>& db, const Util::String& tableName )
{
    Ptr<Table> table = 0;
    if(db->HasTable(tableName))
    {
        table = db->GetTableByName(tableName);
    }
    else
    {
        table = DbFactory::Instance()->CreateTable();
        table->SetName(tableName);
    }
    return table;
}

//------------------------------------------------------------------------------
/**
*/
void
EditorBlueprintManager::CreateColumn( const Ptr<Db::Table>& table, Db::Column::Type type, Attr::AttrId attributeId )
{
    if(false == table->HasColumn(attributeId))
    {
        Column column;
        column.SetType(type);
        column.SetAttrId(attributeId);
        table->AddColumn(column);		
    }	
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::AddAttributeColumns( Ptr<Table> table, const AttributeContainer& attrs)
{
    const Dictionary<AttrId, Attribute>& attrDic = attrs.GetAttrs();
    for(IndexT i = 0 ; i<attrDic.Size() ; i++)
    {
        this->CreateColumn(table, Column::Default, attrDic.KeyAtIndex(i));
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::WriteTemplates( const Ptr<Db::Table> & table, const Util::String & category )
{
    const Array<TemplateEntry> & templates = this->templates[category];
    Ptr<Db::Dataset> dataset;
    Ptr<Db::ValueTable> valueTable;

    dataset = table->CreateDataset();
    dataset->AddAllTableColumns();	

    dataset->PerformQuery();
    valueTable = dataset->Values();

    for(IndexT i = 0 ; i<templates.Size() ; i++)
    {
        const TemplateEntry & entry = templates[i];        
        const Dictionary<AttrId, Attribute>& attrDic = entry.attrs.GetAttrs();
        IndexT row = valueTable->AddRow();
        for(IndexT column = 0 ; column < attrDic.Size() ; column++)
        {
            if(valueTable->HasColumn(attrDic.KeyAtIndex(column)))
            {
                valueTable->SetAttr(attrDic.ValueAtIndex(column), row);
            }            
        }
    }
    dataset->CommitChanges();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Db::Database>
EditorBlueprintManager::CreateDatabase(const IO::URI & filename)
{    
	Ptr<Db::Database> db;
    if(IO::IoServer::Instance()->FileExists(filename))
    {
		if (!IO::IoServer::Instance()->DeleteFile(filename))
		{
			return db;
		}        
    }
    db = DbFactory::Instance()->CreateDatabase();
    db->SetURI(filename);
    db->SetAccessMode(Database::ReadWriteCreate);
    db->SetIgnoreUnknownColumns(true);
    db->Open();
    return db;
}

//------------------------------------------------------------------------------
/**
*/
bool 
EditorBlueprintManager::CreateDatabases(const Util::String & folder)
{
   Ptr<Database> staticDb = this->CreateDatabase(folder + "static.db4");
   Ptr<Database> gameDb = this->CreateDatabase(folder + "game.db4");

   if (!staticDb.isvalid() || !gameDb.isvalid())
   {
	   n_warning("Databases locked\n");
	   return false;
   }
   this->WriteAttributes(staticDb);
   this->WriteAttributes(gameDb);
   this->CreateCategoryTables(staticDb, gameDb);   
   this->CreateLevelTables(staticDb, "_Template_Levels");
   this->CreateLevelTables(gameDb, "_Instance_Levels");
   this->CreateScriptTables(staticDb);
   this->ExportGlobals(gameDb);
   Ptr<ToolkitUtil::PostEffectExporter> pfxExporter = ToolkitUtil::PostEffectExporter::Create();
   pfxExporter->SetDb(staticDb);
   pfxExporter->ExportAll();
   this->CreateEmptyLevel(staticDb, gameDb);
   staticDb->Close();
   gameDb->Close();
   return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::CreateLevelTables(const Ptr<Db::Database> & db, const Util::String & tableName)
{
    Ptr<Table> table = this->CreateTable(db, tableName);
    this->CreateColumn(table, Column::Primary, Attr::Id);
    this->CreateColumn(table, Column::Default, Attr::Name);
    this->CreateColumn(table, Column::Default, Attr::StartLevel);
    this->CreateColumn(table, Column::Default, Attr::_Layers);
    this->CreateColumn(table, Column::Default, Attr::WorldCenter);
    this->CreateColumn(table, Column::Default, Attr::WorldExtents);
    this->CreateColumn(table, Column::Default, Attr::PostEffectPreset);
    this->CreateColumn(table, Column::Default, Attr::GlobalLightTransform);    
    db->AddTable(table);
    table->CommitChanges();
	Util::Array<Attr::AttrId> ids;
	ids.Append(Attr::Id);	
	table->CreateMultiColumnIndex(ids);
	ids.Clear();
	ids.Append(Attr::Name);
	table->CreateMultiColumnIndex(ids);
	table->CommitChanges();
}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::CreateScriptTables(const Ptr<Db::Database> & staticDb)
{
   
    Ptr<Table> table = this->CreateTable(staticDb, "_Scripts_ConditionScripts");
    this->CreateColumn(table, Column::Primary, AttrId("ScriptName"));				
    this->CreateColumn(table, Column::Default, AttrId("ConditionBlock"));
    this->CreateColumn(table, Column::Default, AttrId("ConditionRef"));		
    staticDb->AddTable(table);
    table->CommitChanges();		

    table = this->CreateTable(staticDb, "_Scripts_Conditions");		
    this->CreateColumn(table, Column::Default, AttrId("Id"));
    this->CreateColumn(table, Column::Primary, AttrId("ConditionGUID"));
    this->CreateColumn(table, Column::Default, AttrId("ConditionContent"));
    this->CreateColumn(table, Column::Default, AttrId("ConditionType"));
    this->CreateColumn(table, Column::Default, AttrId("ConditionBlockGUID"));
    this->CreateColumn(table, Column::Default, AttrId("ConditionStatementGUID"));
    staticDb->AddTable(table);
    table->CommitChanges();

    table = this->CreateTable(staticDb, "_Scripts_Statements");
    this->CreateColumn(table, Column::Primary, AttrId("StatementGUID"));
    this->CreateColumn(table, Column::Default, AttrId("StatementContent"));
    this->CreateColumn(table, Column::Default, AttrId("StatementType"));
    this->CreateColumn(table, Column::Default, AttrId("StatementBlock"));
    this->CreateColumn(table, Column::Default, AttrId("StatementBlockGUID"));
    this->CreateColumn(table, Column::Default, AttrId("StatementRef"));
    staticDb->AddTable(table);
    table->CommitChanges();

    table = this->CreateTable(staticDb, "_Scripts_ActionScripts");
    this->CreateColumn(table, Column::Primary, AttrId("ScriptName"));
    this->CreateColumn(table, Column::Default, AttrId("StatementRef"));
    this->CreateColumn(table, Column::Default, AttrId("StatementBlock"));
    staticDb->AddTable(table);
    table->CommitChanges();

    table = this->CreateTable(staticDb, "_Script_StateMachines");
    this->CreateColumn(table, Column::Default, AttrId("Id"));
    this->CreateColumn(table, Column::Default, AttrId("StartState"));
    staticDb->AddTable(table);
    table->CommitChanges();

    table = this->CreateTable(staticDb, "_Script_StateMachineStates");
    this->CreateColumn(table, Column::Default, AttrId("Id"));
    this->CreateColumn(table, Column::Default, AttrId("MachineName"));
    this->CreateColumn(table, Column::Default, AttrId("StateName"));		
    this->CreateColumn(table, Column::Default, AttrId("OnEntryStatementRef"));
    this->CreateColumn(table, Column::Default, AttrId("OnEntryStatementBlock"));
    this->CreateColumn(table, Column::Default, AttrId("OnFrameStatementRef"));
    this->CreateColumn(table, Column::Default, AttrId("OnFrameStatementBlock"));
    this->CreateColumn(table, Column::Default, AttrId("OnExitStatementRef"));
    this->CreateColumn(table, Column::Default, AttrId("OnExitStatementBlock"));
    staticDb->AddTable(table);
    table->CommitChanges(); 

    table = this->CreateTable(staticDb, "_Script_StateTransitions");
    this->CreateColumn(table, Column::Default, AttrId("Id"));
    this->CreateColumn(table, Column::Default, AttrId("MachineName"));
    this->CreateColumn(table, Column::Default, AttrId("StateName"));
    this->CreateColumn(table, Column::Default, AttrId("ToState"));
    this->CreateColumn(table, Column::Default, AttrId("ConditionRef"));		
    this->CreateColumn(table, Column::Default, AttrId("ConditionBlock"));
    this->CreateColumn(table, Column::Default, AttrId("StatementRef"));
    this->CreateColumn(table, Column::Default, AttrId("StatementBlock"));	
    staticDb->AddTable(table);
    table->CommitChanges();        
}

//------------------------------------------------------------------------------
/**
	Exports global attributes found in data/globals.xml
*/
void 
EditorBlueprintManager::ExportGlobals(const Ptr<Db::Database> & gameDb)
{

	IO::IoServer* ioServer = IO::IoServer::Instance();

	if(ioServer->FileExists("root:data/tables/globals.xml"))
	{
		this->logger->Print("Found globals.xml, exporting...\n");
										
		Ptr<Stream> stream = IoServer::Instance()->CreateStream("root:data/tables/globals.xml");
		Ptr<XmlReader> xmlReader = XmlReader::Create();
		xmlReader->SetStream(stream);
		if(xmlReader->Open())
		{
			if(gameDb->HasTable("_Globals"))
			{
				gameDb->DeleteTable("_Globals");
			}
			// initialize a database writer
			Ptr<Db::Writer> dbWriter = Db::Writer::Create();
			dbWriter->SetDatabase(gameDb);
			dbWriter->SetTableName("_Globals");

			if(xmlReader->SetToFirstChild("Attribute"))
			{
				Util::Array<Attr::Attribute> attrs;
				do
				{
					Util::String name = xmlReader->GetString("name");
					Attr::ValueType vtype = Attr::Attribute::StringToValueType(xmlReader->GetString("type"));
					Util::String val = xmlReader->GetString("value");
					if(!Attr::AttributeDefinitionBase::FindByName(name))
					{
						Attr::AttributeDefinitionBase::RegisterDynamicAttribute(name, Util::FourCC(), vtype, Attr::ReadWrite);
					}						
					Attr::Attribute atr;
					atr.SetAttrId(Attr::AttrId(name));						
					atr.SetValueFromString(val);
					dbWriter->AddColumn(Db::Column(atr.GetAttrId()));
					attrs.Append(atr);
				}
				while(xmlReader->SetToNextChild("Attribute"));

				dbWriter->Open();
				dbWriter->BeginRow();

				for(IndexT i = 0 ; i < attrs.Size() ; i++)
				{
					const Attr::Attribute& value = attrs[i];
					const Attr::AttrId& attrId = value.GetAttrId();

					switch (value.GetValueType())
					{
					case Attr::IntType:
						dbWriter->SetInt(attrId, value.GetInt());
						break;
					case Attr::FloatType:
						dbWriter->SetFloat(attrId, value.GetFloat());
						break;
					case Attr::BoolType:
						dbWriter->SetBool(attrId, value.GetBool());
						break;
					case Attr::Float4Type:
						dbWriter->SetFloat4(attrId, value.GetFloat4());
						break;
					case Attr::StringType:
						dbWriter->SetString(attrId, value.GetString());
						break;
					case Attr::Matrix44Type:
						dbWriter->SetMatrix44(attrId, value.GetMatrix44());
						break;
					case Attr::GuidType:
						dbWriter->SetGuid(attrId, value.GetGuid());
						break;
					case Attr::BlobType:
						dbWriter->SetBlob(attrId, value.GetBlob());
						break;
					default:
						break;
					}							
				}
				dbWriter->EndRow();
				dbWriter->Close();
			}
			xmlReader->Close();				
		}					
	}
	else
	{
		this->logger->Warning("No globals.xml");
	}
	this->logger->Print("---- Done exporting global attributes ----\n");

}

//------------------------------------------------------------------------------
/**
*/
void 
EditorBlueprintManager::CreateEmptyLevel(const Ptr<Db::Database> & staticDb, const Ptr<Db::Database> & gameDb)
{
    {
        Ptr<Table> table = gameDb->GetTableByName("_Instance_Levels");
        Ptr<Db::Dataset> dataset = table->CreateDataset();
        dataset->AddAllTableColumns();
        dataset->PerformQuery();
        Ptr<ValueTable> valueTable = dataset->Values();

        IndexT row = valueTable->AddRow();

        valueTable->SetString(Attr::Id, row, "EmptyWorld");
        valueTable->SetString(Attr::Name, row, "EmptyWorld");
        valueTable->SetBool(Attr::StartLevel, row, false);
        valueTable->SetString(Attr::PostEffectPreset, row, "Default");
        valueTable->SetMatrix44(Attr::GlobalLightTransform, row, Math::matrix44());
        dataset->CommitChanges();
        table->CommitChanges();
        table = 0;

    }    
    {
        Ptr<Table> table = staticDb->GetTableByName("_Template_Levels");
        Ptr<Db::Dataset> dataset = table->CreateDataset();
        dataset->AddAllTableColumns();
        dataset->PerformQuery();
        Ptr<ValueTable> valueTable = dataset->Values();

        IndexT row = valueTable->AddRow();

        valueTable->SetString(Attr::Id, row, "EmptyWorld");
        valueTable->SetString(Attr::Name, row, "EmptyWorld");
        valueTable->SetBool(Attr::StartLevel, row, false);        
        dataset->CommitChanges();
        table->CommitChanges();
        table = 0;        
    }		    
}

//------------------------------------------------------------------------------
/**
*/
void
EditorBlueprintManager::SetTemplateTargetFolder(const Util::String & folder)
{
	this->templateDir = folder;
}

}