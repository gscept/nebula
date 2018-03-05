#pragma once
//------------------------------------------------------------------------------
/**
	Loads a frame shader from JSON file
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "framescript.h" 
#include "framepass.h"
#include "framesubpass.h"
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
	/// main parse function
	static void ParseFrameScript(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse color texture list
	static void ParseColorTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse depth-stencil texture list
	static void ParseDepthStencilTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse image read-write texture list
	static void ParseImageReadWriteTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse image read-write buffer list
	static void ParseImageReadWriteBufferList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse algorithm list
	static void ParseAlgorithmList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse event list
	static void ParseEventList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse shader state list
	static void ParseShaderStateList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse global state
	static void ParseGlobalState(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse blit
	static void ParseBlit(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse subpass copy
	static void ParseCopy(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse compute
	static void ParseCompute(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse compute algorithm
	static void ParseComputeAlgorithm(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse swapbuffer
	static void ParseSwapbuffers(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse event
	static void ParseEvent(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse barrier in global scope
	static void ParseBarrier(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse core of barrier
	static void ParseBarrierInternal(const Ptr<Frame::FrameScript>& script, JzonValue* node, CoreGraphics::BarrierCreateInfo& barrier);

	/// parse pass
	static void ParsePass(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse attachment list
	static void ParseAttachmentList(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
	/// parse subpass
	static void ParseSubpass(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, const Ptr<Frame::FramePass>& framePass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
	/// parse subpass dependencies
	static void ParseSubpassDependencies(const Ptr<Frame::FramePass>& pass, CoreGraphics::Subpass& subpass, JzonValue* node);
	/// parse subpass dependencies
	static void ParseSubpassAttachments(const Ptr<Frame::FramePass>& pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
	/// parse subpass inputs
	static void ParseSubpassInputs(const Ptr<Frame::FramePass>& pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
	/// parse subpass viewports
	static void ParseSubpassViewports(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass scissors
	static void ParseSubpassScissors(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass algorithm
	static void ParseSubpassAlgorithm(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass batch
	static void ParseSubpassBatch(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass sorted batch
	static void ParseSubpassSortedBatch(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass post effect
	static void ParseSubpassFullscreenEffect(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse event in subpass
	static void ParseSubpassEvent(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse barrier in subpass
	static void ParseSubpassBarrier(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse system in subpass
	static void ParseSubpassSystem(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);
	/// parse plugins in subpass
	static void ParseSubpassPlugins(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node);

	/// helper to parse shader variables
	static void ParseShaderVariables(const Ptr<Frame::FrameScript>& script, const CoreGraphics::ShaderStateId& state, JzonValue* node);
};
} // namespace Frame2
