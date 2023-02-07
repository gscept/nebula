//------------------------------------------------------------------------------
//  fbxmeshnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfmesh.h"
#include "model/meshutil/meshbuildervertex.h"
#include "ngltfscene.h"
#include "util/bitfield.h"
#include "jobs/jobs.h"
#include "meshprimitive.h"

namespace ToolkitUtil
{

using namespace Math;
using namespace Util;
using namespace CoreAnimation;
using namespace ToolkitUtil;

__ImplementClass(ToolkitUtil::NglTFMesh, 'ASMN', ToolkitUtil::NglTFNode);

//------------------------------------------------------------------------------
/**
*/
NglTFMesh::NglTFMesh()
{
    this->meshBuilder = new MeshBuilder();
}

//------------------------------------------------------------------------------
/**
*/
NglTFMesh::~NglTFMesh()
{
    delete this->meshBuilder;
}

//------------------------------------------------------------------------------
/**
*/
void 
NglTFMesh::Setup(Util::Array<MeshBuilder>& meshes
        , const Gltf::Mesh* gltfMesh
        , const Gltf::Document* scene
        , const ToolkitUtil::ExportFlags flags
)
{
    if (gltfMesh->primitives.Size() == 0)
        return;

    Jobs::CreateJobPortInfo portInfo;
    portInfo.name = "meshJobPort"_atm;
    portInfo.numThreads = Math::min(8, gltfMesh->primitives.Size());
    portInfo.priority = Threading::Thread::Priority::High; // UINT_MAX
    portInfo.affinity = System::Cpu::Core0 | System::Cpu::Core1 | System::Cpu::Core2 | System::Cpu::Core3 | System::Cpu::Core4 | System::Cpu::Core5 | System::Cpu::Core6 | System::Cpu::Core7;

    auto jobPort = Jobs::CreateJobPort(portInfo);

    Jobs::CreateJobSyncInfo syncInfo;
    syncInfo.callback = nullptr;

    auto jobInternalSync = Jobs::CreateJobSync(syncInfo);
    auto jobHostSync = Jobs::CreateJobSync({ nullptr });

    // Extract and pre-process primitives ------------

    Util::Array<PrimitiveJobInput> pjInputs;
    Util::Array<PrimitiveJobOutput> pjOutputs;

    for (auto const& primitive : gltfMesh->primitives)
    {
        PrimitiveJobInput input = {
            gltfMesh,
            &primitive,
            // TODO: attributes per primitive group
            flags
        };

        pjInputs.Append(input);

        PrimitiveJobOutput output;
        output.mesh = &meshes.Emplace();

        pjOutputs.Append(output);
    }

    Jobs::JobContext primitiveJobContext;
    primitiveJobContext.uniform.data[0] = scene;
    primitiveJobContext.uniform.dataSize[0] = 1;
    primitiveJobContext.uniform.numBuffers = 1;
    primitiveJobContext.uniform.scratchSize = 0;

    primitiveJobContext.input.numBuffers = 1;
    primitiveJobContext.input.data[0] = pjInputs.Begin();
    primitiveJobContext.input.dataSize[0] = sizeof(PrimitiveJobInput) * pjInputs.Size();
    primitiveJobContext.input.sliceSize[0] = sizeof(PrimitiveJobInput);

    primitiveJobContext.output.numBuffers = 1;
    primitiveJobContext.output.data[0] = pjOutputs.Begin();
    primitiveJobContext.output.dataSize[0] = sizeof(PrimitiveJobOutput) * pjOutputs.Size();
    primitiveJobContext.output.sliceSize[0] = sizeof(PrimitiveJobOutput);

    Jobs::CreateJobInfo primitiveJob{ &SetupPrimitiveGroupJobFunc };

    auto primitiveJobId = Jobs::CreateJob(primitiveJob);

    // run job
    Jobs::JobSchedule(primitiveJobId, jobPort, primitiveJobContext);
    Jobs::JobSyncThreadSignal(jobInternalSync, jobPort);
    Jobs::JobSyncHostWait(jobInternalSync);
    Jobs::DestroyJob(primitiveJobId);

    Jobs::DestroyJobPort(jobPort);
}

} // namespace ToolkitUtil