//------------------------------------------------------------------------------
//  terrainculljob.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "jobs/jobs.h"
#include "math/clipstatus.h"
#include "math/bbox.h"
#include "profiling/profiling.h"

#include "terraincontext.h"
#include "terrain.h"
namespace Terrain
{

//------------------------------------------------------------------------------
/**
*/
void
TerrainSubTextureUpdateJob(const Jobs::JobFuncContext& ctx)
{
	N_SCOPE(TerrainSubTextureUpdateJob, Terrain);

	// get camera matrix
	const Math::mat4* camera = (const Math::mat4*)ctx.uniforms[0];
	const Terrain::SubTextureUpdateJobUniforms* uniforms = (const Terrain::SubTextureUpdateJobUniforms*)ctx.uniforms[1];

	for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
	{
		const Terrain::TerrainSubTexture* subTexture = (const Terrain::TerrainSubTexture*)N_JOB_INPUT(ctx, sliceIdx, 0);

		// control the maximum resolution as such, to get 10.24 texels/cm, we need to have 65536 pixels (theoretical) for a 64 meter region
		const uint maxResolution = uniforms->subTextureWorldSize * 1024;

		// distance in meters where we should switch lods
		const float switchDistance = 10.0f;

		// mask out y coordinate by multiplying result with, 1, 0 ,1
		Math::vec4 min = Math::vec4(subTexture->worldCoordinate[0], 0, subTexture->worldCoordinate[1], 0);
		Math::vec4 max = min + Math::vec4(uniforms->subTextureWorldSize, 0.0f, uniforms->subTextureWorldSize, 0.0f);
		Math::vec4 cameraXZ = camera->position * Math::vec4(1, 0, 1, 0);
		Math::vec4 nearestPoint = Math::minimize(Math::maximize(cameraXZ, min), max);
		float distance = length(nearestPoint - cameraXZ);

		// if we are outside the virtual area, just default the resolution to 0
		uint resolution = 0;
		uint lod = 0;
		if (distance > 300)
			goto skipResolution;

		// at every regular distance interval, increase t
		uint t = Math::n_max(1.0f, (distance / switchDistance));

		// calculate lod logarithmically, such that it goes geometrically slower to progress to higher lods
		lod = Math::n_min((uint)Math::n_log2(t), uniforms->maxMip);

		// calculate the resolution by offseting the max resolution with the lod
		resolution = maxResolution >> lod;

	skipResolution:

		// calculate the amount of tiles, which is the final lodded resolution divided by the size of a tile
		// the max being maxResolution and the smallest being 1
		uint tiles = resolution / uniforms->physicalTileSize;

		// only care about subtextures with at least 4 tiles
		tiles = tiles >= 4 ? tiles : 0;

		// produce output
		SubTextureUpdateJobOutput* output = (SubTextureUpdateJobOutput*)N_JOB_OUTPUT(ctx, sliceIdx, 0);
		output->oldMaxMip = subTexture->maxMip;
		output->oldTiles = subTexture->tiles;
		output->oldCoord = Math::uint2{ subTexture->indirectionOffset[0], subTexture->indirectionOffset[1] };

		output->newMaxMip = tiles > 0 ? Math::n_log2(tiles) : 0;
		output->newTiles = tiles;
		output->newCoord = Math::uint2{ 0xFFFFFFFF, 0xFFFFFFFF };

		// default is that the subtexture did not change
		output->updateState = SubTextureUpdateState::NoChange;

		// set state
		if (tiles >= 4)
		{
			if (subTexture->tiles == 0)
				output->updateState = SubTextureUpdateState::Created;
			else if (tiles > subTexture->tiles)
				output->updateState = SubTextureUpdateState::Grew;
			else if (tiles < subTexture->tiles)
				output->updateState = SubTextureUpdateState::Shrank;
		}
		else
		{
			// if tiles is now below the limit but it used to be bigger, delete 
			if (subTexture->tiles > 0)
				output->updateState = SubTextureUpdateState::Deleted;
		}
	}
}

} // namespace Terrain
