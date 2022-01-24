//------------------------------------------------------------------------------
// vkpipelinedatabase.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkpipelinedatabase.h"
#include "vkshaderprogram.h"
#include "vkpass.h"
#include "coregraphics/shadercache.h"

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
        this->SetVertexLayout(this->currentVertexLayout);
    }
    else
    {
        this->ct3 = tierNodeAllocator.Alloc<Tier3Node>();
        this->ct2->children.Add(program, this->ct3);
        this->SetVertexLayout(this->currentVertexLayout);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetVertexLayout(const VkPipelineVertexInputStateCreateInfo* layout)
{
    this->currentVertexLayout = layout;
    IndexT index = this->ct3->children.FindIndex(layout);
    if (index != InvalidIndex)
    {
        this->ct4 = this->ct3->children.ValueAtIndex(index);
        this->SetInputLayout(this->currentInputAssemblyInfo);
    }
    else
    {
        this->ct4 = tierNodeAllocator.Alloc<Tier4Node>();
        this->ct3->children.Add(layout, this->ct4);
        this->SetInputLayout(this->currentInputAssemblyInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VkPipelineDatabase::SetInputLayout(const VkPipelineInputAssemblyStateCreateInfo* input)
{
    this->currentInputAssemblyInfo = input;
    IndexT index = this->ct4->children.FindIndex(input);
    if (index != InvalidIndex)
    {
        this->ct5 = this->ct4->children.ValueAtIndex(index);
    }
    else
    {
        this->ct5 = tierNodeAllocator.Alloc<Tier5Node>();
        this->ct4->children.Add(input, this->ct5);
    }
}

//------------------------------------------------------------------------------
/**
*/
VkPipeline
VkPipelineDatabase::GetCompiledPipeline()
{
    n_assert(this->dev != VK_NULL_HANDLE);
    n_assert(this->cache != VK_NULL_HANDLE);
    if (this->ct1->initial ||
        this->ct2->initial ||
        this->ct3->initial ||
        this->ct4->initial ||
        this->ct5->initial)
    {
        // get fragment of graphics pipeline residing in shader
        VkGraphicsPipelineCreateInfo shaderInfo = this->currentShaderInfo;

        // get other fragment from framebuffer
        VkGraphicsPipelineCreateInfo passInfo = PassGetVkFramebufferInfo(this->currentPass);
        VkPipelineColorBlendStateCreateInfo colorBlendInfo = *shaderInfo.pColorBlendState;
        colorBlendInfo.attachmentCount = PassGetNumSubpassAttachments(this->currentPass, this->currentSubpass);

        // use shader, framebuffer, vertex input and layout, input assembly and pass info to construct a complete pipeline
        VkGraphicsPipelineCreateInfo info =
        {
            VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            NULL,
            0,
            shaderInfo.stageCount,
            shaderInfo.pStages,
            this->currentVertexLayout,
            this->currentInputAssemblyInfo,
            shaderInfo.pTessellationState,
            passInfo.pViewportState,
            shaderInfo.pRasterizationState,
            shaderInfo.pMultisampleState,
            shaderInfo.pDepthStencilState,
            &colorBlendInfo,
            shaderInfo.pDynamicState,
            shaderInfo.layout,
            PassGetVkRenderPassBeginInfo(this->currentPass).renderPass,
            this->currentSubpass,
            VK_NULL_HANDLE,
            -1
        };
        VkResult res = vkCreateGraphicsPipelines(this->dev, this->cache, 1, &info, NULL, &this->currentPipeline);
        n_assert(res == VK_SUCCESS);
        this->ct5->pipeline = this->currentPipeline;

        // DAG path is created, so set entire path to not initial
        this->ct1->initial = this->ct2->initial = this->ct3->initial = this->ct4->initial = this->ct5->initial = false;
    }
    else
    {
        this->currentPipeline = this->ct5->pipeline;
    }

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
    , const VkGraphicsPipelineCreateInfo& gfxPipe)
{
    this->SetPass(pass);
    this->SetSubpass(subpass);
    this->SetShader(program, gfxPipe);
    this->SetVertexLayout(gfxPipe.pVertexInputState);
    this->SetInputLayout(gfxPipe.pInputAssemblyState);
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
    this->currentInputAssemblyInfo = 0;
    this->currentPipeline = VK_NULL_HANDLE;
    this->ct1 = NULL;
    this->ct2 = NULL;
    this->ct3 = NULL;
    this->ct4 = NULL;
    this->ct5 = NULL;
}

//------------------------------------------------------------------------------
/**
*/
void 
VkPipelineDatabase::Reload(const CoreGraphics::ShaderProgramId id)
{
    // save initial state
    Tier1Node* tmp1 = this->ct1;
    Tier2Node* tmp2 = this->ct2;
    Tier3Node* tmp3 = this->ct3;
    Tier4Node* tmp4 = this->ct4;
    Tier5Node* tmp5 = this->ct5;

    // walk through the tree and update every pipeline using the shader
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

                        IndexT m;
                        for (m = 0; m < t4->children.Size(); m++)
                        {
                            Tier5Node* t5 = t4->children.ValueAtIndex(m);
                            this->ct1 = t1;
                            this->ct2 = t2;
                            this->ct3 = t3;
                            this->ct4 = t4;
                            this->ct5 = t5;

                            // since t3 is initial, we will create a new pipeline
                            //this->GetCompiledPipeline();
                        }
                    }
                }
            }
        }
    }

    // reset old state
    this->ct1 = tmp1;
    this->ct2 = tmp2;
    this->ct3 = tmp3;
    this->ct4 = tmp4;
    this->ct5 = tmp5;
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

                    IndexT m;
                    for (m = 0; m < t4->children.Size(); m++)
                    {
                        Tier5Node* t5 = t4->children.ValueAtIndex(m);

                        if (t5->pipeline != VK_NULL_HANDLE)
                        {
                            // destroy any existing pipeline
                            vkDestroyPipeline(this->dev, t5->pipeline, nullptr);
                            t5->pipeline = VK_NULL_HANDLE;
                        }

                        // set t5 to initial, so that the next time we try to get compiled pipeline, it gets recreated
                        t5->initial = true;
                    }
                }
            }
        }
    }
}

} // namespace Vulkan
