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
	/// parse texture list
	static void ParseTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse image read-write buffer list
	static void ParseReadWriteBufferList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse algorithm list
	static void ParsePluginList(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse blit
	static void ParseBlit(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse subpass copy
	static void ParseCopy(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse subpass copy
	static void ParseMipmap(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse compute
	static void ParseCompute(const Ptr<Frame::FrameScript>& script, JzonValue* node);
	/// parse compute algorithm
	static void ParsePlugin(const Ptr<Frame::FrameScript>& script, JzonValue* node);
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
	static void ParseSubpassPlugin(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse subpass batch
	static void ParseSubpassBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse subpass sorted batch
	static void ParseSubpassSortedBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);
	/// parse subpass post effect
	static void ParseSubpassFullscreenEffect(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node);

	/// helper to parse shader state
	static void ParseShaderState(const Ptr<Frame::FrameScript>& script, JzonValue* node, CoreGraphics::ShaderId& shd, CoreGraphics::ResourceTableId& table, Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& constantBuffers, Util::Array<std::tuple<IndexT, CoreGraphics::ConstantBufferId, CoreGraphics::TextureId>>& textures);
	/// helper to parse shader variables
	static void ParseShaderVariables(const Ptr<Frame::FrameScript>& script, const CoreGraphics::ShaderId& shd, CoreGraphics::ResourceTableId& table, Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& constantBuffers, Util::Array<std::tuple<IndexT, CoreGraphics::ConstantBufferId, CoreGraphics::TextureId>>& textures, JzonValue* node);
	/// helper to parse resources
	static void ParseResourceDependencies(const Ptr<Frame::FrameScript>& script, Frame::FrameOp* op, JzonValue* node);
	
	static Frame::FrameSubmission* LastSubmission[CoreGraphics::NumQueryTypes];
	typedef Frame::FramePlugin* (*Fn)(Memory::ArenaAllocator<BIG_CHUNK>&);
	static Util::HashTable<uint, Fn> constructors;
};
} // namespace Frame2
