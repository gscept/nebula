//------------------------------------------------------------------------------
//  bruteforcesystem.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "bruteforcesystem.h"
#include "math/clipstatus.h"
#include "math/mat4.h"
#include "profiling/profiling.h"
#include "models/modelcontext.h"
namespace Visibility
{

//------------------------------------------------------------------------------
/**
*/
void 
BruteforceSystemJobFunc(const Jobs::JobFuncContext& ctx)
{
    N_SCOPE(BruteforceViewFrustumCulling, Visibility);
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
        const Math::bbox* bbox = (const Math::bbox*)N_JOB_INPUT(ctx, sliceIdx, 0);
        uint32_t& entityFlags = *(uint32_t*)N_JOB_INPUT(ctx, sliceIdx, 1);

        // If not active, skip
        if (!AnyBits(entityFlags, Models::NodeInstanceFlags::NodeInstance_Active | Models::NodeInstanceFlags::NodeInstance_LodActive))
        {
            entityFlags = UnsetBits(entityFlags, Models::NodeInstance_Visible);
            continue;
        }

        // If already visible, skip
        if (AllBits(entityFlags, Models::NodeInstanceFlags::NodeInstance_Visible))
        {
            continue;
        }

        // If always visible, just set flag and continue
        if (AllBits(entityFlags, Models::NodeInstanceFlags::NodeInstance_AlwaysVisible))
        {
            entityFlags = SetBits(entityFlags, Models::NodeInstance_Visible);
            continue;
        }

        // If we want to check visibility, run clip check
        Math::ClipStatus::Type clipStatus = bbox->clipstatus(m_col_x, m_col_y, m_col_z, m_col_w);
        switch (clipStatus)
        {
            case Math::ClipStatus::Clipped:
            case Math::ClipStatus::Inside:
                entityFlags = SetBits(entityFlags, Models::NodeInstance_Visible);
                break;
            case Math::ClipStatus::Outside:
                entityFlags = UnsetBits(entityFlags, Models::NodeInstance_Visible);
                break;
        }
        /*
        Math::ClipStatus::Type* flag = (Math::ClipStatus::Type*)N_JOB_OUTPUT(ctx, sliceIdx, 0);

        *flag = bbox->clipstatus(m_col_x, m_col_y, m_col_z, m_col_w);
        */
    }
}

} // namespace Visibility
