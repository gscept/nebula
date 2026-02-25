//------------------------------------------------------------------------------
// vkpipelinedatabase.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkpipelinedatabase.h"
#include "vkpipeline.h"
#include "vkpass.h"
#include "vktypes.h"

namespace Vulkan
{

__ImplementSingleton(VkPipelineDatabase);
//------------------------------------------------------------------------------
/**
*/
VkPipelineDatabase::VkPipelineDatabase() :
    dev(VK_NULL_HANDLE),
    cache(VK_NULL_HANDLE)
{
    __ConstructSingleton;
    this->Reset();
}

//------------------------------------------------------------------------------
/**
*/
VkPipelineDatabase::~VkPipelineDatabase()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::Setup(const VkDevice dev, const VkPipelineCache cache)
{
    this->dev = dev;
    this->cache = cache;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::Discard()
{
    IndexT i;
    for (i = 0; i < this->tier1.Size(); i++)
    {
        Tier1Node* t1 = this->tier1.ValueAtIndex(i);

        IndexT j;
        for (j = 0; j < t1->children.Size(); j++)
        {
            Tier2Node* t2 = t1->children.ValueAtIndex(j);

            IndexT k;
            for (k = 0; k < t2->children.Size(); k++)
            {
                Tier3Node* t3 = t2->children.ValueAtIndex(k);

                IndexT l;
                for (l = 0; l < t3->children.Size(); l++)
                {
                    Tier4Node* t4 = t3->children.ValueAtIndex(l);
                    CoreGraphics::DestroyGraphicsPipeline(t4->pipeline);
                    t4->pipeline = CoreGraphics::InvalidPipelineId;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetPass(const CoreGraphics::PassId pass)
{
    this->currentPass = pass;
    IndexT index = this->tier1.FindIndex(pass);
    if (index != InvalidIndex)
    {
        this->ct1 = this->tier1.ValueAtIndex(index);
        this->SetSubpass(this->currentSubpass);
    }
    else
    {
        this->ct1 = tierNodeAllocator.Alloc<Tier1Node>();
        this->tier1.Add(pass, this->ct1);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetSubpass(uint32_t subpass)
{
    this->currentSubpass = subpass;
    IndexT index = this->ct1->children.FindIndex(subpass);
    if (index != InvalidIndex)
    {
        this->ct2 = this->ct1->children.ValueAtIndex(index);
        this->SetShader(this->currentShaderProgram, this->currentShaderInfo);
    }
    else
    {
        this->ct2 = tierNodeAllocator.Alloc<Tier2Node>();
        this->ct1->children.Add(subpass, this->ct2);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetShader(const CoreGraphics::ShaderProgramId program, const VkGraphicsPipelineCreateInfo& gfxPipe)
{
    this->currentShaderProgram = program;
    this->currentShaderInfo = gfxPipe;
    IndexT index = this->ct2->children.FindIndex(program);
    if (index != InvalidIndex)
    {
        this->ct3 = this->ct2->children.ValueAtIndex(index);
        this->SetInputAssembly(this->currentInputAssembly);
    }
    else
    {
        this->ct3 = tierNodeAllocator.Alloc<Tier3Node>();
        this->ct2->children.Add(program, this->ct3);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetInputAssembly(const CoreGraphics::InputAssemblyKey key)
{
    this->currentInputAssembly = key;
    IndexT index = this->ct3->children.FindIndex(key);
    if (index != InvalidIndex)
    {
        this->ct4 = this->ct3->children.ValueAtIndex(index);
    }
    else
    {
        this->ct4 = tierNodeAllocator.Alloc<Tier4Node>();
        this->ct3->children.Add(key, this->ct4);
    }
}

//------------------------------------------------------------------------------
/**
*/
CoreGraphics::PipelineId
VkPipelineDatabase::GetPipeline(
    const CoreGraphics::PassId pass
    , const uint32_t subpass
    , const CoreGraphics::ShaderProgramId program
    , const CoreGraphics::InputAssemblyKey inputAssembly
    , const VkGraphicsPipelineCreateInfo& shaderInfo)
{
    this->SetPass(pass);
    this->SetSubpass(subpass);
    this->SetShader(program, shaderInfo);
    this->SetInputAssembly(inputAssembly);
    if (!this->ct1->initial && !this->ct2->initial && !this->ct3->initial && !this->ct4->initial)
    {
        return this->ct4->pipeline;
    }
    return CoreGraphics::InvalidPipelineId;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::CachePipeline(
    const CoreGraphics::PassId pass
    , const uint32_t subpass
    , const CoreGraphics::ShaderProgramId program
    , const CoreGraphics::InputAssemblyKey inputAssembly
    , const VkGraphicsPipelineCreateInfo& shaderInfo
    , CoreGraphics::PipelineId pipeline
)
{
    this->SetPass(pass);
    this->SetSubpass(subpass);
    this->SetShader(program, shaderInfo);
    this->SetInputAssembly(inputAssembly);
    n_assert(this->ct4->initial);
    this->ct4->pipeline = pipeline;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::InvalidatePipeline(const CoreGraphics::PipelineId pipeline)
{
    // Hmm, this is hard actually, we have to walk the entire tree and find the pipeline, then set it to initial
}

//------------------------------------------------------------------------------
/**
*/
VkPipeline
VkPipelineDatabase::GetCompiledPipeline()
{
    n_assert(this->dev != VK_NULL_HANDLE);
    n_assert(this->cache != VK_NULL_HANDLE);
    if (
        this->ct1->initial ||
        this->ct2->initial ||
        this->ct3->initial ||
        this->ct4->initial
        )
    {
        // If we are set to invalidate pipelines, delete the old pipeline prior to creating
        if (this->ct4->pipeline != CoreGraphics::InvalidPipelineId)
            CoreGraphics::DestroyGraphicsPipeline(ct4->pipeline);

        CoreGraphics::PipelineCreateInfo pipeInfo;
        pipeInfo.inputAssembly = this->currentInputAssembly;
        pipeInfo.shader = this->currentShaderProgram;
        pipeInfo.pass = this->currentPass;
        pipeInfo.subpass = this->currentSubpass;
        pipeInfo.ignoreCache = true;
        this->ct4->pipeline = CoreGraphics::CreateGraphicsPipeline(pipeInfo);

        //this->CreatePipeline(this->currentPass, this->currentSubpass, this->currentShaderProgram, this->currentInputAssembly, this->currentShaderInfo);

        // DAG path is created, so set entire path to not initial
        this->ct1->initial = this->ct2->initial = this->ct3->initial = this->ct4->initial = false;
    }

    this->currentPipeline = Vulkan::PipelineGetVkPipeline(this->ct4->pipeline);
    return this->currentPipeline;
}

//------------------------------------------------------------------------------
/**
*/
VkPipeline
VkPipelineDatabase::GetCompiledPipeline(
    const CoreGraphics::PassId pass
    , const uint32_t subpass
    , const CoreGraphics::ShaderProgramId program
    , const CoreGraphics::InputAssemblyKey inputAssembly
    , const VkGraphicsPipelineCreateInfo& shaderInfo)
{
    this->SetPass(pass);
    this->SetSubpass(subpass);
    this->SetShader(program, shaderInfo);
    this->SetInputAssembly(inputAssembly);
    return this->GetCompiledPipeline();
}


//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::Reset()
{
    this->currentPass = CoreGraphics::InvalidPassId;
    this->currentSubpass = -1;
    this->currentShaderProgram = CoreGraphics::InvalidShaderProgramId;
    this->currentVertexLayout = 0;
    this->currentPipeline = VK_NULL_HANDLE;
    this->ct1 = NULL;
    this->ct2 = NULL;
    this->ct3 = NULL;
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::Reload(const CoreGraphics::ShaderProgramId id)
{
    /*
    // save initial state
    Tier1Node* tmp1 = this->ct1;
    Tier2Node* tmp2 = this->ct2;
    Tier3Node* tmp3 = this->ct3;

    // walk through the tree and update every pipeline using the shader
    IndexT i;
    for (i = 0; i < this->tier1.Size(); i++)
    {
        Tier1Node* t1 = this->tier1.ValueAtIndex(i);
        const CoreGraphics::PassId pass = this->tier1.KeyAtIndex(i);

        IndexT j;
        for (j = 0; j < t1->children.Size(); j++)
        {
            Tier2Node* t2 = t1->children.ValueAtIndex(j);
            const uint subpass = t1->children.KeyAtIndex(j);

            IndexT k;
            for (k = 0; k < t2->children.Size(); k++)
            {
                const CoreGraphics::ShaderProgramId& prog = t2->children.KeyAtIndex(k);
                Tier3Node* t3 = t2->children.ValueAtIndex(k);
                // shaders match, so start a reload!
                if (prog == id)
                {
                    t3->initial = true;
                    IndexT l;
                    for (l = 0; l < t3->children.Size(); l++)
                    {
                        Tier4Node* t4 = t3->children.ValueAtIndex(l);
                        CoreGraphics::InputAssemblyKey inputAssembly = t3->children.KeyAtIndex(l);

                        // Destroy pipeline first
                        vkDestroyPipeline(this->dev, t4->pipeline, nullptr);
                        t4->pipeline = VK_NULL_HANDLE;
                        t4->initial = true;

                        // Setup all state
                        this->ct1 = t1;
                        this->ct2 = t2;
                        this->ct3 = t3;
                        this->ct4 = t4;

                        t4->pipeline = this->CreatePipeline(pass, subpass, prog, inputAssembly);

                        // Now requ
                        this->GetCompiledPipeline();
                    }
                }
            }
        }
    }

    // reset old state
    this->ct1 = tmp1;
    this->ct2 = tmp2;
    this->ct3 = tmp3;
    */
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::RecreatePipelines()
{
    IndexT i;
    for (i = 0; i < this->tier1.Size(); i++)
    {
        Tier1Node* t1 = this->tier1.ValueAtIndex(i);

        IndexT j;
        for (j = 0; j < t1->children.Size(); j++)
        {
            Tier2Node* t2 = t1->children.ValueAtIndex(j);

            IndexT k;
            for (k = 0; k < t2->children.Size(); k++)
            {
                Tier3Node* t3 = t2->children.ValueAtIndex(k);

                IndexT l;
                for (l = 0; l < t3->children.Size(); l++)
                {
                    Tier4Node* t4 = t3->children.ValueAtIndex(l);

                    // Set the tree to initial to provoke a new pipeline
                    t1->initial = t2->initial = t3->initial = t4->initial = true;
                }
            }
        }
    }
}

} // namespace Vulkan
