#pragma once
//------------------------------------------------------------------------------
/**
    @class Material::MaterialLoader
    
    Utility for loading materials from XML.
    
    (C) 2011-2016 Individual contributors, see AUTHORS file
*/

#include "io/uri.h"
#include "io/bxmlreader.h"
#include "materials/materialpalette.h"
#include "materials/material.h"
#include "util/dictionary.h"
#include "util/stack.h"

namespace Materials
{

class MaterialLoader
{
public:
	/// load materials from an XML file
	static Ptr<MaterialPalette> LoadMaterialPalette(const Resources::ResourceName& name, const IO::URI& uri, bool optional = false);
private:
	/// parse palette from XML
	static void ParsePalette(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<MaterialPalette>& materialPalette);
	/// parse material from XML
	static void ParseMaterial(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<MaterialPalette>& materialPalette);
	/// parse material parameter from XML
	static void ParseParameter(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<Material>& material);
	/// parse material pass from XML
	static void ParseMaterialPass(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<Material>& material);
	/// parse dependency from XML
	static void ParseDependency(const Ptr<IO::BXmlReader>& xmlReader, const Ptr<MaterialPalette>& material);

	static Util::Stack<IO::URI> LoadingStack;
};

} // namespace Materials
//------------------------------------------------------------------------------