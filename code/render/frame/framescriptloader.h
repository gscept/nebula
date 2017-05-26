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
#include "coregraphics/shaderstate.h"
namespace CoreGraphics
{
	class Barrier;
}
namespace Frame2
{
class FrameScriptLoader
{
public:
	static Ptr<FrameScript> LoadFrameScript(const IO::URI& path);
private:
	/// main parse function
	static void ParseFrameScript(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse color texture list
	static void ParseColorTextureList(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse depth-stencil texture list
	static void ParseDepthStencilTextureList(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse image read-write texture list
	static void ParseImageReadWriteTextureList(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse image read-write buffer list
	static void ParseImageReadWriteBufferList(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse algorithm list
	static void ParseAlgorithmList(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse event list
	static void ParseEventList(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse shader state list
	static void ParseShaderStateList(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse global state
	static void ParseGlobalState(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse blit
	static void ParseBlit(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse subpass copy
	static void ParseCopy(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse compute
	static void ParseCompute(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse compute algorithm
	static void ParseComputeAlgorithm(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse swapbuffer
	static void ParseSwapbuffers(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse event
	static void ParseEvent(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse barrier in global scope
	static void ParseBarrier(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse core of barrier
	static void ParseBarrierInternal(const Ptr<Frame2::FrameScript>& script, JzonValue* node, const Ptr<CoreGraphics::Barrier>& barrier);

	/// parse pass
	static void ParsePass(const Ptr<Frame2::FrameScript>& script, JzonValue* node);
	/// parse attachment list
	static void ParseAttachmentList(const Ptr<Frame2::FrameScript>& script, const Ptr<CoreGraphics::Pass>& pass, JzonValue* node);
	/// parse subpass
	static void ParseSubpass(const Ptr<Frame2::FrameScript>& script, const Ptr<CoreGraphics::Pass>& pass, const Ptr<Frame2::FramePass>& framePass, JzonValue* node);
	/// parse subpass dependencies
	static void ParseSubpassDependencies(const Ptr<Frame2::FramePass>& pass, CoreGraphics::Pass::Subpass& subpass, JzonValue* node);
	/// parse subpass dependencies
	static void ParseSubpassAttachments(const Ptr<Frame2::FramePass>& pass, CoreGraphics::Pass::Subpass& subpass, JzonValue* node);
	/// parse subpass inputs
	static void ParseSubpassInputs(const Ptr<Frame2::FramePass>& pass, CoreGraphics::Pass::Subpass& subpass, JzonValue* node);
	/// parse subpass viewports
	static void ParseSubpassViewports(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass scissors
	static void ParseSubpassScissors(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass algorithm
	static void ParseSubpassAlgorithm(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass batch
	static void ParseSubpassBatch(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass sorted batch
	static void ParseSubpassSortedBatch(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse subpass post effect
	static void ParseSubpassFullscreenEffect(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse event in subpass
	static void ParseSubpassEvent(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse barrier in subpass
	static void ParseSubpassBarrier(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse system in subpass
	static void ParseSubpassSystem(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);
	/// parse plugins in subpass
	static void ParseSubpassPlugins(const Ptr<Frame2::FrameScript>& script, const Ptr<Frame2::FrameSubpass>& subpass, JzonValue* node);

	/// helper to parse shader variables
	static void ParseShaderVariables(const Ptr<Frame2::FrameScript>& script, const Ptr<CoreGraphics::ShaderState>& state, JzonValue* node);
};
} // namespace Frame2
