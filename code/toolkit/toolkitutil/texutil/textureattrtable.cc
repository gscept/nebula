//------------------------------------------------------------------------------
//  textureattrtable.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "textureattrtable.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "io/xmlreader.h"
#include "io/xmlwriter.h"

// remove comment below if you want the old nebula texture attributes
// #define NEBULA3_TEXTURE_ATTRIBUTES

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;

//------------------------------------------------------------------------------
/**
*/
TextureAttrTable::TextureAttrTable() :
    valid(false)
{
    this->texAttrs.Reserve(10000);
    this->indexMap.Reserve(10000);
}

//------------------------------------------------------------------------------
/**
*/
TextureAttrTable::~TextureAttrTable()
{
    if (this->IsValid())
    {
        this->Discard();
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
TextureAttrTable::Setup(const String& path)
{
    n_assert(!this->IsValid());
    n_assert(this->texAttrs.IsEmpty());
    n_assert(this->indexMap.IsEmpty());

	IoServer* ioServer = IoServer::Instance();

#ifdef NEBULA3_TEXTURE_ATTRIBUTES
    // read the batchattributes.xml file
	Ptr<Stream> stream = ioServer->CreateStream(path + "/batchattributes.xml");
    Ptr<XmlReader> xmlReader = XmlReader::Create();
    xmlReader->SetStream(stream);
    if (xmlReader->Open())
    {
        xmlReader->SetToNode("/Textures");
        if (xmlReader->SetToFirstChild("Texture")) do
        {
            // read current attributes
            String pattern  = xmlReader->GetString("pattern");
            SizeT maxHeight = xmlReader->GetInt("maxHeight");
            SizeT maxWidth  = xmlReader->GetInt("maxWidth");
            TextureAttrs::Filter mipFilter = TextureAttrs::StringToFilter(xmlReader->GetString("mipFilter"));
            bool genMipMaps = xmlReader->GetString("mipMaps") == "Yes";
            TextureAttrs::PixelFormat rgbFormat = TextureAttrs::StringToPixelFormat(xmlReader->GetString("rgb"));
            TextureAttrs::PixelFormat rgbaFormat = TextureAttrs::StringToPixelFormat(xmlReader->GetString("rgba"));
            TextureAttrs::Filter scaleFilter = TextureAttrs::StringToFilter(xmlReader->GetString("scaleFilter"));
            TextureAttrs::Quality quality = TextureAttrs::StringToQuality(xmlReader->GetString("quality"));

            // create a new TextureAttrs object and add it to the dictionary
            TextureAttrs attrs;
            attrs.SetMaxWidth(maxWidth);
            attrs.SetMaxHeight(maxHeight);
            attrs.SetGenMipMaps(genMipMaps);
            attrs.SetRGBPixelFormat(rgbFormat);
            attrs.SetRGBAPixelFormat(rgbaFormat);
            attrs.SetMipMapFilter(mipFilter);
            attrs.SetScaleFilter(scaleFilter);
            attrs.SetQuality(quality);
            if (pattern == "*/*")
            {
                this->defaultAttrs = attrs;
            }
            else
            {
                this->texAttrs.Append(attrs);
                this->indexMap.Add(pattern, this->texAttrs.Size() - 1);
            }
        }
        while (xmlReader->SetToNextChild("Texture"));
        this->valid = true;
        return true;
    }
    else
    {
        return false;
    }
#else
	// load all texture XML-files into attribute table
	Util::Array<String> dirs, files;
	dirs = ioServer->ListDirectories(path, "*");

	IndexT i, j;
	for (i = 0; i < dirs.Size(); i++)
	{
		// get texture directory files
		files = ioServer->ListFiles(path + "/" + dirs[i], "*.xml");

		for (j = 0; j < files.Size(); j++)
		{
			Ptr<Stream> stream = ioServer->CreateStream(path + "/" + dirs[i] + "/" + files[j]);
			Ptr<XmlReader> xmlReader = XmlReader::Create();
			xmlReader->SetStream(stream);
			if (xmlReader->Open())
			{
				xmlReader->SetToNode("/Nebula3");
				xmlReader->SetToNode("Texture");

				// read current attributes
				String file = files[j];
				file.StripFileExtension();
				String pattern  = dirs[i] + "/" + file;
				SizeT maxHeight = xmlReader->GetInt("maxHeight");
				SizeT maxWidth  = xmlReader->GetInt("maxWidth");
				TextureAttrs::Filter mipFilter = TextureAttrs::StringToFilter(xmlReader->GetString("mipFilter"));
				bool genMipMaps = xmlReader->GetString("mipMaps") == "Yes";
				TextureAttrs::PixelFormat rgbFormat = TextureAttrs::StringToPixelFormat(xmlReader->GetString("rgb"));
				TextureAttrs::PixelFormat rgbaFormat = TextureAttrs::StringToPixelFormat(xmlReader->GetString("rgba"));
				TextureAttrs::Filter scaleFilter = TextureAttrs::StringToFilter(xmlReader->GetString("scaleFilter"));
				TextureAttrs::Quality quality = TextureAttrs::StringToQuality(xmlReader->GetString("quality"));
				TextureAttrs::ColorSpace colorSpace = TextureAttrs::StringToColorSpace(xmlReader->GetOptString("colorSpace", "sRGB"));

				// create a new TextureAttrs object and add it to the dictionary
				TextureAttrs attrs;
				attrs.SetMaxWidth(maxWidth);
				attrs.SetMaxHeight(maxHeight);
				attrs.SetGenMipMaps(genMipMaps);
				attrs.SetRGBPixelFormat(rgbFormat);
				attrs.SetRGBAPixelFormat(rgbaFormat);
				attrs.SetMipMapFilter(mipFilter);
				attrs.SetScaleFilter(scaleFilter);
				attrs.SetQuality(quality);
				attrs.SetColorSpace(colorSpace);
                attrs.SetTime(ioServer->GetFileWriteTime(stream->GetURI()));

				this->texAttrs.Append(attrs);
				this->indexMap.Add(pattern, this->texAttrs.Size() - 1);
			}
		}
	}

	this->defaultAttrs.SetMaxHeight(2048);
	this->defaultAttrs.SetMaxWidth(2048);
	this->defaultAttrs.SetGenMipMaps(true);
	this->defaultAttrs.SetRGBPixelFormat(TextureAttrs::DXT1C);
	this->defaultAttrs.SetRGBAPixelFormat(TextureAttrs::DXT3);
	this->defaultAttrs.SetMipMapFilter(TextureAttrs::Kaiser);
	this->defaultAttrs.SetScaleFilter(TextureAttrs::Kaiser);
	this->defaultAttrs.SetQuality(TextureAttrs::Low);
	this->defaultAttrs.SetColorSpace(TextureAttrs::sRGB);

	this->valid = true;
	return true;
#endif
}

//------------------------------------------------------------------------------
/**
*/
bool 
TextureAttrTable::Save( const Util::String& path )
{
	n_assert(this->IsValid());
	n_assert(!this->texAttrs.IsEmpty());
	n_assert(!this->indexMap.IsEmpty());

	IoServer* ioServer = IoServer::Instance();

#ifdef NEBULA3_TEXTURE_ATTRIBUTES
	// read the batchattributes.xml file
	Ptr<Stream> stream = ioServer->CreateStream(path);
	Ptr<XmlWriter> xmlWriter = XmlWriter::Create();
	xmlWriter->SetStream(stream);
	if (xmlWriter->Open())
    {
		// write Textures node
        xmlWriter->BeginNode("Textures");

		// go through attributes and write them
		IndexT i;
		for (i = 0; i < this->texAttrs.Size(); i++)
		{
			// get attributes
			const TextureAttrs& attrs = this->texAttrs[i];

			// get name of attributes
			IndexT mapIndex = this->indexMap.ValuesAsArray().FindIndex(i);
			StringAtom name = this->indexMap.KeyAtIndex(mapIndex);

			// begin texture node
			xmlWriter->BeginNode("Texture");

			// write data
			xmlWriter->SetString("pattern", name.AsString());
			xmlWriter->SetString("comment", "");
			xmlWriter->SetInt("maxHeight", attrs.GetMaxHeight());
			xmlWriter->SetInt("maxWidth", attrs.GetMaxWidth());
			xmlWriter->SetString("mipFilter", TextureAttrs::FilterToString(attrs.GetMipMapFilter()));
			xmlWriter->SetString("mipMaps", attrs.GetGenMipMaps() ? "Yes" : "No");
			xmlWriter->SetString("mipSharpen", "None");
			xmlWriter->SetString("quality", TextureAttrs::QualityToString(attrs.GetQuality()));
			xmlWriter->SetString("rgb", TextureAttrs::PixelFormatToString(attrs.GetRGBPixelFormat()));
			xmlWriter->SetString("rgba", TextureAttrs::PixelFormatToString(attrs.GetRGBAPixelFormat()));
			xmlWriter->SetString("scaleFilter", TextureAttrs::FilterToString(attrs.GetScaleFilter()));			
			xmlWriter->SetFloat("scaleX", 1.0f);
			xmlWriter->SetFloat("scaleY", 1.0f);

			// end texture node
			xmlWriter->EndNode();
		}

		// now add default
		const TextureAttrs& defaultAttrs = this->defaultAttrs;

		// begin texture node
		xmlWriter->BeginNode("Texture");

		// write data
		xmlWriter->SetString("pattern", "*/*");
		xmlWriter->SetString("comment", "Default attribute");
		xmlWriter->SetInt("maxHeight", defaultAttrs.GetMaxHeight());
		xmlWriter->SetInt("maxWidth", defaultAttrs.GetMaxWidth());
		xmlWriter->SetString("mipFilter", TextureAttrs::FilterToString(defaultAttrs.GetMipMapFilter()));
		xmlWriter->SetString("mipMaps", defaultAttrs.GetGenMipMaps() ? "Yes" : "No");
		xmlWriter->SetString("mipSharpen", "None");
		xmlWriter->SetString("quality", TextureAttrs::QualityToString(defaultAttrs.GetQuality()));
		xmlWriter->SetString("rgb", TextureAttrs::PixelFormatToString(defaultAttrs.GetRGBPixelFormat()));
		xmlWriter->SetString("rgba", TextureAttrs::PixelFormatToString(defaultAttrs.GetRGBAPixelFormat()));
		xmlWriter->SetString("scaleFilter", TextureAttrs::FilterToString(defaultAttrs.GetScaleFilter()));
		xmlWriter->SetFloat("scaleX", 1.0f);
		xmlWriter->SetFloat("scaleY", 1.0f);


		// end texture node
		xmlWriter->EndNode();

		// end Textures node
		xmlWriter->EndNode();
        return true;
    }
    else
    {
        return false;
    }
#else

	IndexT mapIndex = this->indexMap[path];
	StringAtom name = this->indexMap.KeyAtIndex(mapIndex);

	String nameString = name.AsString();
	nameString.StripFileExtension();

	// get attributes
	const TextureAttrs& attrs = this->texAttrs[mapIndex];

	String tablePath = "src:assets/" + path + ".xml";
	Ptr<Stream> stream = ioServer->CreateStream(tablePath);
	Ptr<XmlWriter> xmlWriter = XmlWriter::Create();
	xmlWriter->SetStream(stream);
	if (xmlWriter->Open())
	{
		// write main nodes
		xmlWriter->BeginNode("Nebula3");
		xmlWriter->BeginNode("Texture");

		// write data
		xmlWriter->SetString("comment", "");
		xmlWriter->SetInt("maxHeight", attrs.GetMaxHeight());
		xmlWriter->SetInt("maxWidth", attrs.GetMaxWidth());
		xmlWriter->SetString("mipFilter", TextureAttrs::FilterToString(attrs.GetMipMapFilter()));
		xmlWriter->SetString("mipMaps", attrs.GetGenMipMaps() ? "Yes" : "No");
		xmlWriter->SetString("mipSharpen", "None");
		xmlWriter->SetString("quality", TextureAttrs::QualityToString(attrs.GetQuality()));
		xmlWriter->SetString("rgb", TextureAttrs::PixelFormatToString(attrs.GetRGBPixelFormat()));
		xmlWriter->SetString("rgba", TextureAttrs::PixelFormatToString(attrs.GetRGBAPixelFormat()));
		xmlWriter->SetString("scaleFilter", TextureAttrs::FilterToString(attrs.GetScaleFilter()));			
		xmlWriter->SetString("colorSpace", TextureAttrs::ColorSpaceToString(attrs.GetColorSpace()));
		xmlWriter->SetFloat("scaleX", 1.0f);
		xmlWriter->SetFloat("scaleY", 1.0f);

		// end nodes
		xmlWriter->EndNode();
		xmlWriter->EndNode();
		xmlWriter->Close();
	}


	return true;
#endif
}


//------------------------------------------------------------------------------
/**
*/
void
TextureAttrTable::Discard()
{
    n_assert(this->IsValid());
    this->valid = false;
    this->texAttrs.Clear();
    this->indexMap.Clear();
}

//------------------------------------------------------------------------------
/**
    NOTE: the texName must have the form "category/texture.ext", so it's
    not an actual file name!
*/
bool
TextureAttrTable::HasEntry(const String& texName) const
{
    n_assert(this->IsValid());
    return this->indexMap.Contains(texName);
}

//------------------------------------------------------------------------------
/**
    NOTE: the texName must have the form "category/texture.ext", so it's
    not an actual file name!
    If no matching entry is found, the default texture attribute object 
    will be returned.
*/
const TextureAttrs&
TextureAttrTable::GetEntry(const String& texName) const
{
    n_assert(this->IsValid());
    IndexT i = this->indexMap.FindIndex(texName);
    if (InvalidIndex == i)
    {
        return this->defaultAttrs;
    }
    else
    {
        return this->texAttrs[this->indexMap.ValueAtIndex(i)];
    }
}

//------------------------------------------------------------------------------
/**
*/
const TextureAttrs&
TextureAttrTable::GetDefaultEntry() const
{
    n_assert(this->IsValid());
    return this->defaultAttrs;
}

//------------------------------------------------------------------------------
/**
*/
void 
TextureAttrTable::SetEntry( const Util::String& texName, const TextureAttrs& attrs )
{
	n_assert(this->IsValid());
	IndexT i = this->indexMap.FindIndex(texName);
	if (InvalidIndex == i)
	{
		this->indexMap.Add(texName, this->texAttrs.Size());
		this->texAttrs.Append(attrs);		
	}
	else
	{
		this->texAttrs[i] = attrs;
	}
}
} // namespace ToolkitUtil
