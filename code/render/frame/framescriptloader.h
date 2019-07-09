#pragma once
//------------------------------------------------------------------------------
/**
	Loads a frame shader from JSON file
	
	(C) 2016-2018 Individual contributors, see AUTHORS file
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
	/// parse frame submission phase
	static void ParseFrameSubmission(const Ptr<Frame::FrameScript>& script, char startOrEnd, JzonValue* node);
	/// parse barrier
	static void ParseBarrier(const Ptr<Frame::FrameScript>& script, JzonValue* node);

	/// parse pass
	static void ParsePass(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse attachment list
	static void ParseAttachmentList(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
	/// parse subpass
	static void ParseSubpass(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Frame::FramePass* framePass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
	/// parse subpass dependencies
	static void ParseSubpassDependencies(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, JzonValue* node);
	/// parse subpass dependencies
	static void ParseSubpassAttachments(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
	/// parse subpass inputs
	static void ParseSubpassInputs(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node);
	/// parse subpass viewports
	static void ParseSubpassViewports(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse subpass scissors
	static void ParseSubpassScissors(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse subpass algorithm
	static void ParseSubpassAlgorithm(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse subpass batch
	static void ParseSubpassBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse subpass sorted batch
	static void ParseSubpassSortedBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse subpass post effect
	static void ParseSubpassFullscreenEffect(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse system in subpass
	static void ParseSubpassSystem(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse plugins in subpass
	static void ParseSubpassPlugins(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);

	/// helper to parse shader state
	static void ParseShaderState(const Ptr<Frame::FrameScript>& script, JzonValue* node, CoreGraphics::ShaderId& shd, CoreGraphics::ResourceTableId& table, Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& constantBuffers);
	/// helper to parse shader variables
	static void ParseShaderVariables(const Ptr<Frame::FrameScript>& script, const CoreGraphics::ShaderId& shd, CoreGraphics::ResourceTableId& table, Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& constantBuffers, JzonValue* node);
	/// helper to parse resources
	static void ParseResourceDependencies(const Ptr<Frame::FrameScript>& script, Frame::FrameOp* op, JzonValue* node);
	
	static Frame::FrameSubmission* LastSubmission;
	typedef Algorithms::Algorithm* (*Fn)(Memory::ArenaAllocator<BIG_CHUNK>&);
	static Util::HashTable<uint, Fn> constructors;
};
} // namespace Frame2
