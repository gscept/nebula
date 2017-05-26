//------------------------------------------------------------------------------
//  meshpagehandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/debug/meshpagehandler.h"
#include "coregraphics/mesh.h"
#include "http/html/htmlpagewriter.h"
#include "resources/resourcemanager.h"
#include "io/ioserver.h"

namespace Debug
{
__ImplementClass(Debug::MeshPageHandler, 'DMSH', Http::HttpRequestHandler);

using namespace IO;
using namespace CoreGraphics;
using namespace Util;
using namespace Http;
using namespace Resources;

//------------------------------------------------------------------------------
/**
*/
MeshPageHandler::MeshPageHandler()
{
    this->SetName("Meshes");
    this->SetDesc("show debug information about shared mesh resources");
    this->SetRootLocation("mesh");
}

//------------------------------------------------------------------------------
/**
*/
void
MeshPageHandler::HandleRequest(const Ptr<HttpRequest>& request)
{
    n_assert(HttpMethod::Get == request->GetMethod());

    // first check if a command has been defined in the URI
    Dictionary<String,String> query = request->GetURI().ParseQuery();
    if (query.Contains("meshinfo"))
    {
        request->SetStatus(this->HandleMeshInfoRequest(ResourceId(query["meshinfo"]), request->GetResponseContentStream()));
        return;
    }
    else if (query.Contains("vertexdump"))
    {
        ResourceId resId = query["vertexdump"];
        IndexT minIndex = query["min"].AsInt();
        IndexT maxIndex = query["max"].AsInt();
        request->SetStatus(this->HandleVertexDumpRequest(resId, minIndex, maxIndex, request->GetResponseContentStream()));
        return;
    }

    // no command, send the home page
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(request->GetResponseContentStream());
    htmlWriter->SetTitle("NebulaT Meshes");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, "Shared Mesh Resources");
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();

        // get all mesh resources
        Array<Ptr<Resource> > meshResources = ResourceManager::Instance()->GetResourcesByType(Mesh::RTTI);
    
        // create a table of all existing meshes
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "ResId");
                htmlWriter->Element(HtmlElement::TableHeader, "UseCount");
                htmlWriter->Element(HtmlElement::TableHeader, "Dump");
            htmlWriter->End(HtmlElement::TableRow);
            
            // iterate over shared resources
            IndexT i;
            for (i = 0; i < meshResources.Size(); i++)
            {
                const Ptr<Mesh>& mesh = meshResources[i].downcast<Mesh>();
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Begin(HtmlElement::TableData);
                        htmlWriter->AddAttr("href", "/mesh?meshinfo=" + mesh->GetResourceId().AsString());
                        htmlWriter->Element(HtmlElement::Anchor, mesh->GetResourceId().Value());
                    htmlWriter->End(HtmlElement::TableData);
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(mesh->GetUseCount()));
                    htmlWriter->Begin(HtmlElement::TableData);
                        String str;
                        str.Format("/mesh?vertexdump=%s&min=0&max=100", mesh->GetResourceId().Value());
                        htmlWriter->AddAttr("href", str);
                        htmlWriter->Element(HtmlElement::Anchor, "dump");
                    htmlWriter->End(HtmlElement::TableData);
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
*/
HttpStatus::Code
MeshPageHandler::HandleMeshInfoRequest(const ResourceId& resId, const Ptr<Stream>& responseContentStream)
{
    // lookup the mesh in the ResourceManager
    const Ptr<ResourceManager>& resManager = ResourceManager::Instance();
    if (!resManager->HasResource(resId))
    {
        return HttpStatus::NotFound;
    }
    const Ptr<Resource>& res = resManager->LookupResource(resId);
    if (!res->IsA(Mesh::RTTI))
    {
        // resource exists but is not a mesh
        return HttpStatus::NotFound;
    }
    const Ptr<Mesh>& mesh = res.downcast<Mesh>();

    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(responseContentStream);
    htmlWriter->SetTitle("NebulaT Mesh Info");
    if (htmlWriter->Open())
    {
        htmlWriter->Element(HtmlElement::Heading1, resId.Value());
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("href", "/mesh");
        htmlWriter->Element(HtmlElement::Anchor, "Meshes Home");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();

        // display some info about the mesh
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
                htmlWriter->Element(HtmlElement::TableData, "Use Count: ");
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(mesh->GetUseCount()));
            htmlWriter->End(HtmlElement::TableRow);
        htmlWriter->End(HtmlElement::Table);

        // write vertex buffer info
        htmlWriter->Element(HtmlElement::Heading3, "Vertices");
        if (mesh->HasVertexBuffer())
        {
            const Ptr<VertexBuffer>& vb = mesh->GetVertexBuffer();
            const Ptr<VertexLayout>& vertexLayout = vb->GetVertexLayout();
            htmlWriter->Begin(HtmlElement::Table);
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, "Num Vertices: ");
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(vb->GetNumVertices()));
                htmlWriter->End(HtmlElement::TableRow);
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, "Vertex Stride: ");
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(vertexLayout->GetVertexByteSize()) + " bytes");                
                htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->End(HtmlElement::Table);
            htmlWriter->Element(HtmlElement::Heading3, "Vertex Components");
            htmlWriter->AddAttr("border", "1");
            htmlWriter->AddAttr("rules", "cols");
            htmlWriter->Begin(HtmlElement::Table);
                htmlWriter->AddAttr("bgcolor", "lightsteelblue");
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableHeader, "Semantic");
                    htmlWriter->Element(HtmlElement::TableHeader, "SemIdx");
                    htmlWriter->Element(HtmlElement::TableHeader, "Format");
                    htmlWriter->Element(HtmlElement::TableHeader, "Offset");
                    htmlWriter->Element(HtmlElement::TableHeader, "Size");
                htmlWriter->End(HtmlElement::TableRow);
                IndexT i;
                for (i = 0; i < vertexLayout->GetNumComponents(); i++)
                {
                    htmlWriter->Begin(HtmlElement::TableRow);
                        const VertexComponent& comp = vertexLayout->GetComponentAt(i);
                        htmlWriter->Element(HtmlElement::TableData, VertexComponent::SemanticNameToString(comp.GetSemanticName()));
                        htmlWriter->Element(HtmlElement::TableData, String::FromInt(comp.GetSemanticIndex()));
                        htmlWriter->Element(HtmlElement::TableData, VertexComponent::FormatToString(comp.GetFormat()));
                        htmlWriter->Element(HtmlElement::TableData, String::FromInt(comp.GetByteOffset()));
                        htmlWriter->Element(HtmlElement::TableData, String::FromInt(comp.GetByteSize()));
                    htmlWriter->End(HtmlElement::TableRow);
                }
            htmlWriter->End(HtmlElement::Table);
        }
        else
        {
            htmlWriter->Text("NO VERTICES");
            htmlWriter->LineBreak();
        }

        // write index buffer info
        htmlWriter->Element(HtmlElement::Heading3, "Indices");
        if (mesh->HasIndexBuffer())
        {
            const Ptr<IndexBuffer>& ib = mesh->GetIndexBuffer();
            htmlWriter->Begin(HtmlElement::Table);
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, "Num Indices: ");
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(ib->GetNumIndices()));
                htmlWriter->End(HtmlElement::TableRow);
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, "Index Type: ");
                    htmlWriter->Element(HtmlElement::TableData, IndexType::ToString(ib->GetIndexType()));
                htmlWriter->End(HtmlElement::TableRow);
            htmlWriter->End(HtmlElement::Table);
        }
        else
        {
            htmlWriter->Text("NO INDICES");
            htmlWriter->LineBreak();
        }

        // write primitive group info
        htmlWriter->Element(HtmlElement::Heading3, "Primitive Groups");
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "Num Primitives");
                htmlWriter->Element(HtmlElement::TableHeader, "Base Vertex");
                htmlWriter->Element(HtmlElement::TableHeader, "Num Vertices");
                htmlWriter->Element(HtmlElement::TableHeader, "Base Index");
                htmlWriter->Element(HtmlElement::TableHeader, "Num Indices");
                htmlWriter->Element(HtmlElement::TableHeader, "Topology");
            htmlWriter->End(HtmlElement::TableRow);
            IndexT i;
            for (i = 0; i < mesh->GetNumPrimitiveGroups(); i++)
            {
                const PrimitiveGroup& primGroup = mesh->GetPrimitiveGroupAtIndex(i);
                htmlWriter->Begin(HtmlElement::TableRow);
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetNumPrimitives(mesh->GetTopology())));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetBaseVertex()));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetNumVertices()));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetBaseIndex()));
                    htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetNumIndices()));
                    htmlWriter->Element(HtmlElement::TableData, PrimitiveTopology::ToString(mesh->GetTopology()));
                htmlWriter->End(HtmlElement::TableRow);
            }
        htmlWriter->End(HtmlElement::Table);
        htmlWriter->Close();
        return HttpStatus::OK;
    }
    return HttpStatus::InternalServerError;
}

//------------------------------------------------------------------------------
/**
    Handle a mesh dump request.
*/
HttpStatus::Code
MeshPageHandler::HandleVertexDumpRequest(const ResourceId& resId, IndexT minVertexIndex, IndexT maxVertexIndex, const Ptr<Stream>& responseContentStream)
{
    // lookup the mesh in the ResourceManager
    const Ptr<ResourceManager>& resManager = ResourceManager::Instance();
    if (!resManager->HasResource(resId))
    {
        return HttpStatus::NotFound;
    }
    const Ptr<Resource>& res = resManager->LookupResource(resId);
    if (!res->IsA(Mesh::RTTI))
    {
        // resource exists but is not a mesh
        return HttpStatus::NotFound;
    }
    
    const Ptr<Mesh>& mesh = res.downcast<Mesh>();
    const Ptr<VertexBuffer>& vb = mesh->GetVertexBuffer();
    const Ptr<VertexLayout>& vl = vb->GetVertexLayout();

    // clip to valid range
    if (minVertexIndex > vb->GetNumVertices())
    {
        minVertexIndex = vb->GetNumVertices();
    }
    if (maxVertexIndex > vb->GetNumVertices())
    {
        maxVertexIndex = vb->GetNumVertices();
    }
    
    Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
    htmlWriter->SetStream(responseContentStream);
    htmlWriter->SetTitle("NebulaT Mesh Dump");
    if (htmlWriter->Open())
    {
        // write header
        htmlWriter->Element(HtmlElement::Heading1, resId.Value());
        htmlWriter->AddAttr("href", "/index.html");
        htmlWriter->Element(HtmlElement::Anchor, "Home");
        htmlWriter->LineBreak();
        htmlWriter->AddAttr("href", "/mesh");
        htmlWriter->Element(HtmlElement::Anchor, "Meshes Home");
        htmlWriter->LineBreak();
        htmlWriter->LineBreak();
        
        // first build a simple range table, since we cannot display all vertices at once!   
        htmlWriter->Element(HtmlElement::Heading3, "Select Range");
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
        IndexT rangeIndex;
        const SizeT rangeSize = 100;        
        for (rangeIndex = 0; rangeIndex < vb->GetNumVertices(); rangeIndex += rangeSize)
        {
            IndexT maxRangeIndex = rangeIndex + rangeSize;
            if (maxRangeIndex > vb->GetNumVertices())
            {
                maxRangeIndex = vb->GetNumVertices();
            }
        
            String link;
            link.Format("/mesh?vertexdump=%s&min=%d&max=%d", mesh->GetResourceId().Value(), rangeIndex, maxRangeIndex);
            String str;
            str.Format("%d .. %d", rangeIndex, maxRangeIndex);
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Begin(HtmlElement::TableData);
                    htmlWriter->AddAttr("href", link);
                    htmlWriter->Element(HtmlElement::Anchor, str);                        
                htmlWriter->End(HtmlElement::TableData);
            htmlWriter->End(HtmlElement::TableRow);
        }
        htmlWriter->End(HtmlElement::Table);
    
        // write actual vertex dump table        
        htmlWriter->Element(HtmlElement::Heading3, "Vertex Dump");
        htmlWriter->AddAttr("border", "1");
        htmlWriter->AddAttr("rules", "cols");
        htmlWriter->Begin(HtmlElement::Table);
            htmlWriter->AddAttr("bgcolor", "lightsteelblue");
            htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableHeader, "Index");            
                SizeT numComps = vl->GetNumComponents();
                IndexT compIndex;
                for (compIndex = 0; compIndex < numComps; compIndex++)
                {
                    const VertexComponent& c = vl->GetComponentAt(compIndex);
                    String n;
                    n.Format("%s%d (%s)", 
                        VertexComponent::SemanticNameToString(c.GetSemanticName()).AsCharPtr(),
                        c.GetSemanticIndex(),
                        VertexComponent::FormatToString(c.GetFormat()).AsCharPtr());
                    htmlWriter->Element(HtmlElement::TableHeader, n);
                }
            htmlWriter->End(HtmlElement::TableRow);
            
            // for each vertex...
            SizeT vertexStride = vl->GetVertexByteSize();
            ubyte* ptr = (ubyte*) vb->Map(VertexBuffer::MapRead);
            IndexT vertexIndex;
            for (vertexIndex = minVertexIndex; vertexIndex < maxVertexIndex; vertexIndex++)
            {
                // start a new row and write vertex index
                htmlWriter->Begin(HtmlElement::TableRow);
                htmlWriter->Element(HtmlElement::TableData, String::FromInt(vertexIndex));
                                
                // for each vertex component...
                String str;
                ubyte* vertexPtr = ptr + (vertexIndex * vertexStride);
                for (compIndex = 0; compIndex < numComps; compIndex++)
                {
                    const VertexComponent& c = vl->GetComponentAt(compIndex);
                    ubyte* cPtr = vertexPtr + c.GetByteOffset();
                    float* fltPtr = (float*) cPtr;
                    ubyte* ubPtr  = (ubyte*) cPtr;
                    ushort* usPtr  = (ushort*) cPtr;
                    switch (c.GetFormat())
                    {
                        case VertexComponent::Float:
                            str.Format("%.2f", fltPtr[0]);
                            break;
                        
                        case VertexComponent::Float2:
                            str.Format("%.2f/%.2f", fltPtr[0], fltPtr[1]);
                            break;
                        
                        case VertexComponent::Float3:
                            str.Format("%.2f/%.2f/%.2f", fltPtr[0], fltPtr[1], fltPtr[2]);
                            break;
                        
                        case VertexComponent::Float4:
                            str.Format("%.2f/%.2f/%.2f/%.2f", fltPtr[0], fltPtr[1], fltPtr[2], fltPtr[3]);
                            break;
                        
                        case VertexComponent::UByte4:
                        case VertexComponent::UByte4N:
                            str.Format("0x%02X/0x%02X/0x%02X/0x%02X", ubPtr[0], ubPtr[1], ubPtr[2], ubPtr[3]);
                            break;
                        
                        case VertexComponent::Short2:
                        case VertexComponent::Short2N:
                            str.Format("0x%04X/0x%04X", usPtr[0], usPtr[1]);
                            break;
                        
                        case VertexComponent::Short4:
                        case VertexComponent::Short4N:
                            str.Format("0x%04X/0x%04X/0x%04X/0x%04X", usPtr[0], usPtr[1], usPtr[2], usPtr[3]);
                            break;

                        default:
                            str = "<invalid>";
                            break;
                                
                    } // switch format
                    htmlWriter->Element(HtmlElement::TableData, str);
                } // for compIndex
                htmlWriter->End(HtmlElement::TableRow);                    
            } // for vertexIndex
            vb->Unmap();
            
        htmlWriter->End(HtmlElement::Table);
        htmlWriter->Close();
        return HttpStatus::OK;
    } // if Open
    return HttpStatus::InternalServerError;
}

} // namespace Debug
