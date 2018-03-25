#include "stdneb.h"
#include "posteffectexporter.h"
#include "posteffectparser.h"
#include "db/dbfactory.h"
#include "base/exporterbase.h"
#include "db/database.h"
#include "attr/attrid.h"
#include "game/templateexporter.h"
#include "io/ioserver.h"
#include "editorblueprintmanager.h"

using namespace Db;
using namespace IO;
using namespace ToolkitUtil;
using namespace Toolkit;
using namespace Attr;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::PostEffectExporter, 'PEEX', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
PostEffectExporter::PostEffectExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
PostEffectExporter::~PostEffectExporter()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
PostEffectExporter::Open()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
PostEffectExporter::Close()
{	
	this->staticDb = 0;
	ExporterBase::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
PostEffectExporter::SetupTables()
{

	// make sure all required attributes exist

	if (!AttrId::IsValidName("GlobalLightTransform"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightTransform", Util::FourCC(), Matrix44Type, ReadWrite);
	if (!AttrId::IsValidName("GlobalLightAmbient"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightAmbient", Util::FourCC(), Float4Type, ReadWrite);
	if (!AttrId::IsValidName("GlobalLightDiffuse"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightDiffuse", Util::FourCC(), Float4Type, ReadWrite);
	if (!AttrId::IsValidName("GlobalLightOpposite"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightOpposite", Util::FourCC(), Float4Type, ReadWrite);
	if (!AttrId::IsValidName("GlobalLightCastShadows"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightCastShadows", Util::FourCC(), BoolType, ReadWrite);
	if (!AttrId::IsValidName("GlobalLightShadowIntensity"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightShadowIntensity", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("GlobalLightIntensity"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightIntensity", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("GlobalLightBacklightFactor"))
		AttributeDefinitionBase::RegisterDynamicAttribute("GlobalLightBacklightFactor", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("Saturation"))
		AttributeDefinitionBase::RegisterDynamicAttribute("Saturation", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("Balance"))
		AttributeDefinitionBase::RegisterDynamicAttribute("Balance", Util::FourCC(), Float4Type, ReadWrite);
	if (!AttrId::IsValidName("Luminance"))
		AttributeDefinitionBase::RegisterDynamicAttribute("Luminance", Util::FourCC(), Float4Type, ReadWrite);
	if (!AttrId::IsValidName("MaxLuminance"))
		AttributeDefinitionBase::RegisterDynamicAttribute("MaxLuminance", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("FocusDistance"))
		AttributeDefinitionBase::RegisterDynamicAttribute("FocusDistance", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("FocusLength"))
		AttributeDefinitionBase::RegisterDynamicAttribute("FocusLength", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("FocusRadius"))
		AttributeDefinitionBase::RegisterDynamicAttribute("FocusRadius", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("FogHeight"))
		AttributeDefinitionBase::RegisterDynamicAttribute("FogHeight", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("FogColor"))
		AttributeDefinitionBase::RegisterDynamicAttribute("FogColor", Util::FourCC(), Float4Type, ReadWrite);
	if (!AttrId::IsValidName("FogNearDist"))
		AttributeDefinitionBase::RegisterDynamicAttribute("FogNearDist", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("FogFarDist"))
		AttributeDefinitionBase::RegisterDynamicAttribute("FogFarDist", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("BackLightFactor"))
		AttributeDefinitionBase::RegisterDynamicAttribute("BackLightFactor", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("BloomColor"))
		AttributeDefinitionBase::RegisterDynamicAttribute("BloomColor", Util::FourCC(), Float4Type, ReadWrite);
	if (!AttrId::IsValidName("BloomThreshold"))
		AttributeDefinitionBase::RegisterDynamicAttribute("BloomThreshold", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("BloomScale"))
		AttributeDefinitionBase::RegisterDynamicAttribute("BloomScale", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("SkyTexture"))
		AttributeDefinitionBase::RegisterDynamicAttribute("SkyTexture", Util::FourCC(), StringType, ReadWrite);
	if (!AttrId::IsValidName("SkyContrast"))
		AttributeDefinitionBase::RegisterDynamicAttribute("SkyContrast", Util::FourCC(), FloatType, ReadWrite);
    if (!AttrId::IsValidName("SkyRotationFactor"))
        AttributeDefinitionBase::RegisterDynamicAttribute("SkyRotationFactor", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("SkyBrightness"))
		AttributeDefinitionBase::RegisterDynamicAttribute("SkyBrightness", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("SkyModel"))
		AttributeDefinitionBase::RegisterDynamicAttribute("SkyModel", Util::FourCC(), StringType, ReadWrite);
	if (!AttrId::IsValidName("AOStrength"))
		AttributeDefinitionBase::RegisterDynamicAttribute("AOStrength", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("AORadius"))
		AttributeDefinitionBase::RegisterDynamicAttribute("AORadius", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("AOPower"))
		AttributeDefinitionBase::RegisterDynamicAttribute("AOPower", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("AOAngleBias"))
		AttributeDefinitionBase::RegisterDynamicAttribute("AOAngleBias", Util::FourCC(), FloatType, ReadWrite);
	if (!AttrId::IsValidName("PEBlendTime"))
		AttributeDefinitionBase::RegisterDynamicAttribute("PEBlendTime", Util::FourCC(), FloatType, ReadWrite);	
    if (!AttrId::IsValidName("ProbeReflectionMap"))
        AttributeDefinitionBase::RegisterDynamicAttribute("ProbeReflectionMap", Util::FourCC(), StringType, ReadWrite);
    if (!AttrId::IsValidName("ProbeIrradianceMap"))
        AttributeDefinitionBase::RegisterDynamicAttribute("ProbeIrradianceMap", Util::FourCC(), StringType, ReadWrite);

	if (this->staticDb->HasTable("_PostEffect_Presets"))
	{
		this->staticDb->DeleteTable("_PostEffect_Presets");
	}
	
	{
		Ptr<Db::Table> table;
		Ptr<Db::Dataset> dataset;

		table = DbFactory::Instance()->CreateTable();
		table->SetName("_PostEffect_Presets");

		EditorBlueprintManager::CreateColumn(table, Column::Primary, Attr::AttrId("Id"));

		this->staticDb->AddTable(table);
		dataset = table->CreateDataset();
		dataset->AddAllTableColumns();
		dataset->CommitChanges();

		AttrId id;		
		id = AttrId("GlobalLightTransform");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("GlobalLightAmbient");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("GlobalLightDiffuse");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("GlobalLightOpposite");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("GlobalLightCastShadows");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("GlobalLightShadowIntensity");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("GlobalLightIntensity");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("GlobalLightShadowBias");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("GlobalLightBacklightFactor");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("Saturation");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("Balance");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("Luminance");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("MaxLuminance");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("FocusDistance");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("FocusLength");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("FocusRadius");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("FogHeight");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("FogColor");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("FogNearDist");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("FogFarDist");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("BloomColor");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("BloomThreshold");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("BackLightFactor");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("BloomScale");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("SkyTexture");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("SkyContrast");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
        id = AttrId("SkyRotationFactor");
        EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("SkyBrightness");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("SkyModel");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("SkyModel");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("AOStrength");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("AORadius");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("AOPower");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("AOAngleBias");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
		id = AttrId("PEBlendTime");
		EditorBlueprintManager::CreateColumn(table, Column::Default, id);
        id = AttrId("ProbeReflectionMap");
        EditorBlueprintManager::CreateColumn(table, Column::Default, id);
        id = AttrId("ProbeIrradianceMap");
        EditorBlueprintManager::CreateColumn(table, Column::Default, id);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
PostEffectExporter::CheckDefaultPreset()
{
	Ptr< IO::IoServer> ioserver = IO::IoServer::Instance();
	if (!ioserver->FileExists("root:data/tables/posteffect/Default.xml"))
	{
		this->logger->Warning("No default preset found, creating...\n");
		PostEffect::PostEffectEntity::ParamSet parms;
		parms.Init();		
		if (!IO::IoServer::Instance()->DirectoryExists("root:data/tables/posteffect"))
		{
			IO::IoServer::Instance()->CreateDirectory("root:data/tables/posteffect");
		}
		PostEffectParser::Save("root:data/tables/posteffect/Default.xml", parms);
		parms.Discard();
	}	
}
//------------------------------------------------------------------------------
/**
*/
void
ToolkitUtil::PostEffectExporter::ExportAll()
{
	// check if default preset exist and create if it doesnt
	this->CheckDefaultPreset();

	Util::Array<Util::String> files = IO::IoServer::Instance()->ListFiles(IO::URI("root:data/tables/posteffect"), "*.xml", true);
	IndexT fileIndex;

	this->SetupTables();

	

	Ptr<Db::Table> table = this->staticDb->GetTableByName("_PostEffect_Presets");

	Ptr<Dataset> dataset;
	Ptr<ValueTable> valueTable;
	dataset = table->CreateDataset();
	dataset->AddAllTableColumns();
	dataset->PerformQuery();
	valueTable = dataset->Values();

	for (fileIndex = 0; fileIndex < files.Size(); fileIndex++)
	{
		PostEffect::PostEffectEntity::ParamSet parms;
		parms.Init();
		if (ToolkitUtil::PostEffectParser::Load(files[fileIndex], parms))
		{
			IndexT row = valueTable->AddRow();
			valueTable->SetString(AttrId("Id"), row, parms.common->GetName());
			valueTable->SetFloat(AttrId("PEBlendTime"), row, parms.common->GetBlendTime());
			valueTable->SetMatrix44(AttrId("GlobalLightTransform"), row, parms.light->GetLightTransform());
			valueTable->SetFloat4(AttrId("GlobalLightDiffuse"), row, parms.light->GetLightColor());
			valueTable->SetFloat4(AttrId("GlobalLightOpposite"), row, parms.light->GetLightOppositeColor());
			valueTable->SetFloat4(AttrId("GlobalLightAmbient"), row, parms.light->GetLightAmbientColor());
			valueTable->SetBool(AttrId("GlobalLightCastShadows"), row, parms.light->GetLightCastsShadows());
			valueTable->SetFloat(AttrId("GlobalLightShadowIntensity"), row, parms.light->GetLightShadowIntensity());
			valueTable->SetFloat(AttrId("GlobalLightIntensity"), row, parms.light->GetLightIntensity());
			valueTable->SetFloat(AttrId("GlobalLightBacklightFactor"), row, parms.light->GetBackLightFactor());
			valueTable->SetFloat(AttrId("GlobalLightShadowBias"), row, parms.light->GetLightShadowBias());
			valueTable->SetFloat(AttrId("Saturation"), row, parms.color->GetColorSaturation());
			valueTable->SetFloat4(AttrId("Balance"), row, parms.color->GetColorBalance());
			valueTable->SetFloat(AttrId("MaxLuminance"), row, parms.color->GetColorMaxLuminance());
			valueTable->SetFloat(AttrId("BloomScale"), row, parms.hdr->GetHdrBloomIntensity());
			valueTable->SetFloat4(AttrId("BloomColor"), row, parms.hdr->GetHdrBloomColor());
			valueTable->SetFloat(AttrId("BloomThreshold"), row, parms.hdr->GetHdrBloomThreshold());
			valueTable->SetFloat(AttrId("FogNearDist"), row, parms.fog->GetFogNearDistance());
			valueTable->SetFloat(AttrId("FogFarDist"), row, parms.fog->GetFogFarDistance());
			valueTable->SetFloat(AttrId("FogHeight"), row, parms.fog->GetFogHeight());
			valueTable->SetFloat4(AttrId("FogColor"), row, parms.fog->GetFogColorAndIntensity());
			valueTable->SetFloat(AttrId("FocusDistance"), row, parms.dof->GetFocusDistance());
			valueTable->SetFloat(AttrId("FocusLength"), row, parms.dof->GetFocusLength());
			valueTable->SetFloat(AttrId("FocusRadius"), row, parms.dof->GetFilterSize());
			valueTable->SetFloat(AttrId("SkyContrast"), row, parms.sky->GetSkyContrast());
            valueTable->SetFloat(AttrId("SkyRotationFactor"), row, parms.sky->GetSkyRotationFactor());
			valueTable->SetFloat(AttrId("SkyBrightness"), row, parms.sky->GetSkyBrightness());
			valueTable->SetString(AttrId("SkyTexture"), row, parms.sky->GetSkyTexturePath());
            valueTable->SetString(AttrId("ProbeReflectionMap"), row, parms.sky->GetReflectanceTexturePath());
            valueTable->SetString(AttrId("ProbeIrradianceMap"), row, parms.sky->GetIrradianceTexturePath());
			valueTable->SetFloat(AttrId("AOStrength"), row, parms.ao->GetStrength());
			valueTable->SetFloat(AttrId("AORadius"), row, parms.ao->GetRadius());
			valueTable->SetFloat(AttrId("AOPower"), row, parms.ao->GetPower());
		}
		else
		{
			this->logger->Error("Failed to parse post effect preset file: %s\n", files[fileIndex].AsCharPtr());
			this->SetHasErrors(true);
		}
	}
	dataset->CommitChanges();
	table->CommitChanges();
	table = 0;
}

}