//------------------------------------------------------------------------------
// framescriptloader.cc
// (C) 2016-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framescriptloader.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "framepass.h"
#include "frameglobalstate.h"
#include "frameblit.h"
#include "framecopy.h"
#include "framemipmap.h"
#include "framecompute.h"
#include "frameplugin.h"
#include "framesubpass.h"
#include "frameevent.h"
#include "framesubpassplugin.h"
#include "framesubpassbatch.h"
#include "framesubpassorderedbatch.h"
#include "framesubpassfullscreeneffect.h"
#include "frameserver.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/config.h"
#include "resources/resourceserver.h"
#include "core/factory.h"
#include "framesubpassplugin.h"
#include "framebarrier.h"
#include "coregraphics/barrier.h"
#include "coregraphics/displaydevice.h"
#include "memory/arenaallocator.h"
#include <mutex>

using namespace CoreGraphics;
using namespace IO;
namespace Frame
{

Frame::FrameSubmission* FrameScriptLoader::LastSubmission[CoreGraphics::NumQueryTypes] = {nullptr, nullptr};
//------------------------------------------------------------------------------
/**
*/
Ptr<Frame::FrameScript>
FrameScriptLoader::LoadFrameScript(const IO::URI& path)
{
	Ptr<FrameScript> script = FrameScript::Create();
	Ptr<Stream> stream = IoServer::Instance()->CreateStream(path);
	if (stream->Open())
	{
		void* data = stream->Map();
		SizeT size = stream->GetSize();

		// create copy for jzon
		char* jzon_buf = n_new_array(char, size+1);
		memcpy(jzon_buf, data, size);
		jzon_buf[size] = '\0';

		stream->Unmap();
		stream->Close();

		// make sure last byte is 0, since jzon doesn't care about input size
		JzonParseResult result = jzon_parse(jzon_buf);
		JzonValue* json = result.output;
		if (!result.success)
		{
			n_error("jzon parse error around: '%.100s'\n", result.error);
		}

		// assert version is compatible
		JzonValue* node = jzon_get(json, "version");
		n_assert(node->int_value >= 2);
		node = jzon_get(json, "engine");
		n_assert(Util::String(node->string_value) == "Nebula");
		node = jzon_get(json, "sub_script");
		if (node) script->subScript = node->bool_value;

		// run parser entry point
		json = jzon_get(json, "framescript");
		ParseFrameScript(script, json);

		// clear jzon
		n_delete_array(jzon_buf);
		jzon_free(json);

	}
	else
	{
		n_error("Failed to load frame script '%s'\n", path.LocalPath().AsCharPtr());
	}
	return script;
}


//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseFrameScript(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		Util::String name(cur->key);
		if (name == "textures")							ParseTextureList(script, cur);
		else if (name == "read_write_buffers")			ParseReadWriteBufferList(script, cur);
		else if (name == "blit")						ParseBlit(script, cur);
		else if (name == "copy")						ParseCopy(script, cur);
		else if (name == "mipmap")						ParseMipmap(script, cur);
		else if (name == "compute")						ParseCompute(script, cur);
		else if (name == "plugin" || name == "call")	ParsePlugin(script, cur);
		else if (name == "pass")						ParsePass(script, cur);
		else if (name == "begin_submission")			ParseFrameSubmission(script, 0, cur); // 0 means begin
		else if (name == "end_submission")				ParseFrameSubmission(script, 1, cur); // 1 means end
		else if (name == "barrier")						ParseBarrier(script, cur);
		else if (name == "comment" || name == "_comment") continue; // just skip comments
		else
		{
			n_error("Frame script operation '%s' is unrecognized.\n", name.AsCharPtr());
		}
	}

	// set end of frame barrier if not a sub-script
	if (!script->subScript)
	{
		n_assert_fmt(FrameScriptLoader::LastSubmission[GraphicsQueueType] != nullptr, "Frame script '%s' must contain an end submission, otherwise the script is invalid!", script->GetResourceName().Value());
		FrameScriptLoader::LastSubmission[GraphicsQueueType]->resourceResetBarriers = &script->resourceResetBarriers;
		FrameScriptLoader::LastSubmission[GraphicsQueueType] = nullptr;
	}
	
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameScriptLoader::ParseTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != nullptr);
		if (Util::String(name->string_value) == "__WINDOW__")
		{
			// code to fetch window render texture goes here
			CoreGraphics::TextureId tex = FrameServer::Instance()->GetWindowTexture();
			script->AddTexture("__WINDOW__", tex);

			// save the window used for the relative dimensions used for this framescript
			script->window = DisplayDevice::Instance()->GetCurrentWindow();
		}
		else
		{
			JzonValue* format = jzon_get(cur, "format");
			n_assert(format != nullptr);
			JzonValue* width = jzon_get(cur, "width");
			n_assert(width != nullptr);
			JzonValue* height = jzon_get(cur, "height");
			n_assert(height != nullptr);

			JzonValue* usage = jzon_get(cur, "usage");
			n_assert(usage != nullptr);
			JzonValue* type = jzon_get(cur, "type");
			n_assert(type != nullptr);

			// get format
			CoreGraphics::PixelFormat::Code fmt = CoreGraphics::PixelFormat::FromString(format->string_value);

			// create texture
			TextureCreateInfo info;
			info.name = name->string_value;
			info.usage = TextureUsageFromString(usage->string_value);
			info.tag = "frame_script"_atm;
			info.type = TextureTypeFromString(type->string_value);
			info.format = fmt;

			if (JzonValue* alias = jzon_get(cur, "alias"))
				info.alias = script->GetTexture(alias->string_value);

			if (JzonValue* layers = jzon_get(cur, "layers"))
			{
				n_assert_fmt(info.type >= Texture1DArray, "Texture format must be array type if the layers value is set");
				info.layers = layers->int_value;
			}

			if (JzonValue* mips = jzon_get(cur, "mips"))
				if (Util::String(mips->string_value) == "auto")
					info.mips = TextureAutoMips;
				else
					info.mips = mips->int_value;

			if (JzonValue* depth = jzon_get(cur, "depth"))
				info.depth = (float)depth->float_value;

			// set relative, dynamic or msaa if defined
			if (jzon_get(cur, "relative"))	info.windowRelative = jzon_get(cur, "relative")->bool_value;
			if (jzon_get(cur, "samples"))	info.samples = jzon_get(cur, "samples")->int_value;
			if (jzon_get(cur, "sparse"))	info.sparse = jzon_get(cur, "sparse")->int_value;

			// set dimension after figuring out if the texture is a cube
			info.width = (float)width->float_value;
			info.height = (float)height->float_value;

			// add to script
			TextureId tex = CreateTexture(info);
			script->AddTexture(name->string_value, tex);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseReadWriteBufferList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != nullptr);
		JzonValue* size = jzon_get(cur, "size");
		n_assert(size != nullptr);

		// create shader buffer 
		BufferCreateInfo info;
		info.name = name->string_value;
		info.size = 1;
		info.elementSize = size->int_value;
		info.mode = CoreGraphics::DeviceLocal;
		info.usageFlags = CoreGraphics::ReadWriteBuffer;
		
		// add to script
		BufferId buf = CreateBuffer(info);
		script->AddBuffer(name->string_value, buf);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseBlit(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	FrameBlit* op = script->GetAllocator().Alloc<FrameBlit>();

	// set name of op
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphics::QueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != nullptr);
	const CoreGraphics::TextureId& fromTex = script->GetTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != nullptr);
	const CoreGraphics::TextureId& toTex = script->GetTexture(to->string_value);

	bool isDepth = CoreGraphics::PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(fromTex));

	// add implicit barriers
	ImageSubresourceInfo subres;
	subres.aspect = isDepth ? CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits : CoreGraphics::ImageAspect::ColorBits;
	subres.layer = 0;
	subres.layerCount = 1;
	subres.mip = 0;
	subres.mipCount = 1;
	op->textureDeps.Add(fromTex, Util::MakeTuple(from->string_value, CoreGraphics::BarrierAccess::TransferRead, CoreGraphics::BarrierStage::Transfer, subres, CoreGraphics::ImageLayout::TransferSource));
	op->textureDeps.Add(toTex, Util::MakeTuple(to->string_value, CoreGraphics::BarrierAccess::TransferWrite, CoreGraphics::BarrierStage::Transfer, subres, CoreGraphics::ImageLayout::TransferDestination));

	// setup blit operation
	op->from = fromTex;
	op->to = toTex;
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseCopy(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	FrameCopy* op = script->GetAllocator().Alloc<FrameCopy>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphics::QueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != nullptr);
	const CoreGraphics::TextureId& fromTex = script->GetTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != nullptr);
	const CoreGraphics::TextureId& toTex = script->GetTexture(to->string_value);

	bool isDepth = CoreGraphics::PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(fromTex));

	// add implicit barriers
	ImageSubresourceInfo subres;
	subres.aspect = isDepth ? (CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits) : CoreGraphics::ImageAspect::ColorBits;
	subres.layer = 0;
	subres.layerCount = 1;
	subres.mip = 0;
	subres.mipCount = 1;
	op->textureDeps.Add(fromTex, Util::MakeTuple(from->string_value, CoreGraphics::BarrierAccess::TransferRead, CoreGraphics::BarrierStage::Transfer, subres, CoreGraphics::ImageLayout::TransferSource));
	op->textureDeps.Add(toTex, Util::MakeTuple(to->string_value, CoreGraphics::BarrierAccess::TransferWrite, CoreGraphics::BarrierStage::Transfer, subres, CoreGraphics::ImageLayout::TransferDestination));

	// setup copy operation
	op->from = fromTex;
	op->to = toTex;
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseMipmap(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	FrameMipmap* op = script->GetAllocator().Alloc<FrameMipmap>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphics::QueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

	JzonValue* tex = jzon_get(node, "texture");
	n_assert(tex != nullptr);
	const CoreGraphics::TextureId& ttex = script->GetTexture(tex->string_value);
	bool isDepth = CoreGraphics::PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(ttex));

	// add implicit barriers
	ImageSubresourceInfo subres;
	subres.aspect = isDepth ? (CoreGraphics::ImageAspect::DepthBits | CoreGraphics::ImageAspect::StencilBits) : CoreGraphics::ImageAspect::ColorBits;
	subres.layer = 0;
	subres.layerCount = 1;
	subres.mip = 0;
	subres.mipCount = 1;
	op->textureDeps.Add(ttex, Util::MakeTuple(tex->string_value, CoreGraphics::BarrierAccess::TransferRead, CoreGraphics::BarrierStage::Transfer, subres, CoreGraphics::ImageLayout::TransferSource));

	// setup copy operation
	op->tex = ttex;
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseCompute(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{	
	FrameCompute* op = script->GetAllocator().Alloc<FrameCompute>();

	// get name of compute sequence
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphics::QueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

	JzonValue* inputs = jzon_get(node, "resource_dependencies");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	// create shader state
	JzonValue* shader = jzon_get(node, "shader_state");
	n_assert(shader != nullptr);
	ParseShaderState(script, shader, op->shader, op->resourceTable, op->constantBuffers, op->textures);

	JzonValue* variation = jzon_get(node, "variation");
	n_assert(variation != nullptr);
	op->program = ShaderGetProgram(op->shader, ShaderServer::Instance()->FeatureStringToMask(variation->string_value));

	// dimensions, must be 3
	if (JzonValue* dims = jzon_get(node, "dimensions"))
	{
		n_assert(dims->size == 3);
		op->x = dims->array_values[0]->int_value;
		op->y = dims->array_values[1]->int_value;
		op->z = dims->array_values[2]->int_value;
	}
	else if (JzonValue* dims = jzon_get(node, "texture_dimensions"))
	{
		CoreGraphics::TextureId tex = script->GetTexture(dims->string_value);
		CoreGraphics::TextureDimensions tDims = CoreGraphics::TextureGetDimensions(tex);
		op->x = tDims.width;
		op->y = tDims.height;
		op->z = tDims.depth;
	}

	// add op to script
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParsePlugin(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	auto callback = Frame::GetCallback(name->string_value);
	if (callback != nullptr)
	{
		FramePlugin* op = script->GetAllocator().Alloc<FramePlugin>();
		op->SetName(name->string_value);

		JzonValue* queue = jzon_get(node, "queue");
		if (queue == nullptr)
			op->queue = CoreGraphics::QueueType::GraphicsQueueType;
		else
			op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

		JzonValue* inputs = jzon_get(node, "resource_dependencies");
		if (inputs != nullptr)
		{
			ParseResourceDependencies(script, op, inputs);
		}

		// get algorithm
		op->func = callback;

		// add to script
		script->AddOp(op);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseBarrier(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	FrameBarrier* op = script->GetAllocator().Alloc<FrameBarrier>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	op->SetName(name->string_value);

	JzonValue* inputs = jzon_get(node, "resource_dependencies");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	// add operation to script
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void 
FrameScriptLoader::ParseFrameSubmission(const Ptr<Frame::FrameScript>& script, char startOrEnd, JzonValue* node)
{
	FrameSubmission* op = script->GetAllocator().Alloc<FrameSubmission>();
	op->startOrEnd = startOrEnd;

	// get function and name
	if (startOrEnd == 0)
	{
		JzonValue* name = jzon_get(node, "name");
		n_assert(name != nullptr);
		op->SetName(name->string_value);
	}
	else
	{
		op->SetName("End");
	}

	JzonValue* queue = jzon_get(node, "queue");
	op->queue = CoreGraphics::QueueTypeFromString(queue->string_value);

	// if we are starting a new submission, also look for the queue wait flag
	if (startOrEnd == 0)
	{
		JzonValue* waitQueue = jzon_get(node, "wait_for_queue");
		if (waitQueue)
			op->waitQueue = CoreGraphics::QueueTypeFromString(waitQueue->string_value);
	}
	else
	{
		// copy wait queue from previous begin on the same queue
		op->waitQueue = FrameScriptLoader::LastSubmission[op->queue]->waitQueue;
	}

	// remember the begin
	FrameScriptLoader::LastSubmission[op->queue] = op;

	// add operation to script
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParsePass(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	// create pass
	FramePass* op = script->GetAllocator().Alloc<FramePass>();

	PassCreateInfo info;

	// get name of pass
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	info.name = name->string_value;

	Util::Array<Resources::ResourceName> attachmentNames;

	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		Util::String name(cur->key);
		if (name == "name")					op->SetName(cur->string_value);
		else if (name == "attachments")		ParseAttachmentList(script, info, attachmentNames, cur);
		else if (name == "depth_stencil")
		{
			float clearDepth = 1;
			uint clearStencil = 0;
			AttachmentFlagBits depthStencilClearFlags = AttachmentFlagBits::NoFlags;
			JzonValue* cd = jzon_get(cur, "clear");
			if (cd != nullptr)
			{
				depthStencilClearFlags |= AttachmentFlagBits::Clear;
				info.clearDepth = (float)cd->float_value;
			}

			JzonValue* cs = jzon_get(cur, "clear_stencil");
			if (cs != nullptr)
			{
				depthStencilClearFlags |= AttachmentFlagBits::ClearStencil;
				info.clearStencil = cs->int_value;
			}

			JzonValue* ld = jzon_get(cur, "load");
			if (ld != nullptr && ld->bool_value)
			{
				n_assert2(cd == nullptr, "Can't load depth from previous pass AND clear.");				
				depthStencilClearFlags |= AttachmentFlagBits::Load;
			}

			JzonValue* ls = jzon_get(cur, "load_stencil");
			if (ls != nullptr && ls->bool_value)
			{
				// can't really load and store
				n_assert2(cs == nullptr, "Can't load stenil from previous pass AND clear.");
				depthStencilClearFlags |= AttachmentFlagBits::LoadStencil;
			}

			JzonValue* sd = jzon_get(cur, "store");
			if (sd != nullptr && sd->bool_value)
			{
				depthStencilClearFlags |= AttachmentFlagBits::Store;
			}

			JzonValue* ss = jzon_get(cur, "store_stencil");
			if (ss != nullptr && ss->bool_value)
			{
				depthStencilClearFlags |= AttachmentFlagBits::StoreStencil;
			}

			info.depthStencilFlags = (AttachmentFlagBits)depthStencilClearFlags;
			info.clearStencil = clearStencil;
			info.clearDepth = clearDepth;

			// set attachment in framebuffer
			JzonValue* ds = jzon_get(cur, "name");

			TextureId tex = script->GetTexture(ds->string_value);
			TextureViewCreateInfo viewCreate =
			{
				tex
				, 0, 1, 0, TextureGetNumLayers(tex)
				, TextureGetPixelFormat(tex)
			};
			info.depthStencilAttachment = CreateTextureView(viewCreate);
		}
		else if (name == "subpass")
			ParseSubpass(script, info, op, attachmentNames, cur);
		else
		{
			n_error("Passes don't support operations, and '%s' is no exception.\n", name.AsCharPtr());
		}
	}

	// setup framebuffer and bind to pass
	op->pass = CreatePass(info);
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseAttachmentList(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != nullptr);

		TextureId tex = script->GetTexture(name->string_value);
		TextureViewCreateInfo viewCreate =
		{
			tex
			, 0, 1, 0, TextureGetNumLayers(tex)
			, TextureGetPixelFormat(tex)
		};

		pass.colorAttachments.Append(CreateTextureView(viewCreate));
		attachmentNames.Append(name->string_value);

		// set clear flag if present
		JzonValue* clear = jzon_get(cur, "clear");
		AttachmentFlagBits flags = AttachmentFlagBits::NoFlags;
		if (clear != nullptr)
		{
			Math::vec4 clearValue;
			n_assert(clear->size <= 4);
			uint j;
			for (j = 0; j < clear->size; j++)
			{
				clearValue[j] = clear->array_values[j]->float_value;
			}
			pass.colorAttachmentClears.Append(clearValue);
			flags |= AttachmentFlagBits::Clear;
		}
		else
			pass.colorAttachmentClears.Append(Math::vec4(1)); // we set the clear to 1, but the flag is not to clear...

		// set if attachment should store at the end of the pass
		JzonValue* store = jzon_get(cur, "store");
		if (store && store->bool_value)
		{
			flags |= AttachmentFlagBits::Store;
		}

		JzonValue* load = jzon_get(cur, "load");
		if (load && load->bool_value)
		{
			// we can't really load and clear
			n_assert2(clear == nullptr, "Can't load color if it's being cleared.");
			flags |= AttachmentFlagBits::Load;
		}
		pass.colorAttachmentFlags.Append((AttachmentFlagBits)flags);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpass(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, Frame::FramePass* framePass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
	FrameSubpass* frameSubpass = script->GetAllocator().Alloc<FrameSubpass>();

	frameSubpass->domain = BarrierDomain::Pass;
	Subpass subpass;
	subpass.resolve = false;
	subpass.bindDepth = false;
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		Util::String name(cur->key);
		if (name == "name")								frameSubpass->SetName(cur->string_value);
		else if (name == "subpass_dependencies")		ParseSubpassDependencies(framePass, subpass, cur);
		else if (name == "attachments")					ParseSubpassAttachments(framePass, subpass, attachmentNames, cur);
		else if (name == "inputs")						ParseSubpassInputs(framePass, subpass, attachmentNames, cur);
		else if (name == "depth")						subpass.bindDepth = cur->bool_value;
		else if (name == "resolve")						subpass.resolve = cur->bool_value;
		else if (name == "resource_dependencies")		ParseResourceDependencies(script, framePass, cur);
		else if (name == "plugin" || name == "call")	ParseSubpassPlugin(script, frameSubpass, cur);
		else if (name == "batch")						ParseSubpassBatch(script, frameSubpass, cur);
		else if (name == "sorted_batch")				ParseSubpassSortedBatch(script, frameSubpass, cur);
		else if (name == "fullscreen_effect")			ParseSubpassFullscreenEffect(script, frameSubpass, cur);
		else if (name == "comment" || name == "_comment") continue; // just skip comments
		else
		{
			n_error("Subpass operation '%s' is invalid.\n", name.AsCharPtr());
		}
	}

	// correct amount of viewports and scissors if not set explicitly (both values default to 0)
	if (subpass.numViewports == 0)
		subpass.numViewports = subpass.attachments.Size();
	if (subpass.numScissors == 0)
		subpass.numScissors = subpass.attachments.Size();

	// link together frame operations
	pass.subpasses.Append(subpass);
	framePass->AddSubpass(frameSubpass);
}

//------------------------------------------------------------------------------
/**
	Extend to use subpass names
*/
void
FrameScriptLoader::ParseSubpassDependencies(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		if (cur->is_int)			subpass.dependencies.Append(cur->int_value);
		else if (cur->is_string)
		{
			Util::String id(cur->string_value);
			const Util::Array<FrameSubpass*>& subpasses = pass->GetSubpasses();
			IndexT j;
			for (j = 0; j < subpasses.Size(); j++)
			{
				if (subpasses[j]->GetName() == id)
				{
					subpass.dependencies.Append(j);
					break;
				}
			}
			if (j == subpasses.Size())
			{
				n_error("Could not find previous subpass '%s'", id.AsCharPtr());
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
	Extend to use attachment names
*/
void
FrameScriptLoader::ParseSubpassAttachments(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		if (cur->is_int)		subpass.attachments.Append(cur->int_value);
		else if (cur->is_string)
		{
			Util::String id(cur->string_value);
			IndexT j;
			for (j = 0; j < attachmentNames.Size(); j++)
			{
				if (attachmentNames[j] == id)
				{
					subpass.attachments.Append(j);
					break;
				}
			}
			if (j == attachmentNames.Size())
			{
				n_error("Could not find attachment '%s'", id.AsCharPtr());
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassInputs(Frame::FramePass* pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		if (cur->is_int)	subpass.inputs.Append(cur->int_value);
		else if (cur->is_string)
		{
			Util::String id(cur->string_value);
			IndexT j;
			for (j = 0; j < attachmentNames.Size(); j++)
			{
				if (attachmentNames[j] == id)
				{
					subpass.inputs.Append(j);
					break;
				}
			}
			if (j == attachmentNames.Size())
			{
				n_error("Could not find previous attachment '%s'", id.AsCharPtr());
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassPlugin(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	FrameSubpassPlugin* op = script->GetAllocator().Alloc<FrameSubpassPlugin>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	auto callback = Frame::GetCallback(name->string_value);
	if (callback != nullptr)
	{
		op->SetName(name->string_value);

		op->domain = BarrierDomain::Pass;
		op->queue = CoreGraphics::QueueType::GraphicsQueueType;

		JzonValue* inputs = jzon_get(node, "resource_dependencies");
		if (inputs != nullptr)
		{
			ParseResourceDependencies(script, op, inputs);
		}

		// get algorithm
		op->func = callback;

		// add to script
		subpass->AddOp(op);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	FrameSubpassBatch* op = script->GetAllocator().Alloc<FrameSubpassBatch>();
	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphics::QueueType::GraphicsQueueType;

	JzonValue* inputs = jzon_get(node, "resource_dependencies");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	op->batch = CoreGraphics::BatchGroup::FromName(node->string_value);
	subpass->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassSortedBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	FrameSubpassOrderedBatch* op = script->GetAllocator().Alloc<FrameSubpassOrderedBatch>();
	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphics::QueueType::GraphicsQueueType;

	JzonValue* inputs = jzon_get(node, "resource_dependencies");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	op->batch = CoreGraphics::BatchGroup::FromName(node->string_value);
	subpass->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassFullscreenEffect(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	FrameSubpassFullscreenEffect* op = script->GetAllocator().Alloc<FrameSubpassFullscreenEffect>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	op->SetName(name->string_value);

	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphics::QueueType::GraphicsQueueType;

	JzonValue* inputs = jzon_get(node, "resource_dependencies");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	// create shader state
	JzonValue* shaderState = jzon_get(node, "shader_state");
	n_assert(shaderState != nullptr);
	ParseShaderState(script, shaderState, op->shader, op->resourceTable, op->constantBuffers, op->textures);

	// get texture
	JzonValue* texture = jzon_get(node, "size_from_texture");
	n_assert(texture != nullptr);
	op->tex = script->GetTexture(texture->string_value);
	
	// add op to subpass
	op->Setup();
	subpass->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderState(
	const Ptr<Frame::FrameScript>& script, 
	JzonValue* node, 
	CoreGraphics::ShaderId& shd, 
	CoreGraphics::ResourceTableId& table, 
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& constantBuffers,
	Util::Array<Util::Tuple<IndexT, CoreGraphics::ConstantBufferId, CoreGraphics::TextureId>>& textures
)
{
	JzonValue* shader = jzon_get(node, "shader");
	n_assert(shader != nullptr);
    Util::String shaderRes = "shd:" + Util::String(shader->string_value) + ".fxb";
	shd = ShaderGet(shaderRes);
	table = ShaderCreateResourceTable(shd, NEBULA_BATCH_GROUP);

	JzonValue* vars = jzon_get(node, "variables");
	if (vars != nullptr)
		ParseShaderVariables(script, shd, table, constantBuffers, textures, vars);
	CoreGraphics::ResourceTableCommitChanges(table);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderVariables(
	const Ptr<Frame::FrameScript>& script,
	const CoreGraphics::ShaderId& shd, 
	CoreGraphics::ResourceTableId& table, 
	Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& constantBuffers, 
	Util::Array<Util::Tuple<IndexT, CoreGraphics::ConstantBufferId, CoreGraphics::TextureId>>& textures, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* var = node->array_values[i];

		// variables need to define both semantic and value
		JzonValue* sem = jzon_get(var, "semantic");
		n_assert(sem != nullptr);
		JzonValue* val = jzon_get(var, "value");
		n_assert(val != nullptr);
		Util::String valStr(val->string_value);

		// get variable
		ShaderConstantType type = ShaderGetConstantType(shd, sem->string_value);
		ConstantBufferId cbo = ConstantBufferId::Invalid();
		ConstantBinding bind = -1;
		if (type != SamplerVariableType && type != TextureVariableType && type != ImageReadWriteVariableType && type != BufferReadWriteVariableType)
		{
			Util::StringAtom block = ShaderGetConstantBlockName(shd, sem->string_value);
			IndexT bufferIndex = constantBuffers.FindIndex(block);
			if (bufferIndex == InvalidIndex)
			{
				cbo = ShaderCreateConstantBuffer(shd, block);
				constantBuffers.Add(block, cbo);
			}
			else
			{
				cbo = constantBuffers.ValueAtIndex(bufferIndex);
			}
			bind = ShaderGetConstantBinding(shd, sem->string_value);
			IndexT slot = ShaderGetResourceSlot(shd, block);
			ResourceTableSetConstantBuffer(table, { cbo, slot, 0, false, false, -1, 0 });
		}
		switch (type)
		{
		case IntVariableType:
			ConstantBufferUpdate(cbo, valStr.AsInt(), bind);
			break;
		case FloatVariableType:
			ConstantBufferUpdate(cbo, valStr.AsFloat(), bind);
			break;
		case VectorVariableType:
			ConstantBufferUpdate(cbo, valStr.AsVec4(), bind);
			break;
		case Vector2VariableType:
			ConstantBufferUpdate(cbo, valStr.AsVec2(), bind);
			break;
		case MatrixVariableType:
			ConstantBufferUpdate(cbo, valStr.AsMat4(), bind);
			break;
		case BoolVariableType:
			ConstantBufferUpdate(cbo, valStr.AsBool(), bind);
			break;
		case SamplerHandleType:
		case ImageHandleType:
		case TextureHandleType:
		{
			CoreGraphics::TextureId rtid = script->GetTexture(valStr);
			textures.Append(Util::MakeTuple(bind, cbo, rtid));
			if (rtid != CoreGraphics::TextureId::Invalid())
				ConstantBufferUpdate(cbo, CoreGraphics::TextureGetBindlessHandle(rtid), bind);
			else
				n_error("Unknown resource %s!", valStr.AsCharPtr());
			break;
		}
		case SamplerVariableType:
		case TextureVariableType:
		{
			IndexT slot = ShaderGetResourceSlot(shd, sem->string_value);
			CoreGraphics::TextureId rtid = script->GetTexture(valStr);

			textures.Append(Util::MakeTuple(slot, ConstantBufferId::Invalid(), rtid));
			if (rtid != CoreGraphics::TextureId::Invalid())
				ResourceTableSetTexture(table, { rtid, slot, 0, CoreGraphics::SamplerId::Invalid(), false });
			else
				n_error("Unknown resource %s!", valStr.AsCharPtr());
			break;
		}
		case ImageReadWriteVariableType:
		{
			IndexT slot = ShaderGetResourceSlot(shd, sem->string_value);
			CoreGraphics::TextureId rtid = script->GetTexture(valStr);

			textures.Append(Util::MakeTuple(slot, ConstantBufferId::Invalid(), rtid));
			if (rtid != CoreGraphics::TextureId::Invalid())
				ResourceTableSetRWTexture(table, { rtid, slot, 0, CoreGraphics::SamplerId::Invalid() });
			else
				n_error("Unknown resource %s!", valStr.AsCharPtr());
			break;
		}
		case BufferReadWriteVariableType:
		{
			IndexT slot = ShaderGetResourceSlot(shd, sem->string_value);
			CoreGraphics::BufferId rtid = script->GetBuffer(valStr);
			if (rtid != CoreGraphics::BufferId::Invalid())
				ResourceTableSetRWBuffer(table, { rtid, slot, 0 });
			else
				n_error("Unknown resource %s!", valStr.AsCharPtr());
			break;
		}
		}
	}

}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseResourceDependencies(const Ptr<Frame::FrameScript>& script, Frame::FrameOp* op, JzonValue* node)
{
	n_assert(node->is_array);
	for (uint i = 0; i < node->size; i++)
	{
		JzonValue* dep = node->array_values[i];
		const Util::String valstr = jzon_get(dep, "name")->string_value;
		CoreGraphics::BarrierAccess access = BarrierAccessFromString(jzon_get(dep, "access")->string_value);
		CoreGraphics::BarrierStage dependency = BarrierStageFromString(jzon_get(dep, "stage")->string_value);

		if (script->texturesByName.Contains(valstr))
		{
			CoreGraphics::ImageLayout layout = ImageLayoutFromString(jzon_get(dep, "layout")->string_value);
			CoreGraphics::ImageSubresourceInfo subres;
			JzonValue* nd = nullptr;

			TextureId tex = script->texturesByName[valstr];
			if ((nd = jzon_get(dep, "aspect")) != nullptr) subres.aspect = ImageAspectFromString(nd->string_value);
			else
			{
				bool isDepth = PixelFormat::IsDepthFormat(CoreGraphics::TextureGetPixelFormat(tex));
				subres.aspect = isDepth ? (ImageAspect::DepthBits | ImageAspect::StencilBits) : ImageAspect::ColorBits;
			}
			if ((nd = jzon_get(dep, "mip")) != nullptr) subres.mip = nd->int_value;
			if ((nd = jzon_get(dep, "mip_count")) != nullptr) subres.mipCount = nd->int_value;
			if ((nd = jzon_get(dep, "layer")) != nullptr) subres.layer = nd->int_value;
			if ((nd = jzon_get(dep, "layer_count")) != nullptr) subres.layerCount = nd->int_value;

			op->textureDeps.Add(tex, Util::MakeTuple(valstr, access, dependency, subres, layout));
		}
		else if (script->buffersByName.Contains(valstr))
		{
			BufferId buf = script->buffersByName[valstr];
			CoreGraphics::BufferSubresourceInfo subres;
			JzonValue* nd = nullptr;
			if ((nd = jzon_get(dep, "offset")) != nullptr) subres.offset = nd->int_value;
			if ((nd = jzon_get(dep, "size")) != nullptr) subres.size = nd->int_value;
			op->rwBufferDeps.Add(buf, Util::MakeTuple(valstr, access, dependency, subres));
		}
		else
		{
			n_error("No resource named %s declared\n", valstr.AsCharPtr());
		}
	}
}

} // namespace Frame2
