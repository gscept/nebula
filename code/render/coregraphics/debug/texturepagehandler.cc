//------------------------------------------------------------------------------
//  texturepagehandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/debug/texturepagehandler.h"
#include "coregraphics/texture.h"
#include "http/html/htmlpagewriter.h"
#include "resources/resourcemanager.h"
#include "coregraphics/streamtexturesaver.h"
#include "io/ioserver.h"
#include "coregraphics/imagefileformat.h"

#include "coregraphics/streamtexturepool.h"
#include "coregraphics/memorytexturepool.h"

namespace Debug
{
__ImplementClass(Debug::TexturePageHandler, 'DTXH', Http::HttpRequestHandler);

using namespace IO;
using namespace CoreGraphics;
using namespace Util;
using namespace Http;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
TexturePageHandler::TexturePageHandler()
{
    this->SetName("Textures");
    this->SetDesc("show debug information about texture resources");
    this->SetRootLocation("texture");
}

//------------------------------------------------------------------------------
/**
*/
void
TexturePageHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    n_assert(HttpMethod::Get == request->GetMethod());

    // first check if a command has been defined in the URI
    Dictionary<String,String> query = request->GetURI().ParseQuery();
    if (query.Contains("img"))
    {
        request->SetStatus(this->HandleImageRequest(query, request->GetResponseContentStream()));
        return;
    }
    else if (query.Contains("texinfo"))
    {
        request->SetStatus(this->HandleTextureInfoRequest(ResourceId(query["texinfo"].AsInt()), request->GetResponseContentStream()));
        return;
    }

    // no command, send the Texture home page
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("NebulaT Textures");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Texture Resources (stream loaded)");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();

		const Ptr<StreamTexturePool>& streamPool = ResourceManager::Instance()->GetStreamPool<StreamTexturePool>();
		const Util::Dictionary<Resources::ResourceName, Ids::Id24>& streamResources = streamPool->GetResources();

		// create a table of all existing textures
		htmlWriter->AddAttr("border", "1");
		htmlWriter->AddAttr("rules", "cols");
		htmlWriter->Begin(HtmlElement::Table);
		htmlWriter->AddAttr("bgcolor", "lightsteelblue");
		htmlWriter->Begin(HtmlElement::TableRow);
		htmlWriter->Element(HtmlElement::TableHeader, "ResId");
		htmlWriter->Element(HtmlElement::TableHeader, "State");
		htmlWriter->Element(HtmlElement::TableHeader, "UseCount");
		htmlWriter->Element(HtmlElement::TableHeader, "Type");
		htmlWriter->Element(HtmlElement::TableHeader, "Width");
		htmlWriter->Element(HtmlElement::TableHeader, "Height");
		htmlWriter->Element(HtmlElement::TableHeader, "Depth");
		htmlWriter->Element(HtmlElement::TableHeader, "Mips");
		htmlWriter->Element(HtmlElement::TableHeader, "Format");
		htmlWriter->End(HtmlElement::TableRow);

		// iterate over shared resources
		IndexT i;
		for (i = 0; i < streamResources.Size(); i++)
		{
			const Ids::Id24& res = streamResources.ValueAtIndex(i);
			const ResourceName& resName = streamResources.KeyAtIndex(i);
			Resource::State state = streamPool->GetState(res);
			htmlWriter->Begin(HtmlElement::TableRow);
			if (state == Resource::Loaded)
			{
				// only loaded texture can be inspected
				htmlWriter->Begin(HtmlElement::TableData);
				htmlWriter->AddAttr("href", "/texture?texinfo=" + resName.AsString());
				htmlWriter->Element(HtmlElement::Anchor, resName.Value());
				htmlWriter->End(HtmlElement::TableData);
			}
			else
			{
				htmlWriter->Element(HtmlElement::TableData, resName.Value());
			}

			String resState;

			switch (state)
			{
			case Resource::Loaded:      resState = "Loaded"; break;
			case Resource::Pending:     resState = "Pending"; break;
			case Resource::Failed:      resState = "FAILED"; break;
			case Resource::Unloaded:	resState = "Unloaded"; break;
			default:                    resState = "CANT HAPPEN"; break;
			}
			htmlWriter->Element(HtmlElement::TableData, resState);
			if (state == Resource::Loaded)
			{
				const uint usage = streamPool->GetUsage(res);
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(usage));
				TextureType type = TextureGetType(res);
				switch (type)
				{
				case Texture1D:    htmlWriter->Element(HtmlElement::TableData, "1D"); break;
				case Texture2D:    htmlWriter->Element(HtmlElement::TableData, "2D"); break;
				case Texture3D:    htmlWriter->Element(HtmlElement::TableData, "3D"); break;
				case TextureCube:  htmlWriter->Element(HtmlElement::TableData, "CUBE"); break;
				default:           htmlWriter->Element(HtmlElement::TableData, "ERROR"); break;
				}
				TextureDimensions dims = TextureGetDimensions(res);
				CoreGraphics::PixelFormat::Code fmt = TextureGetPixelFormat(res);
				uint mips = TextureGetNumMips(res);
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(dims.width));
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(dims.height));
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(dims.depth));
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(mips));
				htmlWriter->Element(HtmlElement::TableData, PixelFormat::ToString(fmt));
			}
			else
			{
				// texture not currently loaded
			}
			htmlWriter->End(HtmlElement::TableRow);
		}
		htmlWriter->End(HtmlElement::Table);

		const Ptr<MemoryTexturePool>& memPool = ResourceManager::Instance()->GetMemoryPool<MemoryTexturePool>();
		const Util::Dictionary<Resources::ResourceName, Ids::Id24>& memResources = memPool->GetResources();

		htmlWriter->Element(HtmlElement::Heading1, "Texture Resources (memory loaded)");
		htmlWriter->AddAttr("href", "/index.html");
		htmlWriter->Element(HtmlElement::Anchor, "Home");
		htmlWriter->LineBreak();
		htmlWriter->LineBreak();

		// create a table of all existing textures
		htmlWriter->AddAttr("border", "1");
		htmlWriter->AddAttr("rules", "cols");
		htmlWriter->Begin(HtmlElement::Table);
		htmlWriter->AddAttr("bgcolor", "lightsteelblue");
		htmlWriter->Begin(HtmlElement::TableRow);
		htmlWriter->Element(HtmlElement::TableHeader, "ResId");
		htmlWriter->Element(HtmlElement::TableHeader, "State");
		htmlWriter->Element(HtmlElement::TableHeader, "UseCount");
		htmlWriter->Element(HtmlElement::TableHeader, "Type");
		htmlWriter->Element(HtmlElement::TableHeader, "Width");
		htmlWriter->Element(HtmlElement::TableHeader, "Height");
		htmlWriter->Element(HtmlElement::TableHeader, "Depth");
		htmlWriter->Element(HtmlElement::TableHeader, "Mips");
		htmlWriter->Element(HtmlElement::TableHeader, "Format");
		htmlWriter->End(HtmlElement::TableRow);

		// iterate over shared resources
		IndexT i;
		for (i = 0; i < memResources.Size(); i++)
		{
			const Ids::Id24& res = memResources.ValueAtIndex(i);
			const ResourceName& resName = memResources.KeyAtIndex(i);
			Resource::State state = streamPool->GetState(res);
			htmlWriter->Begin(HtmlElement::TableRow);
			if (state == Resource::Loaded)
			{
				// only loaded texture can be inspected
				htmlWriter->Begin(HtmlElement::TableData);
				htmlWriter->AddAttr("href", "/texture?texinfo=" + resName.AsString());
				htmlWriter->Element(HtmlElement::Anchor, resName.Value());
				htmlWriter->End(HtmlElement::TableData);
			}
			else
			{
				htmlWriter->Element(HtmlElement::TableData, resName.Value());
			}

			String resState;

			switch (state)
			{
			case Resource::Loaded:      resState = "Loaded"; break;
			case Resource::Pending:     resState = "Pending"; break;
			case Resource::Failed:      resState = "FAILED"; break;
			case Resource::Unloaded:	resState = "Unloaded"; break;
			default:                    resState = "CANT HAPPEN"; break;
			}
			htmlWriter->Element(HtmlElement::TableData, resState);
			if (state == Resource::Loaded)
			{
				const uint usage = memPool->GetUsage(res);
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(usage));
				TextureType type = TextureGetType(res);
				switch (type)
				{
				case Texture1D:    htmlWriter->Element(HtmlElement::TableData, "1D"); break;
				case Texture2D:    htmlWriter->Element(HtmlElement::TableData, "2D"); break;
				case Texture3D:    htmlWriter->Element(HtmlElement::TableData, "3D"); break;
				case TextureCube:  htmlWriter->Element(HtmlElement::TableData, "CUBE"); break;
				default:           htmlWriter->Element(HtmlElement::TableData, "ERROR"); break;
				}
				TextureDimensions dims = TextureGetDimensions(res);
				CoreGraphics::PixelFormat::Code fmt = TextureGetPixelFormat(res);
				uint mips = TextureGetNumMips(res);
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(dims.width));
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(dims.height));
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(dims.depth));
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(mips));
				htmlWriter->Element(HtmlElement::TableData, PixelFormat::ToString(fmt));
			}
			else
			{
				// texture not currently loaded
			}
			htmlWriter->End(HtmlElement::TableRow);
		}
		htmlWriter->End(HtmlElement::Table);
      
        htmlWriter->Close();
        request->SetStatus(HttpStatus::OK);
    }
    else
    {
        request->SetStatus(HttpStatus::InternalServerError);
    }
}

//------------------------------------------------------------------------------
/**
    Handle a "raw" texture image request.
*/
HttpStatus::Code
TexturePageHandler::HandleImageRequest(const Dictionary<String,String>& query, const Ptr<Stream>& responseStream)
{
    n_assert(query.Contains("img"));
    const Ptr<ResourceManager>& resManager = ResourceManager::Instance();
    
    // get input args
	ResourceName resId = ResourceName(query["img"]);
    ImageFileFormat::Code format = ImageFileFormat::InvalidImageFileFormat;
    if (query.Contains("fmt"))
    {
        format = ImageFileFormat::FromString(query["fmt"]);
    }
    if (ImageFileFormat::InvalidImageFileFormat == format)
    {
        format = ImageFileFormat::PNG;
    }
    IndexT mipLevel = 0;
    if (query.Contains("mip"))
    {
        mipLevel = query["mip"].AsInt();
    }

    // check if the request resource exists and is a texture
    if (!resManager->HasResource(resId))
    {
        return HttpStatus::NotFound;
    }
    const Ptr<Resource>& res = resManager->LookupResource(resId);
    if (!res->IsA(Texture::RTTI))
    {
        // resource exists but is not a texture
        return HttpStatus::NotFound;
    }
    const Ptr<Texture>& tex = res.downcast<Texture>();

    // attach a StreamTextureSaver to the texture
    // NOTE: the StreamSaver is expected to set the media type on the stream!
    HttpStatus::Code httpStatus = HttpStatus::InternalServerError;
    Ptr<StreamTextureSaver> saver = StreamTextureSaver::Create();
    saver->SetStream(responseStream);
    saver->SetFormat(format);
    saver->SetMipLevel(mipLevel);
    tex->SetSaver(saver.upcast<ResourceSaver>());
    if (tex->Save())
    {
        httpStatus = HttpStatus::OK;
    }
    tex->SetSaver(0);
    return httpStatus;
}

//------------------------------------------------------------------------------
/**
    Handle a texture info request.
*/
HttpStatus::Code
TexturePageHandler::HandleTextureInfoRequest(const ResourceId& resId, const Ptr<Stream>& responseContentStream)
{
    // lookup the texture in the ResourceManager
    const Ptr<ResourceManager>& resManager = ResourceManager::Instance();
	resManager->GetId
    if (!resManager->HasResource(resId))
    {
        return HttpStatus::NotFound;
    }
    const Ptr<Resource>& res = resManager->LookupResource(resId);
    if (!res->IsA(Texture::RTTI))
    {
        // resource exists but is not a texture
        return HttpStatus::NotFound;
    }
    const Ptr<Texture>& tex = res.downcast<Texture>();

    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(responseContentStream);
    htmlWriter->SetTitle("NebulaT Texture Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, resId.Value());
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("href", "/texture");
        htmlWriter->Element(HtmlElement::Anchor, "Textures Home");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();
    
        // display some info about the texture
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Resource Id: ");
                htmlWriter->Element(HtmlElement::TableData, resId.Value());
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Resolved Path: ");
                htmlWriter->Element(HtmlElement::TableData, AssignRegistry::Instance()->ResolveAssigns(resId.Value()).AsString());
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Use Count: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(tex->GetUseCount()));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Type: ");
                switch (tex->GetType())
                {
                    case Texture::Texture2D:    htmlWriter->Element(HtmlElement::TableData, "2D"); break;
                    case Texture::Texture3D:    htmlWriter->Element(HtmlElement::TableData, "3D"); break;
                    case Texture::TextureCube:  htmlWriter->Element(HtmlElement::TableData, "CUBE"); break;
                    default:                    htmlWriter->Element(HtmlElement::TableData, "ERROR"); break;
                }
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Width: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(tex->GetWidth()));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Height: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(tex->GetHeight()));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Depth: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(tex->GetDepth()));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Mip Levels: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(tex->GetNumMipLevels()));
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Pixel Format: ");
                htmlWriter->Element(HtmlElement::TableData, PixelFormat::ToString(tex->GetPixelFormat()));
            htmlWriter->End(HtmlElement::TableRow);
        htmlWriter->End(HtmlElement::Table);
        htmlWriter->LineBreak();

        // display the texture image data
        IndexT mipLevel;
        for (mipLevel = 0; mipLevel < tex->GetNumMipLevels(); mipLevel++)
        {
            String fmt;
            fmt.Format("/texture?img=%s&mip=%d", resId.Value(), mipLevel);
            htmlWriter->AddAttr("src", fmt);
            htmlWriter->Element(HtmlElement::Image, "");
        }
        htmlWriter->Close();
        return HttpStatus::OK;
    }
    return HttpStatus::InternalServerError;
}

} // namespace CoreGraphics
