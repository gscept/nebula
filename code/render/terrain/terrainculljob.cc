//------------------------------------------------------------------------------
//  terrainculljob.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "jobs/jobs.h"
#include "math/clipstatus.h"
#include "math/bbox.h"
#include "profiling/profiling.h"
namespace Terrain
{

//------------------------------------------------------------------------------
/**
*/
void
TerrainCullJob(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE(TerrainCullJob, Terrain);

    const Math::mat4* camera = (const Math::mat4*)ctx.uniforms[0];

    // splat the matrix such that all _x, _y, ... will contain the column values of x, y, ...
    Math::vec4 m_col_x[4];
    Math::vec4 m_col_y[4];
    Math::vec4 m_col_z[4];
    Math::vec4 m_col_w[4];
    m_col_x[0] = Math::splat_x((*camera).r[0]);
    m_col_x[1] = Math::splat_x((*camera).r[1]);
    m_col_x[2] = Math::splat_x((*camera).r[2]);
    m_col_x[3] = Math::splat_x((*camera).r[3]);

    m_col_y[0] = Math::splat_y((*camera).r[0]);
    m_col_y[1] = Math::splat_y((*camera).r[1]);
    m_col_y[2] = Math::splat_y((*camera).r[2]);
    m_col_y[3] = Math::splat_y((*camera).r[3]);

    m_col_z[0] = Math::splat_z((*camera).r[0]);
    m_col_z[1] = Math::splat_z((*camera).r[1]);
    m_col_z[2] = Math::splat_z((*camera).r[2]);
    m_col_z[3] = Math::splat_z((*camera).r[3]);

    m_col_w[0] = Math::splat_w((*camera).r[0]);
    m_col_w[1] = Math::splat_w((*camera).r[1]);
    m_col_w[2] = Math::splat_w((*camera).r[2]);
    m_col_w[3] = Math::splat_w((*camera).r[3]);

    for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
    {
        const Math::bbox* boxes = (const Math::bbox*)N_JOB_INPUT(ctx, sliceIdx, 0);
        bool* flag = (bool*)N_JOB_OUTPUT(ctx, sliceIdx, 0);
        *flag = boxes->clipstatus(m_col_x, m_col_y, m_col_z, m_col_w) != Math::ClipStatus::Outside;
    }
}

} // namespace Terrain
