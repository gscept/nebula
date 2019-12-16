//------------------------------------------------------------------------------
// framescriptloader.cc
// (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framescriptloader.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "framepass.h"
#include "frameglobalstate.h"
#include "frameblit.h"
#include "framecompute.h"
#include "framepluginop.h"
#include "framesubpass.h"
#include "frameevent.h"
#include "framesubpassplugin.h"
#include "framesubpassbatch.h"
#include "framesubpassorderedbatch.h"
#include "framesubpassfullscreeneffect.h"
#include "framecopy.h"
#include "framesubpasssystem.h"
#include "frameserver.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/config.h"
#include "resources/resourcemanager.h"
#include "core/factory.h"
#include "framesubpassplugin.h"
#include "framebarrier.h"
#include "coregraphics/barrier.h"
#include "frame/plugins/frameplugins.h"
#include "coregraphics/displaydevice.h"
#include "memory/arenaallocator.h"
#include <mutex>

using namespace CoreGraphics;
using namespace IO;
namespace Frame
{

Frame::FrameSubmission* FrameScriptLoader::LastSubmission[CoreGraphicsQueryType::NumCoreGraphicsQueryTypes] = {nullptr, nullptr};
Util::HashTable<uint, FrameScriptLoader::Fn> FrameScriptLoader::constructors;
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


#define CONSTRUCTOR_MACRO(type) \
		constructors.Add(("Frame::" #type ##_str).HashCode(), [](Memory::ArenaAllocator<BIG_CHUNK>& alloc) -> Frame::FramePlugin* { \
			void* mem = alloc.Alloc(sizeof(Frame::type));\
			n_new_inplace(Frame::type, mem);\
			return (Frame::FramePlugin*)mem;\
		});

		constructors.Clear();
		CONSTRUCTOR_MACRO(SSAOPlugin);
		CONSTRUCTOR_MACRO(BloomPlugin);
		CONSTRUCTOR_MACRO(TonemapPlugin);

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
		if (name == "textures")					ParseTextureList(script, cur);
		else if (name == "read_write_buffers")	ParseReadWriteBufferList(script, cur);
		else if (name == "plugins")				ParsePluginList(script, cur);
		else if (name == "blit")				ParseBlit(script, cur);
		else if (name == "copy")				ParseCopy(script, cur);
		else if (name == "compute")				ParseCompute(script, cur);
		else if (name == "plugin")				ParsePlugin(script, cur);
		else if (name == "pass")				ParsePass(script, cur);
		else if (name == "begin_submission")	ParseFrameSubmission(script, 0, cur); // 0 means begin
		else if (name == "end_submission")		ParseFrameSubmission(script, 1, cur); // 1 means end
		else if (name == "barrier")				ParseBarrier(script, cur);
		
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
				info.mips = mips->int_value;

			if (JzonValue* depth = jzon_get(cur, "depth"))
				info.depth = (float)depth->float_value;

			// set relative, dynamic or msaa if defined
			if (jzon_get(cur, "relative"))	info.windowRelative = jzon_get(cur, "relative")->bool_value;
			if (jzon_get(cur, "samples"))	info.samples = jzon_get(cur, "samples")->int_value;

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
		n_assert(name != NULL);
		JzonValue* size = jzon_get(cur, "size");
		n_assert(size != NULL);

		// create shader buffer 
		ShaderRWBufferCreateInfo info =
		{
			name->string_value, size->int_value, 1
		};

		bool relativeSize = false;
		if (jzon_get(cur, "relative")) info.screenRelative = jzon_get(cur, "relative")->bool_value;

		// add to script
		ShaderRWBufferId buf = CreateShaderRWBuffer(info);
		script->AddReadWriteBuffer(name->string_value, buf);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParsePluginList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];

		// algorithm needs name and class
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != nullptr);
		JzonValue* clazz = jzon_get(cur, "class");
		n_assert(clazz != nullptr);

		// create algorithm
		Frame::FramePlugin* alg = (constructors[Util::String(clazz->string_value).HashCode()](script->GetAllocator()));

		JzonValue* textures = jzon_get(cur, "textures");
		if (textures != nullptr)
		{
			uint j;
			for (j = 0; j < textures->size; j++)
			{
				JzonValue* nd = textures->array_values[j];
				alg->AddTexture(nd->string_value, script->GetTexture(nd->string_value));
			}
		}

		JzonValue* buffers = jzon_get(cur, "read_write_buffers");
		if (buffers != nullptr)
		{
			uint j;
			for (j = 0; j < buffers->size; j++)
			{
				JzonValue* nd = buffers->array_values[j];
				alg->AddReadWriteBuffer(nd->string_value, script->GetReadWriteBuffer(nd->string_value));
			}
		}

		// add algorithm to script
		alg->Setup();
		script->AddPlugin(name->string_value, alg);
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
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphicsQueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphicsQueueTypeFromString(queue->string_value);

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
	}
	
	JzonValue* from = jzon_get(node, "from");
	n_assert(from != NULL);
	const CoreGraphics::TextureId& fromTex = script->GetTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != NULL);
	const CoreGraphics::TextureId& toTex = script->GetTexture(to->string_value);

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
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphicsQueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphicsQueueTypeFromString(queue->string_value);

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
	}

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != NULL);
	const CoreGraphics::TextureId& fromTex = script->GetTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != NULL);
	const CoreGraphics::TextureId& toTex = script->GetTexture(to->string_value);

	// setup copy operation
	op->from = fromTex;
	op->to = toTex;
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
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphicsQueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphicsQueueTypeFromString(queue->string_value);

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
	}

	// create shader state
	JzonValue* shader = jzon_get(node, "shader_state");
	n_assert(shader != NULL);
	ParseShaderState(script, shader, op->shader, op->resourceTable, op->constantBuffers);

	JzonValue* variation = jzon_get(node, "variation");
	n_assert(variation != NULL);
	op->program = ShaderGetProgram(op->shader, ShaderServer::Instance()->FeatureStringToMask(variation->string_value));

	// dimensions, must be 3
	JzonValue* dims = jzon_get(node, "dimensions");
	n_assert(dims != NULL);
	n_assert(dims->size == 3);
	op->x = dims->array_values[0]->int_value;
	op->y = dims->array_values[1]->int_value;
	op->z = dims->array_values[2]->int_value;

	// add op to script
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParsePlugin(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	FramePluginOp* op = script->GetAllocator().Alloc<FramePluginOp>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphicsQueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphicsQueueTypeFromString(queue->string_value);

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
	}

	// get algorithm
	op->func = Frame::FramePlugin::GetCallback(name->string_value);

	// add to script
	script->AddOp(op);
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
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
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
		n_assert(name != NULL);
		op->SetName(name->string_value);
	}
	else
	{
		op->SetName("End");
	}

	JzonValue* queue = jzon_get(node, "queue");
	op->queue = CoreGraphicsQueueTypeFromString(queue->string_value);

	// if we are starting a new submission, also look for the queue wait flag
	if (startOrEnd == 0)
	{
		JzonValue* waitQueue = jzon_get(node, "wait_for_queue");
		if (waitQueue)
			op->waitQueue = CoreGraphicsQueueTypeFromString(waitQueue->string_value);
	}

	// insert a block queue into the last submission for the opposite queue, if this queue needs to wait
	if (op->waitQueue != InvalidQueueType)
		FrameScriptLoader::LastSubmission[op->queue == GraphicsQueueType ? ComputeQueueType : GraphicsQueueType]->blockQueue = op->queue;

	// update last submission
	if (startOrEnd == 1)
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
	n_assert(name != NULL);
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
			uint depthStencilClearFlags = 0;
			JzonValue* cd = jzon_get(cur, "clear");
			if (cd != NULL)
			{
				depthStencilClearFlags |= Clear;
				info.clearDepth = (float)cd->float_value;
			}

			JzonValue* cs = jzon_get(cur, "clear_stencil");
			if (cs != NULL)
			{
				depthStencilClearFlags |= ClearStencil;
				info.clearStencil = cs->int_value;
			}

			JzonValue* ld = jzon_get(cur, "load");
			if (ld != NULL && ld->bool_value)
			{
				n_assert2(cd == NULL, "Can't load depth from previous pass AND clear.");				
				depthStencilClearFlags |= Load;
			}

			JzonValue* ls = jzon_get(cur, "load_stencil");
			if (ls != NULL && ls->bool_value)
			{
				// can't really load and store
				n_assert2(cs == NULL, "Can't load stenil from previous pass AND clear.");
				depthStencilClearFlags |= LoadStencil;
			}

			JzonValue* sd = jzon_get(cur, "store");
			if (sd != NULL && sd->bool_value)
			{
				depthStencilClearFlags |= Store;
			}

			JzonValue* ss = jzon_get(cur, "store_stencil");
			if (ss != NULL && ss->bool_value)
			{
				depthStencilClearFlags |= StoreStencil;
			}

			info.depthStencilFlags = (AttachmentFlagBits)depthStencilClearFlags;
			info.clearStencil = clearStencil;
			info.clearDepth = clearDepth;

			// set attachment in framebuffer
			JzonValue* ds = jzon_get(cur, "name");
			
			info.depthStencilAttachment = script->GetTexture(ds->string_value);
		}
		else if (name == "subpass")				ParseSubpass(script, info, op, attachmentNames, cur);
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
		n_assert(name != NULL);
		pass.colorAttachments.Append(script->GetTexture(name->string_value));
		attachmentNames.Append(name->string_value);

		// set clear flag if present
		JzonValue* clear = jzon_get(cur, "clear");
		uint flags = 0;
		if (clear != NULL)
		{
			Math::float4 clearValue;
			n_assert(clear->size <= 4);
			uint j;
			for (j = 0; j < clear->size; j++)
			{
				clearValue[j] = clear->array_values[j]->float_value;
			}
			pass.colorAttachmentClears.Append(clearValue);
			flags |= Clear;
		}
		else
			pass.colorAttachmentClears.Append(Math::float4(1)); // we set the clear to 1, but the flag is not to clear...

		// set if attachment should store at the end of the pass
		JzonValue* store = jzon_get(cur, "store");
		if (store && store->bool_value)
		{
			flags |= Store;
		}

		JzonValue* load = jzon_get(cur, "load");
		if (load && load->bool_value)
		{
			// we can't really load and clear
			n_assert2(clear == NULL, "Can't load color if it's being cleared.");
			flags |= Load;
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
		if (name == "name")						frameSubpass->SetName(cur->string_value);
		else if (name == "dependencies")		ParseSubpassDependencies(framePass, subpass, cur);
		else if (name == "attachments")			ParseSubpassAttachments(framePass, subpass, attachmentNames, cur);
		else if (name == "inputs")				ParseSubpassInputs(framePass, subpass, attachmentNames, cur);
		else if (name == "depth")				subpass.bindDepth = cur->bool_value;
		else if (name == "resolve")				subpass.resolve = cur->bool_value;
		else if (name == "resources")			ParseResourceDependencies(script, framePass, cur);
		else if (name == "inputs")				ParseResourceDependencies(script, framePass, cur);
		else if (name == "outputs")				ParseResourceDependencies(script, framePass, cur);
		else if (name == "viewports")			{ ParseSubpassViewports(script, frameSubpass, cur); subpass.numViewports = frameSubpass->viewports.Size(); }
		else if (name == "scissors")			{ ParseSubpassScissors(script, frameSubpass, cur); subpass.numScissors = frameSubpass->scissors.Size(); }
		else if (name == "plugin")				ParseSubpassPlugin(script, frameSubpass, cur);
		else if (name == "batch")				ParseSubpassBatch(script, frameSubpass, cur);
		else if (name == "sorted_batch")		ParseSubpassSortedBatch(script, frameSubpass, cur);
		else if (name == "fullscreen_effect")	ParseSubpassFullscreenEffect(script, frameSubpass, cur);
		else if (name == "system")				ParseSubpassSystem(script, frameSubpass, cur);
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
FrameScriptLoader::ParseSubpassViewports(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* var = node->array_values[i];
		n_assert(var->size == 4);
		Math::rectangle<int> rect(var->array_values[0]->int_value, var->array_values[1]->int_value, var->array_values[2]->int_value, var->array_values[3]->int_value);
		subpass->AddViewport(rect);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassScissors(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* var = node->array_values[i];
		n_assert(var->size == 4);
		Math::rectangle<int> rect(var->array_values[0]->int_value, var->array_values[1]->int_value, var->array_values[2]->int_value, var->array_values[3]->int_value);
		subpass->AddScissor(rect);
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
	n_assert(name != NULL);
	op->SetName(name->string_value);

	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphicsQueueType::GraphicsQueueType;

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
	}

	// get algorithm
	op->func = Frame::FramePlugin::GetCallback(name->string_value);

	// add to script
	subpass->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassBatch(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	FrameSubpassBatch* op = script->GetAllocator().Alloc<FrameSubpassBatch>();
	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphicsQueueType::GraphicsQueueType;

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
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
	op->queue = CoreGraphicsQueueType::GraphicsQueueType;

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
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
	n_assert(name != NULL);
	op->SetName(name->string_value);

	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphicsQueueType::GraphicsQueueType;

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
	}

	// create shader state
	JzonValue* shaderState = jzon_get(node, "shader_state");
	n_assert(shaderState != NULL);
	ParseShaderState(script, shaderState, op->shader, op->resourceTable, op->constantBuffers);

	// get texture
	JzonValue* texture = jzon_get(node, "size_from_texture");
	n_assert(texture != NULL);
	op->tex = script->GetTexture(texture->string_value);
	
	// add op to subpass
	op->Setup();
	subpass->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassSystem(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	FrameSubpassSystem* op = script->GetAllocator().Alloc<FrameSubpassSystem>();

	// assume all of these are graphics
	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphicsQueueType::GraphicsQueueType;

	JzonValue* inputs = jzon_get(node, "inputs");
	if (inputs != nullptr)
	{
		ParseResourceDependencies(script, op, inputs);
	}

	JzonValue* outputs = jzon_get(node, "outputs");
	if (outputs != nullptr)
	{
		ParseResourceDependencies(script, op, outputs);
	}

	Util::String subsystem(node->string_value);
	if (subsystem == "Lights")						op->SetSubsystem(FrameSubpassSystem::Lights);
	else if (subsystem == "LightClassification")	op->SetSubsystem(FrameSubpassSystem::LightClassification);
	else if (subsystem == "LightProbes")			op->SetSubsystem(FrameSubpassSystem::LightProbes);
	else if (subsystem == "LocalShadowsSpot")		op->SetSubsystem(FrameSubpassSystem::LocalShadowsSpot);
	else if (subsystem == "LocalShadowsPoint")		op->SetSubsystem(FrameSubpassSystem::LocalShadowsPoint);
	else if (subsystem == "GlobalShadows")			op->SetSubsystem(FrameSubpassSystem::GlobalShadows);
	else if (subsystem == "UI")						op->SetSubsystem(FrameSubpassSystem::UI);
	else if (subsystem == "Text")					op->SetSubsystem(FrameSubpassSystem::Text);
	else if (subsystem == "Shapes")					op->SetSubsystem(FrameSubpassSystem::Shapes);
	else
	{
		n_error("No subsystem called '%s' exists", subsystem.AsCharPtr());
	}
	subpass->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderState(const Ptr<Frame::FrameScript>& script, JzonValue* node, CoreGraphics::ShaderId& shd, CoreGraphics::ResourceTableId& table, Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& constantBuffers)
{
	bool createResources = false;
	JzonValue* create = jzon_get(node, "create_resource_set");
	if (create != NULL) createResources = create->bool_value;

	JzonValue* shader = jzon_get(node, "shader");
	n_assert(shader != NULL);
    Util::String shaderRes = "shd:" + Util::String(shader->string_value) + ".fxb";
	shd = ShaderGet(shaderRes);
	table = ShaderCreateResourceTable(shd, NEBULA_BATCH_GROUP);

	JzonValue* vars = jzon_get(node, "variables");
	if (vars != NULL) ParseShaderVariables(script, shd, table, constantBuffers, vars);
	CoreGraphics::ResourceTableCommitChanges(table);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderVariables(const Ptr<Frame::FrameScript>& script, const CoreGraphics::ShaderId& shd, CoreGraphics::ResourceTableId& table, Util::Dictionary<Util::StringAtom, CoreGraphics::ConstantBufferId>& constantBuffers, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* var = node->array_values[i];

		// variables need to define both semantic and value
		JzonValue* sem = jzon_get(var, "semantic");
		n_assert(sem != NULL);
		JzonValue* val = jzon_get(var, "value");
		n_assert(val != NULL);
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
			ConstantBufferUpdate(cbo, valStr.AsFloat4(), bind);
			break;
		case Vector2VariableType:
			ConstantBufferUpdate(cbo, valStr.AsFloat2(), bind);
			break;
		case MatrixVariableType:
			ConstantBufferUpdate(cbo, valStr.AsMatrix44(), bind);
			break;
		case BoolVariableType:
			ConstantBufferUpdate(cbo, valStr.AsBool(), bind);
			break;
		case SamplerHandleType:
		case ImageHandleType:
		case TextureHandleType:
		{
			CoreGraphics::TextureId rtid = script->GetTexture(valStr);
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
			if (rtid != CoreGraphics::TextureId::Invalid())
				ResourceTableSetTexture(table, { rtid, slot, 0, CoreGraphics::SamplerId::Invalid(), false });
			else
				n_error("Unknown resource %s!", valStr.AsCharPtr());
			break;
		}
		case ImageReadWriteVariableType:
		{
			IndexT slot = ShaderGetResourceSlot(shd, sem->string_value);
			ResourceTableSetRWTexture(table, { script->GetTexture(valStr), slot, 0, CoreGraphics::SamplerId::Invalid() });
			break;
		}
		case BufferReadWriteVariableType:
		{
			IndexT slot = ShaderGetResourceSlot(shd, sem->string_value);
			ResourceTableSetRWBuffer(table, { script->GetReadWriteBuffer(valStr), slot, 0 });
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
			CoreGraphicsImageLayout layout = ImageLayoutFromString(jzon_get(dep, "layout")->string_value);
			CoreGraphics::ImageSubresourceInfo subres;
			JzonValue* nd = nullptr;
			if ((nd = jzon_get(dep, "aspect")) != nullptr) subres.aspect = ImageAspectFromString(nd->string_value);
			if ((nd = jzon_get(dep, "mip")) != nullptr) subres.mip = nd->int_value;
			if ((nd = jzon_get(dep, "mip_count")) != nullptr) subres.mipCount = nd->int_value;
			if ((nd = jzon_get(dep, "layer")) != nullptr) subres.layer = nd->int_value;
			if ((nd = jzon_get(dep, "layer_count")) != nullptr) subres.layerCount = nd->int_value;

			TextureId tex = script->texturesByName[valstr];
			op->textureDeps.Add(tex, std::make_tuple(valstr, access, dependency, subres, layout));
		}
		else if (script->readWriteBuffersByName.Contains(valstr))
		{
			ShaderRWBufferId buf = script->readWriteBuffersByName[valstr];
			CoreGraphics::BufferSubresourceInfo subres;
			JzonValue* nd = nullptr;
			if ((nd = jzon_get(dep, "offset")) != nullptr) subres.offset = nd->int_value;
			if ((nd = jzon_get(dep, "size")) != nullptr) subres.size = nd->int_value;
			op->rwBufferDeps.Add(buf, std::make_tuple(valstr, access, dependency, subres));
		}
		else
		{
			n_error("No resource named %s declared\n", valstr.AsCharPtr());
		}
	}
}

} // namespace Frame2