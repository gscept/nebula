//------------------------------------------------------------------------------
//  shaderpagehandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/debug/shaderpagehandler.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/shader.h"
#include "http/html/htmlpagewriter.h"
#include "io/ioserver.h"
#include "graphics/graphicsserver.h"
#include "graphics/modelentity.h"
#include "models/modelnodeinstance.h"
#include "models/nodes/statenode.h"
#include "models/nodes/statenodeinstance.h"

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
        request->SetStatus(this->HandleShaderInfoRequest(ResourceId(query["shaderinfo"]), request->GetResponseContentStream()));
        return;
    }

    // no command, send the home page
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("NebulaT Shaders");
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
        const Dictionary<ResourceId, Ptr<Shader> >& shaders = shdServer->GetAllShaders();
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
                const Ptr<Shader>& shd = shaders.ValueAtIndex(i);
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Begin(HtmlElement::TableData);
                        htmlWriter->AddAttr("href", "/shader?shaderinfo=" + shd->GetResourceId().AsString());
                        htmlWriter->Element(HtmlElement::Anchor, shd->GetResourceId().Value());
                    htmlWriter->End(HtmlElement::TableData);
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(shd->GetAllShaderStates().Size()));
                htmlWriter->End(HtmlElement::TableRow);
            }
        htmlWriter->End(HtmlElement::Table);

        // create a table of globally shared variables
        htmlWriter->Element(HtmlElement::Heading3, "Shared Shader Variables");
        Array<Ptr<ShaderVariable> > sharedVars;
        for (i = 0; i < shdServer->GetNumSharedVariables(); i++)
        {
            sharedVars.Append(shdServer->GetSharedVariableByIndex(i));
        }
        this->WriteShaderVariableTable(htmlWriter, sharedVars);

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
ShaderPageHandler::HandleShaderInfoRequest(const ResourceId& resId, const Ptr<Stream>& responseContentStream)
{
    ShaderServer* shdServer = ShaderServer::Instance();

    // check if shader actually exists
    if (!shdServer->HasShader(resId))
    {
        return HttpStatus::NotFound;
    }

    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(responseContentStream);
    htmlWriter->SetTitle("NebulaT Shader Info");
    if (htmlWriter->Open())
    {
        // we need to create a temp shader instance to get reflection info
        const Ptr<Shader>& shd = shdServer->GetShader(resId);

        htmlWriter->Element(HtmlElement::Heading1, resId.Value());
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
                htmlWriter->Element(HtmlElement::TableData, resId.Value());
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Resolved Path: ");
                htmlWriter->Element(HtmlElement::TableData, AssignRegistry::Instance()->ResolveAssigns(resId.Value()).AsString());
            htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, "Instance Count: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(shd->GetAllShaderStates().Size()));
            htmlWriter->End(HtmlElement::TableRow);
        htmlWriter->End(HtmlElement::Table);

        // display shader variables
        htmlWriter->Element(HtmlElement::Heading3, "Shader Variables");
#if __NEBULA3_HTTP__
		const Ptr<CoreGraphics::ShaderState>& state = shd->GetDebugState();
		if (state->GetNumVariables() > 0)
        {
            IndexT i;
            Array<Ptr<ShaderVariable> > variables;
			for (i = 0; i < state->GetNumVariables(); i++)
            {
				variables.Append(state->GetVariableByIndex(i));
            }
            this->WriteShaderVariableTable(htmlWriter, variables);
        }
        else
#endif
        {
            htmlWriter->Text("No Shader Variables.");
            htmlWriter->LineBreak();
        }

        // display shader variations
        htmlWriter->Element(HtmlElement::Heading3, "Shader Variations");
        if (shd->GetNumVariations() > 0)
        {
            htmlWriter->AddAttr("border", "1");
            htmlWriter->AddAttr("rules", "cols");
            htmlWriter->Begin(HtmlElement::Table);
                htmlWriter->AddAttr("bgcolor", "lightsteelblue");
                htmlWriter->Begin(HtmlElement::TableRow);    
                    htmlWriter->Element(HtmlElement::TableHeader, "Name");
                    htmlWriter->Element(HtmlElement::TableHeader, "FeatureBits");
                    htmlWriter->Element(HtmlElement::TableHeader, "NumPasses");
                htmlWriter->End(HtmlElement::TableRow);
                IndexT i;
                for (i = 0; i < shd->GetNumVariations(); i++)
                {
                    const Ptr<ShaderVariation>& variation = shd->GetVariationByIndex(i);
                    htmlWriter->Begin(HtmlElement::TableRow);
                        htmlWriter->Element(HtmlElement::TableData, variation->GetName().Value());
                        htmlWriter->Element(HtmlElement::TableData, shdServer->FeatureMaskToString(variation->GetFeatureMask()));
                        htmlWriter->Element(HtmlElement::TableData, String::FromInt(variation->GetNumPasses()));
                    htmlWriter->End(HtmlElement::TableRow);
                }
            htmlWriter->End(HtmlElement::Table);
        }
        else
        {
            htmlWriter->Text("No Shader Variations.");
            htmlWriter->LineBreak();
        }

        htmlWriter->Element(HtmlElement::Heading3, "Entities uses this shader:"); 
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
        htmlWriter->AddAttr("bgcolor", "lightsteelblue");
        htmlWriter->Begin(HtmlElement::TableRow);    
        htmlWriter->Element(HtmlElement::TableHeader, "Id");
        htmlWriter->Element(HtmlElement::TableHeader, "Name"); 
        htmlWriter->End(HtmlElement::TableRow); 

        GraphicsServer* server = GraphicsServer::Instance();
        const Util::Array<Ptr<GraphicsEntity> >& internalEntities = server->GetEntities();
        IndexT i;
        for (i = 0; i < internalEntities.Size(); ++i)
        {
            if (internalEntities[i].isvalid() && internalEntities[i]->IsA(Graphics::ModelEntity::RTTI))
            {  
                const Ptr<Models::ModelInstance>& modelInst = internalEntities[i].cast<Graphics::ModelEntity>()->GetModelInstance();
                if (modelInst.isvalid())
                {
                    const Ptr<Models::ModelNodeInstance>& root = modelInst->GetRootNodeInstance();
                    if (this->HasShaderInstance(root, resId))
                    {
                        htmlWriter->Begin(HtmlElement::TableRow);
                            htmlWriter->Begin(HtmlElement::TableData); 
                            htmlWriter->AddAttr("href", "/graphics?entity=" + String::FromInt(internalEntities[i]->GetId()));
                            htmlWriter->Element(HtmlElement::Anchor, String::FromInt(internalEntities[i]->GetId()));
                            htmlWriter->End(HtmlElement::TableData); 

                            htmlWriter->Begin(HtmlElement::TableData); 
                            htmlWriter->AddAttr("href", "/graphics?entity=" + String::FromInt(internalEntities[i]->GetId()));
                            htmlWriter->Element(HtmlElement::Anchor, internalEntities[i].cast<Graphics::ModelEntity>()->GetResourceId().Value());
                            htmlWriter->End(HtmlElement::TableData); 
                        htmlWriter->End(HtmlElement::TableRow);                    
                    }
                }   
            }
        }       
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
ShaderPageHandler::WriteShaderVariableTable(const Ptr<HtmlPageWriter>& htmlWriter, const Array<Ptr<ShaderVariable> >& vars)
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

    IndexT i;
    for (i = 0; i < vars.Size(); i++)
    {
        const Ptr<ShaderVariable>& var = vars[i];
        htmlWriter->Begin(HtmlElement::TableRow);
            htmlWriter->Element(HtmlElement::TableData, var->GetName().Value());
            htmlWriter->Element(HtmlElement::TableData, ShaderVariable::TypeToString(var->GetType()));
        htmlWriter->End(HtmlElement::TableRow);
    }
    htmlWriter->End(HtmlElement::Table);
}

//------------------------------------------------------------------------------
/**
*/
bool 
ShaderPageHandler::HasShaderInstance(const Ptr<Models::ModelNodeInstance>& node, const ResourceId& resId)
{
    bool shaderFound = false;
    if (node->IsA(StateNodeInstance::RTTI))
    {
         const Ptr<StateNode>& stateNode = node.cast<StateNodeInstance>()->GetModelNode().cast<StateNode>();
		 const Ptr<Materials::Surface>& instance = stateNode->GetMaterial();
         const Ptr<Materials::Material>& mat = instance->GetMaterialTemplate();

		 // go through shader instances and tag the shader as found if it's in the material
		 IndexT i;
		 for (i = 0; i < mat->GetNumPasses(); i++)
		 {
            shaderFound = mat->GetPassByIndex(i).shader->GetResourceId() == resId;
		    break;
		 }
    }
    const Util::Array<Ptr<ModelNodeInstance> > children = node->GetChildren();
    IndexT index;
    for (index = 0; index < children.Size(); ++index)
    {
        shaderFound |= this->HasShaderInstance(children[index], resId);
    }
    return shaderFound;
}
} // namespace Debug
