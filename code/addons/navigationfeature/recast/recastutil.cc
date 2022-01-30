//  navigation/recast/recastutil.cc
//  (C) 2014-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "Recast.h"
#include "recastutil.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "memory/memory.h"
#include "coregraphics/legacy/nvx2streamreader.h"
#include "coregraphics/primitivegroup.h"
#include "io/ioserver.h"


using namespace Math;
using namespace Util;
namespace Navigation
{
namespace Recast
{

//------------------------------------------------------------------------------
/**

RecastUtil::RecastUtil():
	maxEdgeLength(12),
	cellHeight(0.2f),
	cellSize(0.3f),
	maxEdgeError(1.3f),
	regionMinSize(8),
	regionMergeSize(20),
	detailSampleDist(6),
	detailSampleMaxError(1.0f),
	maxSlope(45.0f),
    config(0)
*/


//------------------------------------------------------------------------------
/**
*/
static void 
SetupConfig(NavMeshSettingsT const& settings, AgentT& kind, Math::vec3 const& bounds_center, Math::vec3 const& bounds_extents, rcConfig& config)
{		
	Memory::Clear(&config,sizeof(rcConfig));
    config.cs = settings.cell_size;
	config.ch = settings.cell_height;
    config.walkableSlopeAngle = settings.max_slope;
	config.walkableHeight = (int)ceilf(kind.agent_height / config.ch);
	config.walkableClimb = (int)floorf(kind.agent_max_climb / config.ch);
	config.walkableRadius = (int)ceilf(kind.agent_radius / config.cs);
	config.maxEdgeLen = (int)(settings.max_edge_length / config.cs);
	config.maxSimplificationError = settings.max_edge_error;
	config.minRegionArea = settings.region_min_size * settings.region_min_size;
    config.mergeRegionArea = settings.region_merge_size * settings.region_merge_size;
	config.maxVertsPerPoly = 6;
	config.detailSampleDist = settings.detail_sample_dist < 0.9f ? 0 : settings.cell_size * settings.detail_sample_dist;
	config.detailSampleMaxError = settings.cell_height * settings.detail_sample_max_error;
	point bmax = bounds_center + bounds_extents;
	point bmin = bounds_center - bounds_extents;

	for(int i = 0 ; i < 3 ; i++)
	{
		config.bmax[i] = bmax[i];
		config.bmin[i] = bmin[i];
	}


}


//------------------------------------------------------------------------------
/**
*/
bool
GenerateNavMesh(NavMeshT const& data, Util::Blob& meshData)
{
    rcConfig config;

    SetupConfig(*data.navmesh_settings, *data.agent_kind, data.bounds_center, data.bounds_extents, config);

	unsigned char* m_triareas = NULL;	
	rcHeightfield* m_solid;
	rcCompactHeightfield* m_chf;
	rcContourSet* m_cset;
	rcPolyMesh* m_pmesh;	
	rcPolyMeshDetail* m_dmesh;
	rcContext m_ctx;
	
	
	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.
	rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

	//
	// Step 1. Rasterize input polygon soup.
	//

	// Allocate voxel heightfield where we rasterize our input data to.
	m_solid = rcAllocHeightfield();
	n_assert(m_solid);
	rcCreateHeightfield(&m_ctx, *m_solid, config.width, config.height, config.bmin, config.bmax, config.cs, config.ch);
	
	// loop through all the mesh files
	for(auto const& entry : data.sources)
	{
        IO::URI meshFile = entry->resource;
        const Math::mat4& transform = entry->transform;
		Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(meshFile);
		Ptr<Legacy::Nvx2StreamReader> nvx2Reader = Legacy::Nvx2StreamReader::Create();
		nvx2Reader->SetStream(stream);
		nvx2Reader->SetUsage(CoreGraphics::GpuBufferTypes::UsageImmutable);
		nvx2Reader->SetAccess(CoreGraphics::GpuBufferTypes::AccessNone);
		nvx2Reader->SetRawMode(true);

		if (nvx2Reader->Open(nullptr))
		{
			
			const Util::Array<CoreGraphics::PrimitiveGroup>& groups = nvx2Reader->GetPrimitiveGroups();		

			float *vertexData = nvx2Reader->GetVertexData();
			unsigned int *indexData = (unsigned int*)nvx2Reader->GetIndexData();
            float* transformedData = (float*)Memory::Alloc(Memory::ScratchHeap, nvx2Reader->GetNumVertices() * 3 * sizeof(float));
			IndexT count = 0;
 			for(int i = 0, k = nvx2Reader->GetNumVertices(); i < k ; ++i)
 			{
 				unsigned int offset = i * nvx2Reader->GetVertexWidth();
 				vec4 v(vertexData[offset],vertexData[offset+1],vertexData[offset+2],1);
                vec4 trans = transform * v;
 				for(int j = 0; j < 3 ; j++)
 				{
                    transformedData[i * 3 + j] = trans[j];
 				}
 			}
  			for(int i=0;i < groups.Size();i++)
			{
				int ntris = groups[i].GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList);
                int startingVertex = groups[i].GetBaseVertex() / nvx2Reader->GetVertexWidth();
                float* verts = &(transformedData[startingVertex]);
				int nverts = groups[i].GetNumVertices();
				int* tris = (int*) & (indexData[groups[i].GetBaseIndex()]);
				
				//n_assert2(groups[i].GetPrimitiveTopology() == CoreGraphics::PrimitiveTopology::TriangleList,"Only triangle lists are supported");
				
				m_triareas = (unsigned char*)Memory::Alloc(Memory::ScratchHeap, groups[i].GetNumPrimitives(CoreGraphics::PrimitiveTopology::TriangleList));
				Memory::Clear(m_triareas,ntris * sizeof(unsigned char));		

				rcMarkWalkableTriangles(&m_ctx, config.walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
				rcRasterizeTriangles(&m_ctx, verts, nverts, tris, m_triareas, ntris, *m_solid, config.walkableClimb);								
				Memory::Free(Memory::ScratchHeap, m_triareas);
			}		
			nvx2Reader->Close();

            Memory::Free(Memory::ScratchHeap, (void*)transformedData);
		}				
	}
	
	//
	// Step 2. Filter walkables surfaces.
	//

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	rcFilterLowHangingWalkableObstacles(&m_ctx, config.walkableClimb, *m_solid);
	rcFilterLedgeSpans(&m_ctx, config.walkableHeight, config.walkableClimb, *m_solid);
	rcFilterWalkableLowHeightSpans(&m_ctx, config.walkableHeight, *m_solid);


	//
	// Step 4. Partition walkable surface to simple regions.
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	m_chf = rcAllocCompactHeightfield();
	
	rcBuildCompactHeightfield(&m_ctx, config.walkableHeight, config.walkableClimb, *m_solid, *m_chf);
	

	
	rcFreeHeightField(m_solid);
	m_solid = 0;
	

	// Erode the walkable area by agent radius.
	rcErodeWalkableArea(&m_ctx, config.walkableRadius, *m_chf);
	
    // add area markers
    for(auto const& area : data.area_modifiers)
    {
        float* points = (float*)Memory::Alloc(Memory::ScratchHeap, 3 * sizeof(float) * area->convex_area_points.size());

        for (size_t i = 0, k = area->convex_area_points.size(); i < k; ++i)
        {
            vec3 p = area->convex_area_points[i];
            points[i * 3] = p.x;
            points[i * 3 + 1] = 0.0f;
            points[i * 3 + 2] = p.z;
        }
        rcMarkConvexPolyArea(&m_ctx, points, (int)area->convex_area_points.size(), data.bounds_center.y - data.bounds_extents.y, data.bounds_center.y + data.bounds_extents.y, area->area_id, *m_chf);
        Memory::Free(Memory::ScratchHeap, (void*)points);
    }

	// Prepare for region partitioning, by calculating distance field along the walkable surface.
	rcBuildDistanceField(&m_ctx, *m_chf);
	

	// Partition the walkable surface into simple regions without holes.
	rcBuildRegions(&m_ctx, *m_chf, 0, config.minRegionArea, config.mergeRegionArea);

	//
	// Step 5. Trace and simplify region contours.
	//

	// Create contours.
	m_cset = rcAllocContourSet();
	
	rcBuildContours(&m_ctx, *m_chf, config.maxSimplificationError, config.maxEdgeLen, *m_cset);
	

	//
	// Step 6. Build polygons mesh from contours.
	//

	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	
	rcBuildPolyMesh(&m_ctx, *m_cset, config.maxVertsPerPoly, *m_pmesh);
	
	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	//

	m_dmesh = rcAllocPolyMeshDetail();
	
	rcBuildPolyMeshDetail(&m_ctx, *m_pmesh, *m_chf, config.detailSampleDist, config.detailSampleMaxError, *m_dmesh);
	
	
	rcFreeCompactHeightfield(m_chf);
	m_chf = 0;
	rcFreeContourSet(m_cset);
	m_cset = 0;
	

	// At this point the navigation mesh data is ready, you can access it from m_pmesh.
	// See duDebugDrawPolyMesh or dtCreateNavMeshData as examples how to access the data.

	//
	// (Optional) Step 8. Create Detour data from Recast poly mesh.
	//

	// The GUI may allow more max points per polygon than Detour can handle.
	// Only build the detour navmesh if we do not exceed the limit.
	n_assert(config.maxVertsPerPoly <= DT_VERTS_PER_POLYGON);
	
	unsigned char* navData = 0;
	int navDataSize = 0;

	
	// Update poly flags from areas.
	for (int i = 0; i < m_pmesh->npolys; ++i)
	{
		if (m_pmesh->areas[i] == RC_WALKABLE_AREA)
		{
			m_pmesh->areas[i] = 1;
			m_pmesh->flags[i] = 1;
		}
		else
		{
			// custom area
			int id = m_pmesh->areas[i];
			m_pmesh->flags[i] = id & 255;
			m_pmesh->areas[i] = id >> 8;
		}
		
	}


	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));
	params.verts = m_pmesh->verts;
	params.vertCount = m_pmesh->nverts;
	params.polys = m_pmesh->polys;
	params.polyAreas = m_pmesh->areas;
	params.polyFlags = m_pmesh->flags;
	params.polyCount = m_pmesh->npolys;
	params.nvp = m_pmesh->nvp;
	params.detailMeshes = m_dmesh->meshes;
	params.detailVerts = m_dmesh->verts;
	params.detailVertsCount = m_dmesh->nverts;
	params.detailTris = m_dmesh->tris;
	params.detailTriCount = m_dmesh->ntris;
	/*
	params.offMeshConVerts = m_geom->getOffMeshConnectionVerts();
	params.offMeshConRad = m_geom->getOffMeshConnectionRads();
	params.offMeshConDir = m_geom->getOffMeshConnectionDirs();
	params.offMeshConAreas = m_geom->getOffMeshConnectionAreas();
	params.offMeshConFlags = m_geom->getOffMeshConnectionFlags();
	params.offMeshConUserID = m_geom->getOffMeshConnectionId();
	params.offMeshConCount = m_geom->getOffMeshConnectionCount();
	*/
	
	params.walkableHeight = data.agent_kind->agent_height;
	params.walkableRadius = data.agent_kind->agent_radius;
	params.walkableClimb = data.agent_kind->agent_max_climb;
	rcVcopy(params.bmin, m_pmesh->bmin);
	rcVcopy(params.bmax, m_pmesh->bmax);
	params.cs = config.cs;
	params.ch = config.ch;
	params.buildBvTree = true;

	dtCreateNavMeshData(&params, &navData, &navDataSize);


	
	if(navDataSize>0)
	{
		dtNavMesh navMesh;
		navMesh.init(navData,navDataSize,0);
		meshData.Set(navData, navDataSize);
        return true;
	}
    return false;
}

}
}
