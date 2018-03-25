//------------------------------------------------------------------------------
//  meshbuildersaver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/meshutil/meshbuildersaver.h"
#include "coregraphics/legacy/nvx2fileformatstructs.h"
#include "coregraphics/nvx3fileformatstructs.h"
#include "io/ioserver.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;
using namespace CoreGraphics;
using namespace System;
using namespace Math;

#define SWAPINT16ENDIAN(x) ( (((x) & 0xFF) << 8) | ((unsigned short)(x) >> 8) )
#define SWAPINT32ENDIAN(x) ( ((x) << 24) | (( (x) << 8) & 0x00FF0000) | (( (x) >> 8) & 0x0000FF00) | ((x) >> 24) )

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderSaver::SaveNvx2(const URI& uri, MeshBuilder& meshBuilder, Platform::Code platform)
{
	// extend vertex components to maintain uniform buffer structure
	meshBuilder.ExtendVertexComponents();

	// sort triangles by group id and create a group map
	meshBuilder.SortTriangles();
	Array<MeshBuilderGroup> groupMap;
	meshBuilder.BuildGroupMap(groupMap);

    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
		ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));

        MeshBuilderSaver::WriteHeaderNvx2(stream, meshBuilder, groupMap.Size(), byteOrder);
		MeshBuilderSaver::WriteGroupsNvx2(stream, meshBuilder, groupMap, byteOrder);
		MeshBuilderSaver::WriteVertices(stream, meshBuilder, byteOrder);
		MeshBuilderSaver::WriteTriangles(stream, meshBuilder, byteOrder);

        stream->Close();
        stream = 0;
        return true;
    }
    else
    {
        // failed to open write stream
        return false;
    }

	return true;
}


//------------------------------------------------------------------------------
/**
*/
bool 
MeshBuilderSaver::SaveNvx3( const IO::URI& uri, MeshBuilder& meshBuilder, Platform::Code platform )
{
	meshBuilder.ExtendVertexComponents();

	// sort triangles by group id and create a group map
	meshBuilder.SortTriangles();
	Array<MeshBuilderGroup> groupMap;
	meshBuilder.BuildGroupMap(groupMap);

	// make sure the target directory exists
	IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

	Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
	stream->SetAccessMode(Stream::WriteAccess);
	if (stream->Open())
	{
		ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));

		MeshBuilderSaver::WriteHeaderNvx3(stream, meshBuilder, groupMap.Size(), byteOrder);
		MeshBuilderSaver::WriteGroupsNvx3(stream, meshBuilder, groupMap, byteOrder);
		MeshBuilderSaver::WriteVertices(stream, meshBuilder, byteOrder);
		MeshBuilderSaver::WriteTriangles(stream, meshBuilder, byteOrder);

		stream->Close();
		stream = 0;
		return true;
	}
	else
	{
		// failed to open write stream
		return false;
	}

	return true;
}

//------------------------------------------------------------------------------
/**
*/
void 
MeshBuilderSaver::WriteHeaderNvx3( const Ptr<IO::Stream>& stream, MeshBuilder& meshBuilder, SizeT numGroups, const System::ByteOrder& byteOrder )
{
	const int vertexWidth = meshBuilder.VertexAt(0).GetWidth();
	const int numVertices = meshBuilder.GetNumVertices();
	const int numTriangles = meshBuilder.GetNumTriangles();

	// write header
	Nvx3Header nvx3Header;
	nvx3Header.magic = byteOrder.Convert<uint>('NVX3');
	nvx3Header.numGroups = byteOrder.Convert<uint>(numGroups);
	nvx3Header.numVertices = byteOrder.Convert<uint>(numVertices);
	nvx3Header.vertexWidth = byteOrder.Convert<uint>(vertexWidth);
	nvx3Header.numIndices = byteOrder.Convert<uint>(numTriangles);
	nvx3Header.vertexComponentMask = byteOrder.Convert<uint>(meshBuilder.VertexAt(0).GetComponentMask());

	// write header
	stream->Write(&nvx3Header, sizeof(nvx3Header));
}

//------------------------------------------------------------------------------
/**
*/
void 
MeshBuilderSaver::WriteGroupsNvx3( const Ptr<IO::Stream>& stream, MeshBuilder& meshBuilder, Util::Array<MeshBuilderGroup>& groupMap, const System::ByteOrder& byteOrder )
{
	int curGroupIndex;
	for (curGroupIndex = 0; curGroupIndex < groupMap.Size(); curGroupIndex++)
	{
		const MeshBuilderGroup& curGroup = groupMap[curGroupIndex];
		int firstTriangle = curGroup.GetFirstTriangleIndex();
		int numTriangles  = curGroup.GetNumTriangles();
		int minVertexIndex, maxVertexIndex;
		meshBuilder.FindGroupVertexRange(curGroup.GetGroupId(), minVertexIndex, maxVertexIndex);
		
		Nvx3Group nvx3Group;
		nvx3Group.firstVertex = byteOrder.Convert<uint>(minVertexIndex);
		nvx3Group.numVertices = byteOrder.Convert<uint>((maxVertexIndex - minVertexIndex) + 1);
		nvx3Group.firstTriangle = byteOrder.Convert<uint>(firstTriangle);
		nvx3Group.numTriangles = byteOrder.Convert<uint>(numTriangles);
		nvx3Group.primType = PrimitiveTopology::TriangleList;

		// write group to stream
		stream->Write(&nvx3Group, sizeof(nvx3Group));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteHeaderNvx2(const Ptr<Stream>& stream, MeshBuilder& meshBuilder, SizeT numGroups, const ByteOrder& byteOrder)
{
	const int vertexWidth = meshBuilder.VertexAt(0).GetWidth();
	const int numVertices = meshBuilder.GetNumVertices();
	const int numTriangles = meshBuilder.GetNumTriangles();
	const int numEdges = 0; // Test-haXX

	// write header

	Nvx2Header nvx2Header;
	nvx2Header.magic = byteOrder.Convert<uint>('NVX2');
	nvx2Header.numGroups = byteOrder.Convert<uint>(numGroups);
	nvx2Header.numVertices = byteOrder.Convert<uint>(numVertices);
	nvx2Header.vertexWidth = byteOrder.Convert<uint>(vertexWidth);
	nvx2Header.numIndices = byteOrder.Convert<uint>(numTriangles);
	nvx2Header.numEdges = byteOrder.Convert<uint>(numEdges);
	nvx2Header.vertexComponentMask = byteOrder.Convert<uint>(meshBuilder.VertexAt(0).GetComponentMask());

    // write header
    stream->Write(&nvx2Header, sizeof(nvx2Header));
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteGroupsNvx2(const Ptr<Stream>& stream, MeshBuilder& meshBuilder, Util::Array<MeshBuilderGroup>& groupMap, const ByteOrder& byteOrder)
{
	int curGroupIndex;
	for (curGroupIndex = 0; curGroupIndex < groupMap.Size(); curGroupIndex++)
	{
		const MeshBuilderGroup& curGroup = groupMap[curGroupIndex];
		int firstTriangle = curGroup.GetFirstTriangleIndex();
		int numTriangles  = curGroup.GetNumTriangles();
		int minVertexIndex, maxVertexIndex;
		meshBuilder.FindGroupVertexRange(curGroup.GetGroupId(), minVertexIndex, maxVertexIndex);
		int minEdgeIndex, maxEdgeIndex;
		minEdgeIndex = maxEdgeIndex = 0;
		
		Nvx2Group nvx2Group;
		nvx2Group.firstVertex = byteOrder.Convert<uint>(minVertexIndex);
		nvx2Group.numVertices = byteOrder.Convert<uint>((maxVertexIndex - minVertexIndex) + 1);
		nvx2Group.firstTriangle = byteOrder.Convert<uint>(firstTriangle);
		nvx2Group.numTriangles = byteOrder.Convert<uint>(numTriangles);
		nvx2Group.firstEdge = byteOrder.Convert<uint>(minEdgeIndex);
		nvx2Group.numEdges = byteOrder.Convert<uint>((maxEdgeIndex - minEdgeIndex) + 1);

		// write group to stream
		stream->Write(&nvx2Group, sizeof(nvx2Group));
	}
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteVertices(const Ptr<Stream>& stream, MeshBuilder& meshBuilder, const ByteOrder& byteOrder)
{
	int numVertices = meshBuilder.GetNumVertices();
	int vertexWidth = meshBuilder.VertexAt(0).GetWidth();
	float* floatBuffer = n_new_array(float, numVertices * vertexWidth);
	float* floatPtr = floatBuffer;
	int curVertexIndex;
	
	for (curVertexIndex = 0; curVertexIndex < numVertices; curVertexIndex++)
	{
		const MeshBuilderVertex& curVertex = meshBuilder.VertexAt(curVertexIndex);
		if (curVertex.HasComponent(MeshBuilderVertex::CoordBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::CoordIndex);
			*floatPtr++ = v.x();
			*floatPtr++ = v.y();
			*floatPtr++ = v.z();
		}
		if (curVertex.HasComponent(MeshBuilderVertex::NormalBit))
		{
			const float4& v =  curVertex.GetComponent(MeshBuilderVertex::NormalIndex);
			*floatPtr++ = v.x();
			*floatPtr++ = v.y();
			*floatPtr++ = v.z();
		}
		//check for UB4NBits
		if (curVertex.HasComponent(MeshBuilderVertex::NormalB4NBit))
		{
			const float4& v =  curVertex.GetComponent(MeshBuilderVertex::NormalB4NIndex);
			float x = v.x() * 0.5f * 255.0f;// / 2 + 0.5f;
			float y = v.y() * 0.5f * 255.0f;// / 2 + 0.5f;
			float z = v.z() * 0.5f * 255.0f;// / 2 + 0.5f;
			float w = v.w() * 0.5f * 255.0f; // we don't need to pack w...
			int xBits = (int)x;
			int yBits = (int)y;
			int zBits = (int)z;
			int wBits = (int)w;
			int xyzwBits = ((wBits << 24) & 0xFF000000) | ((zBits << 16) & 0x00FF0000) | ((yBits << 8) & 0x0000FF00) | (xBits & 0x000000FF);
			*(int*)floatPtr = xyzwBits;	
			floatPtr++;
		}
		int curUvSet;
		for (curUvSet = 0; curUvSet < 8; curUvSet++)
		{
			if (curVertex.HasComponent((MeshBuilderVertex::ComponentBit) (MeshBuilderVertex::Uv0Bit << curUvSet)))
			{
				const float4& v = curVertex.GetComponent((MeshBuilderVertex::ComponentIndex)(MeshBuilderVertex::Uv0Index+curUvSet));
				if (curUvSet%2)
				{
					float x = v.x();
					float y = v.y();

					int xBits = (int)(v.x() * (1 << 13));
					int yBits = (int)(v.y() * (1 << 13));

					int xyBits = ((yBits << 16) | xBits);
					*(int*)floatPtr = xyBits;
					floatPtr++;
				}
				else
				{
					*floatPtr++ = v.x();
					*floatPtr++ = v.y();
				}
			}
		}
		if (curVertex.HasComponent(MeshBuilderVertex::ColorBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::ColorIndex);
			*floatPtr++ = v.x();
			*floatPtr++ = v.y();
			*floatPtr++ = v.z();
			*floatPtr++ = v.w();
		}
		//check for UB4NBits
		if (curVertex.HasComponent(MeshBuilderVertex::ColorUB4NBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::ColorUB4NIndex);
			unsigned int xBits = (unsigned int)(v.x() * (255.0f));
			unsigned int yBits = (unsigned int)(v.y() * (255.0f));
			unsigned int zBits = (unsigned int)(v.z() * (255.0f));
			unsigned int wBits = (unsigned int)(v.w() * (255.0f));
			int xyzwBits = ((wBits << 24) & 0xFF000000) | ((zBits << 16) & 0x00FF0000) | ((yBits << 8) & 0x0000FF00) | (xBits & 0x000000FF);
			*(int*)floatPtr++ = xyzwBits;	
		}
		if (curVertex.HasComponent(MeshBuilderVertex::TangentBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::TangentIndex);
			*floatPtr++ = v.x();
			*floatPtr++ = v.y();
			*floatPtr++ = v.z();
		}
		//check for UB4NBits
		if (curVertex.HasComponent(MeshBuilderVertex::TangentB4NBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::TangentB4NIndex);
			float x = v.x() * 0.5f * 255.0f;
			float y = v.y() * 0.5f * 255.0f;
			float z = v.z() * 0.5f * 255.0f;
			float w = v.w() * 0.5f * 255.0f; // we don't need to pack w...
			int xBits = (int)x;
			int yBits = (int)y;
			int zBits = (int)z;
			int wBits = (int)w;
			int xyzwBits = ((wBits << 24) & 0xFF000000) | ((zBits << 16) & 0x00FF0000) | ((yBits << 8) & 0x0000FF00) | (xBits & 0x000000FF);
			*(int*)floatPtr++ = xyzwBits;		
		}
		if (curVertex.HasComponent(MeshBuilderVertex::BinormalBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::BinormalIndex);
			*floatPtr++ = v.x();
			*floatPtr++ = v.y();
			*floatPtr++ = v.z();
		}
		//check for UB4NBits
		if (curVertex.HasComponent(MeshBuilderVertex::BinormalB4NBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::BinormalB4NIndex);
			float x = v.x() * 0.5f * 255.0f;
			float y = v.y() * 0.5f * 255.0f;
			float z = v.z() * 0.5f * 255.0f;
			float w = v.w() * 0.5f * 255.0f; // we don't need to pack w...
			int xBits = (int)x;
			int yBits = (int)y;
			int zBits = (int)z;
			int wBits = (int)w;
			int xyzwBits = ((wBits << 24) & 0xFF000000) | ((zBits << 16) & 0x00FF0000) | ((yBits << 8) & 0x0000FF00) | (xBits & 0x000000FF);
			*(int*)floatPtr++ = xyzwBits;		
		}
		if (curVertex.HasComponent(MeshBuilderVertex::WeightsBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::WeightsIndex);
			*floatPtr++ = v.x();
			*floatPtr++ = v.y();
			*floatPtr++ = v.z();
			*floatPtr++ = v.w();
		}
		//check for UB4NBits
		if (curVertex.HasComponent(MeshBuilderVertex::WeightsUB4NBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::WeightsUB4NIndex);
			float4 packedWeights = float4::normalize(v);
			//packedWeights = packedWeights * float4::dot3(packedWeights, float4(1,1,1,1));
			unsigned int xBits = (unsigned int)(packedWeights.x() * (255.0f));
			unsigned int yBits = (unsigned int)(packedWeights.y() * (255.0f));
			unsigned int zBits = (unsigned int)(packedWeights.z() * (255.0f));
			unsigned int wBits = (unsigned int)(packedWeights.w() * (255.0f));

			// special case for when a weight is 1 (vertex is affected by only 1 joint)
			if (xBits == 256)
			{
				xBits--;
			}
			if (yBits == 256)
			{
				yBits--;
			}
			if (zBits == 256)
			{
				zBits--;
			}
			if (wBits == 256)
			{
				wBits--;
			}

			int xyzwBits = ((wBits << 24) & 0xFF000000) | ((zBits << 16) & 0x00FF0000) | ((yBits << 8) & 0x0000FF00) | (xBits & 0x000000FF);
			*(int*)floatPtr++ = xyzwBits;	
		}
		if (curVertex.HasComponent(MeshBuilderVertex::JIndicesBit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::JIndicesIndex);
			*floatPtr++ = v.x();
			*floatPtr++ = v.y();
			*floatPtr++ = v.z();
			*floatPtr++ = v.w();
		}
		//check for UB4NBits
		if (curVertex.HasComponent(MeshBuilderVertex::JIndicesUB4Bit))
		{
			const float4& v = curVertex.GetComponent(MeshBuilderVertex::JIndicesUB4Index);
			int xBits = (int)(v.x()); 
			int yBits = (int)(v.y());
			int zBits = (int)(v.z());
			int wBits = (int)(v.w());

			int xyzwBits = ((wBits << 24) & 0xFF000000) | ((zBits << 16) & 0x00FF0000) | ((yBits << 8) & 0x0000FF00) | (xBits & 0x000000FF);
			*(int*)floatPtr++ = xyzwBits;		
		}
	}
	// write mesh to stream
	stream->Write(floatBuffer, numVertices * vertexWidth * sizeof(float));
	n_delete_array(floatBuffer);
	floatBuffer = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderSaver::WriteTriangles(const Ptr<Stream>& stream, MeshBuilder& meshBuilder, const ByteOrder& byteOrder)
{
	int numTriangles = meshBuilder.GetNumTriangles();
	uint* uintBuffer = n_new_array(uint, numTriangles * 3);
	uint* uintPtr = uintBuffer;
	int curTriangleIndex;
	for (curTriangleIndex = 0; curTriangleIndex < numTriangles; curTriangleIndex++)
	{
		const MeshBuilderTriangle& curTriangle = meshBuilder.TriangleAt(curTriangleIndex);
		int i0, i1, i2;
		curTriangle.GetVertexIndices(i0, i1, i2);
		*uintPtr++ = (uint) i0;
		*uintPtr++ = (uint) i1;
		*uintPtr++ = (uint) i2;
	}
	// write triangle to stream
	stream->Write(uintBuffer, numTriangles * 3 * sizeof(uint));
	n_delete_array(uintBuffer);
	uintBuffer = 0;
		

}


} // namespace ToolkitUtil
