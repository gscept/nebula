#pragma once
//------------------------------------------------------------------------------
/**
    Loads a frame shader from JSON file
    
    @copyright
    (C) 2016-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "framescript.h" 
#include "framepass.h"
#include "framesubpass.h"
#include "framesubmission.h"
#include "resources/resourceid.h"
#include "io/uri.h"
#include "jzon-c/jzon.h"
#include "coregraphics/shader.h"

namespace Frame
{
class FrameScriptLoader
{
public:
    static Ptr<FrameScript> LoadFrameScript(const IO::URI& path);
private:
    /// Main parse function
    static void ParseFrameScript(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse texture list
    static void ParseTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse image read-write buffer list
    static void ParseReadWriteBufferList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse blit
    static FrameOp* ParseBlit(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse subpass copy
    static FrameOp* ParseCopy(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse resolve
    static FrameOp* ParseResolve(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse subpass copy
    static FrameOp* ParseMipmap(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse compute
    static FrameOp* ParseCompute(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse plugin (custom code execution)
    static FrameOp* ParsePlugin(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse subgraph
    static FrameOp* ParseSubgraph(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse barrier
    static FrameOp* ParseBarrier(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse swap
    static FrameOp* ParseSwap(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse present
    static FrameOp* ParsePresent(const Ptr<Frame::FrameScript>& script, JzonValue* node);

    /// Parse pass
    static FrameOp* ParsePass(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse frame submission phase
    static void ParseSubmissionList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
    /// Parse attachment list
    static void ParseAttachmentList(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
    /// parse subpass depth attachment
    static void ParseSubpassDepthAttachment(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
    /// parse subpass dependencies
    static void ParseSubpassResolves(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
    /// Parse subpass
    static void ParseSubpassList(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Frame::FramePass* framePass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
    /// Parse subpass dependencies
    static void ParseSubpassDependencyList(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, JzonValue* node);
    /// Parse subpass dependencies
    static void ParseSubpassAttachmentList(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
    /// Parse subpass inputs
    static void ParseSubpassInputList(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
    /// Parse subpass algorithm
    static void ParseSubpassPlugin(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
    /// Parse subpass subgraph
    static void ParseSubpassSubgraph(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
    /// Parse subpass batch
    static void ParseSubpassBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
    /// Parse subpass sorted batch
    static void ParseSubpassSortedBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
    /// Parse subpass post effect
    static void ParseSubpassFullscreenEffect(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);


    /// Helper to parse shader state
    static void ParseShaderState(const Ptr<Frame::FrameScript>& script, JzonValue* node, CoreGraphics::ShaderId& shd, CoreGraphics::ResourceTableId& table, Util::Dictionary<Util::StringAtom, CoreGraphics::BufferId>& constantBuffers, Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId, CoreGraphics::TextureId>>& textures);
    /// Helper to parse shader variables
    static void ParseShaderVariables(const Ptr<Frame::FrameScript>& script, const CoreGraphics::ShaderId& shd, CoreGraphics::ResourceTableId& table, Util::Dictionary<Util::StringAtom, CoreGraphics::BufferId>& constantBuffers, Util::Array<Util::Tuple<IndexT, CoreGraphics::BufferId, CoreGraphics::TextureId>>& textures, JzonValue* node);
    /// Helper to parse resources
    static void ParseResourceDependencies(const Ptr<Frame::FrameScript>& script, Frame::FrameOp* op, JzonValue* node);
    
    static Frame::FrameSubmission* LastSubmission[2];
};
} // namespace Frame2
