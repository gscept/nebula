//------------------------------------------------------------------------------
//  meshpagehandler.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/debug/meshpagehandler.h"
#include "coregraphics/mesh.h"
#include "http/html/htmlpagewriter.h"
#include "resources/resourceserver.h"
#include "coregraphics/streammeshpool.h"
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
		request->SetStatus(this->HandleMeshInfoRequest((query["meshinfo"]), request->GetResponseContentStream()));
		return;
	}
	else if (query.Contains("vertexdump"))
	{
		Util::String resId = query["vertexdump"];
		IndexT minIndex = query["min"].AsInt();
		IndexT maxIndex = query["max"].AsInt();
		request->SetStatus(this->HandleVertexDumpRequest(resId, minIndex, maxIndex, request->GetResponseContentStream()));
		return;
	}

	// no command, send the home page
	Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
	htmlWriter->SetStream(request->GetResponseContentStream());
	htmlWriter->SetTitle("Nebula Meshes");
	if (htmlWriter->Open())
	{
		htmlWriter->Element(HtmlElement::Heading1, "Shared Mesh Resources");
		htmlWriter->AddAttr("href", "/index.html");
		htmlWriter->Element(HtmlElement::Anchor, "Home");
		htmlWriter->LineBreak();
		htmlWriter->LineBreak();

		// get all stream-loaded mesh resources
		const StreamMeshPool* meshPool = ResourceServer::Instance()->GetStreamPool<StreamMeshPool>();
		const Util::Dictionary<Resources::ResourceName, Resources::ResourceId>& meshes = meshPool->GetResources();
	
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
			for (i = 0; i < meshes.Size(); i++)
			{
				const Resources::ResourceName& name = meshes.KeyAtIndex(i);
				const Resources::ResourceId& id = meshes.ValueAtIndex(i);
				const SizeT usage = meshPool->GetUsage(id);
				htmlWriter->Begin(HtmlElement::TableRow);
					htmlWriter->Begin(HtmlElement::TableData);
						htmlWriter->AddAttr("href", "/mesh?meshinfo=" + Util::String::FromLongLong(id.HashCode64()));
						htmlWriter->Element(HtmlElement::Anchor, name.Value());
					htmlWriter->End(HtmlElement::TableData);
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(usage));
					htmlWriter->Begin(HtmlElement::TableData);
						String str;
						str.Format("/mesh?vertexdump=%s&min=0&max=100", name.Value());
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
MeshPageHandler::HandleMeshInfoRequest(const Util::String& resId, const Ptr<Stream>& responseContentStream)
{
	// lookup the mesh in the ResourceServer
	const Ptr<ResourceServer>& resManager = ResourceServer::Instance();
	Resources::ResourceId id = resId.AsLongLong();

	if (!resManager->HasResource(id))
	{
		return HttpStatus::NotFound;
	}

	if (id.resourceType != MeshIdType)
	{
		// resource exists but is not a mesh
		return HttpStatus::NotFound;
	}

	Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
	htmlWriter->SetStream(responseContentStream);
	htmlWriter->SetTitle("Nebula Mesh Info");
	if (htmlWriter->Open())
	{
		const Resources::ResourceName& name = resManager->GetName(id);
		const SizeT usage = resManager->GetUsage(id);
		htmlWriter->Element(HtmlElement::Heading1, name.Value());
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
				htmlWriter->Element(HtmlElement::TableData, "Resource Name: ");
				htmlWriter->Element(HtmlElement::TableData, name.Value());
			htmlWriter->End(HtmlElement::TableRow);
			htmlWriter->Begin(HtmlElement::TableRow);
				htmlWriter->Element(HtmlElement::TableData, "Resolved Path: ");
				htmlWriter->Element(HtmlElement::TableData, AssignRegistry::Instance()->ResolveAssigns(name.Value()).AsString());
			htmlWriter->End(HtmlElement::TableRow);
			htmlWriter->Begin(HtmlElement::TableRow);
				htmlWriter->Element(HtmlElement::TableData, "Use Count: ");
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(usage));
			htmlWriter->End(HtmlElement::TableRow);
		htmlWriter->End(HtmlElement::Table);

		// write vertex buffer info
		htmlWriter->Element(HtmlElement::Heading3, "Vertices");
		BufferId vbo = MeshGetVertexBuffer(id, 0);
		if (vbo != BufferId::Invalid())
		{
            VertexLayoutId vlo = MeshGetPrimitiveGroups(id)[0].GetVertexLayout();
			htmlWriter->Begin(HtmlElement::Table);
				htmlWriter->Begin(HtmlElement::TableRow);
					htmlWriter->Element(HtmlElement::TableData, "Num Vertices: ");
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(BufferGetSize(vbo)));
				htmlWriter->End(HtmlElement::TableRow);
				htmlWriter->Begin(HtmlElement::TableRow);
					htmlWriter->Element(HtmlElement::TableData, "Vertex Stride: ");
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(VertexLayoutGetSize(vlo)) + " bytes");                
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
				const Util::Array<VertexComponent>& comps = VertexLayoutGetComponents(vlo);
				for (i = 0; i < comps.Size(); i++)
				{
					htmlWriter->Begin(HtmlElement::TableRow);
						const VertexComponent& comp = comps[i];
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
		BufferId ibo = MeshGetIndexBuffer(id);
		if (ibo != BufferId::Invalid())
		{
			htmlWriter->Begin(HtmlElement::Table);
				htmlWriter->Begin(HtmlElement::TableRow);
					htmlWriter->Element(HtmlElement::TableData, "Num Indices: ");
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(BufferGetSize(ibo)));
				htmlWriter->End(HtmlElement::TableRow);
				htmlWriter->Begin(HtmlElement::TableRow);
					htmlWriter->Element(HtmlElement::TableData, "Index Type: ");
					htmlWriter->Element(HtmlElement::TableData, IndexType::ToString(CoreGraphics::IndexType::Index32));
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

			const Util::Array<CoreGraphics::PrimitiveGroup>& primGroups = MeshGetPrimitiveGroups(id);
			IndexT i;
			for (i = 0; i < primGroups.Size(); i++)
			{
				const PrimitiveGroup& primGroup = primGroups[i];
				const PrimitiveTopology::Code& topo = MeshGetTopology(id);
				htmlWriter->Begin(HtmlElement::TableRow);
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetNumPrimitives(topo)));
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetBaseVertex()));
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetNumVertices()));
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetBaseIndex()));
					htmlWriter->Element(HtmlElement::TableData, String::FromInt(primGroup.GetNumIndices()));
					htmlWriter->Element(HtmlElement::TableData, PrimitiveTopology::ToString(topo));
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
MeshPageHandler::HandleVertexDumpRequest(const Util::String& resId, IndexT minVertexIndex, IndexT maxVertexIndex, const Ptr<Stream>& responseContentStream)
{
	// lookup the mesh in the ResourceServer
	const Ptr<ResourceServer>& resManager = ResourceServer::Instance();
	ResourceId id = resId.AsLongLong();
	if (!resManager->HasResource(id))
	{
		return HttpStatus::NotFound;
	}
	if (id.resourceType != MeshIdType)
	{
		// resource exists but is not a mesh
		return HttpStatus::NotFound;
	}
	
	const BufferId vb = MeshGetVertexBuffer(id, 0);
    //FIXME does not deal with different primitivegroup vertex layouts
	const VertexLayoutId vl = MeshGetPrimitiveGroups(id)[0].GetVertexLayout();

	// clip to valid range
	SizeT numverts = BufferGetSize(vb);
	if (minVertexIndex > numverts)
	{
		minVertexIndex = numverts;
	}
	if (maxVertexIndex > numverts)
	{
		maxVertexIndex = numverts;
	}
	
	Ptr<HtmlPageWriter> htmlWriter = HtmlPageWriter::Create();
	htmlWriter->SetStream(responseContentStream);
	htmlWriter->SetTitle("Nebula Mesh Dump");
	if (htmlWriter->Open())
	{
		// write header
		htmlWriter->Element(HtmlElement::Heading1, resId.AsCharPtr());
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
		for (rangeIndex = 0; rangeIndex < numverts; rangeIndex += rangeSize)
		{
			IndexT maxRangeIndex = rangeIndex + rangeSize;
			if (maxRangeIndex > numverts)
			{
				maxRangeIndex = numverts;
			}
		
			String link;
			link.Format("/mesh?vertexdump=%s&min=%d&max=%d", resId.AsCharPtr(), rangeIndex, maxRangeIndex);
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
				const Util::Array<VertexComponent>& comps = VertexLayoutGetComponents(vl);
				IndexT compIndex;
				for (compIndex = 0; compIndex < comps.Size(); compIndex++)
				{
					const VertexComponent& c = comps[compIndex];
					String n;
					n.Format("%s%d (%s)", 
						VertexComponent::SemanticNameToString(c.GetSemanticName()).AsCharPtr(),
						c.GetSemanticIndex(),
						VertexComponent::FormatToString(c.GetFormat()).AsCharPtr());
					htmlWriter->Element(HtmlElement::TableHeader, n);
				}
			htmlWriter->End(HtmlElement::TableRow);
			
			// for each vertex...
			SizeT vertexStride = VertexLayoutGetSize(vl);
			ubyte* ptr = (ubyte*)BufferMap(vb);
			IndexT vertexIndex;
			for (vertexIndex = minVertexIndex; vertexIndex < maxVertexIndex; vertexIndex++)
			{
				// start a new row and write vertex index
				htmlWriter->Begin(HtmlElement::TableRow);
				htmlWriter->Element(HtmlElement::TableData, String::FromInt(vertexIndex));
								
				// for each vertex component...
				String str;
				ubyte* vertexPtr = ptr + (vertexIndex * vertexStride);
				for (compIndex = 0; compIndex < comps.Size(); compIndex++)
				{
					const VertexComponent& c = comps[compIndex];
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
			BufferUnmap(vb);
			
		htmlWriter->End(HtmlElement::Table);
		htmlWriter->Close();
		return HttpStatus::OK;
	} // if Open
	return HttpStatus::InternalServerError;
}

} // namespace Debug
