//------------------------------------------------------------------------------
//  shaderpagehandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/debug/shaderpagehandler.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shader.h"
#include "http/html/htmlpagewriter.h"
#include "io/ioserver.h"

namespace Debug
{
__ImplementClass(Debug::ShaderPageHandler, 'SPHL', Http::HttpRequestHandler);

using namespace IO;
using namespace CoreGraphics;
using namespace Graphics;
using namespace Util;
using namespace Http;
using namespace Resources;
using namespace Models;

//------------------------------------------------------------------------------
/**
*/
ShaderPageHandler::ShaderPageHandler()
{
    this->SetName("Shaders");
    this->SetDesc("show shader debug information");
    this->SetRootLocation("shader");
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderPageHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    n_assert(HttpMethod::Get == request->GetMethod());

    // first check if a command has been defined in the URI
    Dictionary<String,String> query = request->GetURI().ParseQuery();
    if (query.Contains("shaderinfo"))
    {
        request->SetStatus(this->HandleShaderInfoRequest(query["shaderinfo"], request->GetResponseContentStream()));
        return;
    }

    // no command, send the home page
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("Nebula Shaders");
    if (htmlWriter->Open())
    {
        ShaderServer* shdServer = ShaderServer::Instance();

        htmlWriter->Element(HtmlElement::Heading1, "Shader Resources");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();

        // create a table of all existing shaders
        htmlWriter->Element(HtmlElement::Heading3, "Shaders");
        const Dictionary<ResourceName, Resources::ResourceId>& shaders = shdServer->GetAllShaders();
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "ResId");
                htmlWriter->Element(HtmlElement::TableHeader, "NumInstances");
            htmlWriter->End(HtmlElement::TableRow);

            IndexT i;
            for (i = 0; i < shaders.Size(); i++)
            {
                const ResourceId shd = shaders.ValueAtIndex(i);
                const ResourceName name = shaders.KeyAtIndex(i);
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Begin(HtmlElement::TableData);
                        htmlWriter->AddAttr("href", "/shader?shaderinfo=" + name.AsString());
                        htmlWriter->Element(HtmlElement::Anchor, name.Value());
                    htmlWriter->End(HtmlElement::TableData);
                htmlWriter->End(HtmlElement::TableRow);
            }
        htmlWriter->End(HtmlElement::Table);

        // create a table of globally shared variables
        htmlWriter->Element(HtmlElement::Heading3, "Shared Shader Variables");

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
*/
HttpStatus::Code
ShaderPageHandler::HandleShaderInfoRequest(const Util::String& resId, const Ptr<IO::Stream>& responseContentStream)
{
    ShaderServer* shdServer = ShaderServer::Instance();
    Resources::ResourceId id = resId.AsLongLong();
    Resources::ResourceName name = ShaderGetName(id);

    // check if shader actually exists
    if (!shdServer->HasShader(resId))
    {
        return HttpStatus::NotFound;
    }

    if (id.resourceType != ShaderIdType)
    {
        // id is not a shader type!
        return HttpStatus::NotFound;
    }

    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(responseContentStream);
    htmlWriter->SetTitle("Nebula Shader Info");
    if (htmlWriter->Open())
    {
        // we need to create a temp shader instance to get reflection info
        const ShaderId& shd = id;
        SizeT numVars = ShaderGetConstantCount(shd);
        htmlWriter->Element(HtmlElement::Heading1, name.Value());
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("href", "/shader");
        htmlWriter->Element(HtmlElement::Anchor, "Shaders Home");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();

        // display some info about the shader
        htmlWriter->Element(HtmlElement::Heading3, "Resource Info");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Resource Id: ");
                htmlWriter->Element(HtmlElement::TableData, name.Value());
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Resolved Path: ");
                htmlWriter->Element(HtmlElement::TableData, AssignRegistry::Instance()->ResolveAssigns(name.Value()).AsString());
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Number of active states: ");
            htmlWriter->End(HtmlElement::TableRow);
        htmlWriter->End(HtmlElement::Table);

        // display shader variables
        htmlWriter->Element(HtmlElement::Heading3, "Shader Variables");
#if __NEBULA_HTTP__
        if (numVars > 0)
        {
            this->WriteShaderVariableTable(htmlWriter, shd);
        }
        else
#endif
        {
            htmlWriter->Text("No Shader Variables.");
            htmlWriter->LineBreak();
        }

        const Util::Dictionary<CoreGraphics::ShaderFeature::Mask, CoreGraphics::ShaderProgramId>& programs = ShaderGetPrograms(id);

        // display shader variations
        htmlWriter->Element(HtmlElement::Heading3, "Shader Variations");
        if (programs.Size() > 0)
        {
            htmlWriter->AddAttr("border", "1");
            htmlWriter->AddAttr("rules", "cols");
            htmlWriter->Begin(HtmlElement::Table);
                htmlWriter->AddAttr("bgcolor", "lightsteelblue");
                htmlWriter->Begin(HtmlElement::TableRow);    
                    htmlWriter->Element(HtmlElement::TableHeader, "Name");
                    htmlWriter->Element(HtmlElement::TableHeader, "FeatureBits");
                htmlWriter->End(HtmlElement::TableRow);
                IndexT i;
                for (i = 0; i < programs.Size(); i++)
                {
                    const CoreGraphics::ShaderProgramId& id = programs.ValueAtIndex(i);
                    const CoreGraphics::ShaderFeature::Mask& mask = programs.KeyAtIndex(i);
                    htmlWriter->Begin(HtmlElement::TableRow);
                        htmlWriter->Element(HtmlElement::TableData, ShaderProgramGetName(id).AsString());
                        htmlWriter->Element(HtmlElement::TableData, shdServer->FeatureMaskToString(mask));
                    htmlWriter->End(HtmlElement::TableRow);
                }
            htmlWriter->End(HtmlElement::Table);
        }
        else
        {
            htmlWriter->Text("No Shader Variations.");
            htmlWriter->LineBreak();
        }

        htmlWriter->Element(HtmlElement::Heading3, "Materials using this shader:");
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
        htmlWriter->AddAttr("bgcolor", "lightsteelblue");
        htmlWriter->Begin(HtmlElement::TableRow);
        htmlWriter->Element(HtmlElement::TableHeader, "Id");
        htmlWriter->Element(HtmlElement::TableHeader, "Name");
        htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "IMPLEMENT ME!");
            htmlWriter->End(HtmlElement::TableRow);
        htmlWriter->End(HtmlElement::Table);


        htmlWriter->Close();
        return HttpStatus::OK;
    }
    return HttpStatus::InternalServerError;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderPageHandler::WriteShaderVariableTable(const Ptr<HtmlPageWriter>& htmlWriter, const CoreGraphics::ShaderId shader)
{
    htmlWriter->AddAttr("border", "1");
    htmlWriter->AddAttr("rules", "cols");
    htmlWriter->Begin(HtmlElement::Table);
        htmlWriter->AddAttr("bgcolor", "lightsteelblue");
        htmlWriter->Begin(HtmlElement::TableRow);    
            htmlWriter->Element(HtmlElement::TableHeader, "Name");
            htmlWriter->Element(HtmlElement::TableHeader, "Type");
            htmlWriter->Element(HtmlElement::TableHeader, "IsArray");
            htmlWriter->Element(HtmlElement::TableHeader, "ArraySize");
        htmlWriter->End(HtmlElement::TableRow);

    SizeT numVars = ShaderGetConstantCount(shader);
    IndexT i;
    for (i = 0; i < numVars; i++)
    {
        const Util::StringAtom& name = ShaderGetConstantName(shader, i);
        const ShaderConstantType& type = ShaderGetConstantType(shader, i);
        htmlWriter->Begin(HtmlElement::TableRow);
            htmlWriter->Element(HtmlElement::TableData, name.AsString());
            htmlWriter->Element(HtmlElement::TableData, ConstantTypeToString(type));
        htmlWriter->End(HtmlElement::TableRow);
    }
    htmlWriter->End(HtmlElement::Table);
}

} // namespace Debug
