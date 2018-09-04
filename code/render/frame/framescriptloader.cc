//------------------------------------------------------------------------------
// framescriptloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "framescriptloader.h"
#include "io/ioserver.h"
#include "io/stream.h"
#include "framepass.h"
#include "coregraphics/rendertexture.h"
#include "frameglobalstate.h"
#include "frameblit.h"
#include "framecompute.h"
#include "framecomputealgorithm.h"
#include "framesubpass.h"
#include "frameevent.h"
#include "framesubpassalgorithm.h"
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
#include "frameswapbuffers.h"
#include "framesubpassplugins.h"
#include "framebarrier.h"
#include "coregraphics/barrier.h"
#include "algorithm/algorithms.h"
#include "coregraphics/displaydevice.h"
#include "memory/chunkallocator.h"
#include <mutex>

using namespace CoreGraphics;
using namespace IO;
namespace Frame
{


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
		n_assert(Util::String(node->string_value) == "NebulaTrifid");


#define CONSTRUCTOR_MACRO(type) \
		constructors.Add(("Algorithms::" #type ##_str).HashCode(), [](Memory::ChunkAllocator<0xFFFF>& alloc) -> Algorithms::Algorithm* { \
			void* mem = alloc.Alloc(sizeof(Algorithms::type));\
			n_new_inplace(Algorithms::type, mem);\
			return (Algorithms::Algorithm*)mem;\
		});

		constructors.Clear();
		CONSTRUCTOR_MACRO(HBAOAlgorithm);
		CONSTRUCTOR_MACRO(BloomAlgorithm);
		CONSTRUCTOR_MACRO(TonemapAlgorithm);

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
		if (name == "renderTextures")			ParseColorTextureList(script, cur);
		else if (name == "depthStencils")		ParseDepthStencilTextureList(script, cur);
		else if (name == "readWriteTextures")	ParseImageReadWriteTextureList(script, cur);
		else if (name == "readWriteBuffers")	ParseImageReadWriteBufferList(script, cur);
		else if (name == "algorithms")			ParseAlgorithmList(script, cur);
		else if (name == "globalState")			ParseGlobalState(script, cur);
		else if (name == "blit")				ParseBlit(script, cur);
		else if (name == "copy")				ParseCopy(script, cur);
		else if (name == "compute")				ParseCompute(script, cur);
		else if (name == "computeAlgorithm")	ParseComputeAlgorithm(script, cur);
		else if (name == "swapbuffers")			ParseSwapbuffers(script, cur);
		else if (name == "pass")				ParsePass(script, cur);
		
		else
		{
			n_error("Frame script operation '%s' is unrecognized.\n", name.AsCharPtr());
		}
	}
}


//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseColorTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
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
			CoreGraphics::RenderTextureId tex = FrameServer::Instance()->GetWindowTexture();
			script->AddColorTexture("__WINDOW__", tex);

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

			// get format
			CoreGraphics::PixelFormat::Code fmt = CoreGraphics::PixelFormat::FromString(format->string_value);

			// create texture
			RenderTextureCreateInfo info =
			{
				name->string_value,
				Texture2D,
				fmt,
				ColorAttachment
			};

			JzonValue* layers = jzon_get(cur, "layers");
			if (layers != nullptr) info.layers = layers->int_value;

			// set relative, dynamic or msaa if defined
			if (jzon_get(cur, "relative"))	info.relativeSize = jzon_get(cur, "relative")->bool_value;
			if (jzon_get(cur, "msaa"))		info.msaa = jzon_get(cur, "msaa")->bool_value;

			// if cube, use 6 layers
			float depth = 1;
			if (jzon_get(cur, "cube"))
			{
				bool isCube = jzon_get(cur, "cube")->bool_value;
				if (isCube)
				{
					info.type = TextureCube;
					depth = 6;
				}
			}

			// set dimension after figuring out if the texture is a cube
			info.width = (float)width->float_value;
			info.height = (float)height->float_value;
			info.depth = depth;

			// add to script
			RenderTextureId tex = CreateRenderTexture(info);
			script->AddColorTexture(name->string_value, tex);
		}		
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseDepthStencilTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != nullptr);
		JzonValue* format = jzon_get(cur, "format");
		n_assert(name != nullptr);
		JzonValue* width = jzon_get(cur, "width");
		n_assert(width != nullptr);
		JzonValue* height = jzon_get(cur, "height");
		n_assert(height != nullptr);

		// get format
		CoreGraphics::PixelFormat::Code fmt = CoreGraphics::PixelFormat::FromString(format->string_value);

		// create texture
		RenderTextureCreateInfo info =
		{
			name->string_value,
			Texture2D,
			fmt,
			DepthStencilAttachment
		};

		JzonValue* layers = jzon_get(cur, "layers");
		if (layers != nullptr) info.layers = layers->int_value;

		// set relative, dynamic or msaa if defined
		if (jzon_get(cur, "relative"))	info.relativeSize = jzon_get(cur, "relative")->bool_value;
		if (jzon_get(cur, "msaa"))		info.msaa = jzon_get(cur, "msaa")->bool_value;

		// if cube, use 6 layers
		float depth = 1;
		if (jzon_get(cur, "cube"))
		{
			bool isCube = jzon_get(cur, "cube")->bool_value;
			if (isCube)
			{
				info.type = TextureCube;
				depth = 6;
			}
		}

		// set dimension after figuring out if the texture is a cube
		info.width = (float)width->float_value;
		info.height = (float)height->float_value;
		info.depth = depth;

		// add to script
		RenderTextureId tex = CreateRenderTexture(info);
		script->AddDepthStencilTexture(name->string_value, tex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseImageReadWriteTextureList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != nullptr);
		JzonValue* format = jzon_get(cur, "format");
		n_assert(name != nullptr);
		JzonValue* width = jzon_get(cur, "width");
		n_assert(width != nullptr);
		JzonValue* height = jzon_get(cur, "height");
		n_assert(height != nullptr);

		// setup shader read-write texture
		ShaderRWTextureCreateInfo info =
		{
			name->string_value,
			Texture2D,	// fixme, not only 2D textures!
			CoreGraphics::PixelFormat::FromString(format->string_value),
			(float)width->float_value, (float)height->float_value, 1,
			1
		};

		if (jzon_get(cur, "relative")) info.relativeSize = jzon_get(cur, "relative")->bool_value;
		ShaderRWTextureId tex = CreateShaderRWTexture(info);

		// add texture
		script->AddReadWriteTexture(name->string_value, tex);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseImageReadWriteBufferList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
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
			size->int_value, 1
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
FrameScriptLoader::ParseAlgorithmList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];

		// algorithm needs name and class
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != NULL);
		JzonValue* clazz = jzon_get(cur, "class");
		n_assert(clazz != NULL);

		// create algorithm
		Algorithms::Algorithm* alg = (constructors[Util::String(clazz->string_value).HashCode()](script->GetAllocator()));

		JzonValue* textures = jzon_get(cur, "renderTextures");
		if (textures != NULL)
		{
			uint j;
			for (j = 0; j < textures->size; j++)
			{
				JzonValue* nd = textures->array_values[j];
				alg->AddRenderTexture(script->GetColorTexture(nd->string_value));
			}
		}

		JzonValue* buffers = jzon_get(cur, "readWriteBuffers");
		if (buffers != NULL)
		{
			uint j;
			for (j = 0; j < buffers->size; j++)
			{
				JzonValue* nd = buffers->array_values[j];
				alg->AddReadWriteBuffer(script->GetReadWriteBuffer(nd->string_value));
			}
		}

		JzonValue* images = jzon_get(cur, "readWriteTextures");
		if (images != NULL)
		{
			uint j;
			for (j = 0; j < images->size; j++)
			{
				JzonValue* nd = images->array_values[j];
				alg->AddReadWriteImage(script->GetReadWriteTexture(nd->string_value));
			}
		}

		// add algorithm to script
		alg->Setup();
		script->AddAlgorithm(name->string_value, alg);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseGlobalState(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	FrameGlobalState* op = script->GetAllocator().Alloc<FrameGlobalState>();

	// set name of op
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// create shared state, this will be set while running the script and update the shared state
	CoreGraphics::ShaderStateId state = ShaderServer::Instance()->ShaderCreateSharedState("shd:shared", { NEBULAT_FRAME_GROUP });
	op->state = state;

	// setup variables
	JzonValue* variables = jzon_get(node, "variables");
	if (variables != NULL)
	{
		uint i;
		for (i = 0; i < variables->size; i++)
		{
			JzonValue* var = variables->array_values[i];

			// variables need to define both semantic and value
			JzonValue* sem = jzon_get(var, "semantic");
			n_assert(sem != NULL);
			JzonValue* val = jzon_get(var, "value");
			n_assert(val != NULL);
			Util::String valStr(val->string_value);

			// get variable
			ShaderConstantId varid = ShaderStateGetConstant(state, sem->string_value);
			ShaderConstantType type = ShaderConstantGetType(varid, state);
			switch (type)
			{
			case IntVariableType:
				ShaderConstantSet(varid, state, valStr.AsInt());
				break;
			case FloatVariableType:
				ShaderConstantSet(varid, state, valStr.AsFloat());
				break;
			case VectorVariableType:
				ShaderConstantSet(varid, state, valStr.AsFloat4());
				break;
			case Vector2VariableType:
				ShaderConstantSet(varid, state, valStr.AsFloat2());
				break;
			case MatrixVariableType:
				ShaderConstantSet(varid, state, valStr.AsMatrix44());
				break;
			case BoolVariableType:
				ShaderConstantSet(varid, state, valStr.AsBool());
				break;
			case SamplerVariableType:
			case TextureVariableType:
			{
				const Ptr<Resources::ResourceManager>& resManager = Resources::ResourceManager::Instance();
				Resources::ResourceId id = resManager->GetId(valStr);
				if (id != Resources::ResourceId::Invalid())
				{
					TextureId tex = id;
					ShaderResourceSetTexture(varid, state, tex);
				}
				break;
			}
			case ImageReadWriteVariableType:
				ShaderResourceSetReadWriteTexture(varid, state, script->GetReadWriteTexture(valStr));
				break;
			case BufferReadWriteVariableType:
				ShaderResourceSetReadWriteBuffer(varid, state, script->GetReadWriteBuffer(valStr));
				break;
			}

			// add variable to op
			op->constants.Append(varid);
		}
	}

	script->AddOp(op);
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

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
	}

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != NULL);
	const CoreGraphics::RenderTextureId& fromTex = script->GetColorTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != NULL);
	const CoreGraphics::RenderTextureId& toTex = script->GetColorTexture(to->string_value);

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

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
	}

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != NULL);
	const CoreGraphics::RenderTextureId& fromTex = script->GetColorTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != NULL);
	const CoreGraphics::RenderTextureId& toTex = script->GetColorTexture(to->string_value);

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

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
	}

	// create shader state
	JzonValue* shader = jzon_get(node, "shaderState");
	n_assert(shader != NULL);

	CoreGraphics::ShaderStateId state;
	ParseShaderState(script, shader, state);
	op->state = state;

	JzonValue* variation = jzon_get(node, "variation");
	n_assert(variation != NULL);
	ShaderId fakeShd;
	fakeShd.allocId = state.shaderId;
	fakeShd.allocType = state.shaderType;
	op->program = ShaderGetProgram(fakeShd, ShaderServer::Instance()->FeatureStringToMask(variation->string_value));

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
FrameScriptLoader::ParseComputeAlgorithm(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	FrameComputeAlgorithm* op = script->GetAllocator().Alloc<FrameComputeAlgorithm>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* queue = jzon_get(node, "queue");
	if (queue == nullptr)
		op->queue = CoreGraphicsQueueType::GraphicsQueueType;
	else
		op->queue = CoreGraphicsQueueTypeFromString(queue->string_value);

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
	}

	JzonValue* alg = jzon_get(node, "algorithm");
	n_assert(alg != NULL);
	JzonValue* function = jzon_get(node, "function");
	n_assert(function != NULL);

	// get algorithm
	Algorithms::Algorithm* algorithm = script->GetAlgorithm(alg->string_value);
	op->alg = algorithm;
	op->funcName = function->string_value;

	// add to script
	script->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSwapbuffers(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	FrameSwapbuffers* op = script->GetAllocator().Alloc<FrameSwapbuffers>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* texture = jzon_get(node, "texture");
	n_assert(texture != NULL);
	const CoreGraphics::RenderTextureId& tex = script->GetColorTexture(texture->string_value);
	op->tex = tex;

	// add operation
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
		else if (name == "depthStencil")
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

			JzonValue* cs = jzon_get(cur, "clearStencil");
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

			JzonValue* ls = jzon_get(cur, "loadStencil");
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

			JzonValue* ss = jzon_get(cur, "storeStencil");
			if (ss != NULL && ss->bool_value)
			{
				depthStencilClearFlags |= StoreStencil;
			}

			info.depthStencilFlags = (AttachmentFlagBits)depthStencilClearFlags;
			info.clearStencil = clearStencil;
			info.clearDepth = clearDepth;

			// set attachment in framebuffer
			JzonValue* ds = jzon_get(cur, "name");
			
			info.depthStencilAttachment = script->GetDepthStencilTexture(ds->string_value);
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
		pass.colorAttachments.Append(script->GetColorTexture(name->string_value));
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
		else if (name == "viewports")			ParseSubpassViewports(script, frameSubpass, cur);
		else if (name == "scissors")			ParseSubpassScissors(script, frameSubpass, cur);
		else if (name == "subpassAlgorithm")	ParseSubpassAlgorithm(script, frameSubpass, cur);
		else if (name == "batch")				ParseSubpassBatch(script, frameSubpass, cur);
		else if (name == "sortedBatch")			ParseSubpassSortedBatch(script, frameSubpass, cur);
		else if (name == "fullscreenEffect")	ParseSubpassFullscreenEffect(script, frameSubpass, cur);
		else if (name == "system")				ParseSubpassSystem(script, frameSubpass, cur);
		else if (name == "plugins")				ParseSubpassPlugins(script, frameSubpass, cur);
		else
		{
			n_error("Subpass operation '%s' is invalid.\n", name.AsCharPtr());
		}
	}

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
FrameScriptLoader::ParseSubpassAlgorithm(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{
	FrameSubpassAlgorithm* op = script->GetAllocator().Alloc<FrameSubpassAlgorithm>();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphicsQueueType::GraphicsQueueType;

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
	}

	JzonValue* alg = jzon_get(node, "algorithm");
	n_assert(alg != NULL);
	JzonValue* function = jzon_get(node, "function");
	n_assert(function != NULL);

	// get algorithm
	Algorithms::Algorithm* algorithm = script->GetAlgorithm(alg->string_value);
	op->alg = algorithm;
	op->funcName = function->string_value;

	// add to script
	op->Setup();
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

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
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

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
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

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
	}

	// create shader state
	JzonValue* shaderState = jzon_get(node, "shaderState");
	n_assert(shaderState != NULL);

	CoreGraphics::ShaderStateId state;
	ParseShaderState(script, shaderState, state);
	op->shaderState = state;

	// get texture
	JzonValue* texture = jzon_get(node, "sizeFromTexture");
	n_assert(texture != NULL);
	op->tex = script->GetColorTexture(texture->string_value);
	
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

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
	}

	Util::String subsystem(node->string_value);
	if (subsystem == "Lights")					op->SetSubsystem(FrameSubpassSystem::Lights);
	else if (subsystem == "LightProbes")		op->SetSubsystem(FrameSubpassSystem::LightProbes);
	else if (subsystem == "LocalShadowsSpot")	op->SetSubsystem(FrameSubpassSystem::LocalShadowsSpot);
	else if (subsystem == "LocalShadowsPoint")	op->SetSubsystem(FrameSubpassSystem::LocalShadowsPoint);
	else if (subsystem == "GlobalShadows")		op->SetSubsystem(FrameSubpassSystem::GlobalShadows);
	else if (subsystem == "UI")					op->SetSubsystem(FrameSubpassSystem::UI);
	else if (subsystem == "Text")				op->SetSubsystem(FrameSubpassSystem::Text);
	else if (subsystem == "Shapes")				op->SetSubsystem(FrameSubpassSystem::Shapes);
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
FrameScriptLoader::ParseSubpassPlugins(const Ptr<Frame::FrameScript>& script, Frame::FrameSubpass* subpass, JzonValue* node)
{	
	FrameSubpassPlugins* op = script->GetAllocator().Alloc<FrameSubpassPlugins>();

	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// assume all of these are graphics
	op->domain = BarrierDomain::Pass;
	op->queue = CoreGraphicsQueueType::GraphicsQueueType;

	JzonValue* resources = jzon_get(node, "resources");
	if (resources != nullptr)
	{
		ParseResourceDependencies(script, op, resources);
	}

	JzonValue* filter = jzon_get(node, "filter");
	n_assert(filter != NULL);
	op->SetPluginFilter(filter->string_value);
	op->Setup();
	subpass->AddOp(op);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderState(const Ptr<Frame::FrameScript>& script, JzonValue* node, CoreGraphics::ShaderStateId& state)
{
	bool createResources = false;
	JzonValue* create = jzon_get(node, "createResourceSet");
	if (create != NULL) createResources = create->bool_value;

	JzonValue* shader = jzon_get(node, "shader");
	n_assert(shader != NULL);
	state = ShaderServer::Instance()->ShaderCreateState(shader->string_value, { NEBULAT_BATCH_GROUP }, createResources);

	JzonValue* vars = jzon_get(node, "variables");
	if (vars != NULL) ParseShaderVariables(script, state, vars);
	CoreGraphics::ShaderStateCommit(state);
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderVariables(const Ptr<Frame::FrameScript>& script, const CoreGraphics::ShaderStateId& state, JzonValue* node)
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
		ShaderConstantId varid = ShaderStateGetConstant(state, sem->string_value);
		ShaderConstantType type = ShaderConstantGetType(varid, state);
		switch (type)
		{
		case IntVariableType:
			ShaderConstantSet(varid, state, valStr.AsInt());
			break;
		case FloatVariableType:
			ShaderConstantSet(varid, state, valStr.AsFloat());
			break;
		case VectorVariableType:
			ShaderConstantSet(varid, state, valStr.AsFloat4());
			break;
		case Vector2VariableType:
			ShaderConstantSet(varid, state, valStr.AsFloat2());
			break;
		case MatrixVariableType:
			ShaderConstantSet(varid, state, valStr.AsMatrix44());
			break;
		case BoolVariableType:
			ShaderConstantSet(varid, state, valStr.AsBool());
			break;
		case SamplerVariableType:
		case TextureVariableType:
		{
			if (script->GetColorTexture(valStr) != CoreGraphics::RenderTextureId::Invalid())
			{
				ShaderResourceSetRenderTexture(varid, state, script->GetColorTexture(valStr));
			}
			else if (script->GetReadWriteTexture(valStr) != CoreGraphics::ShaderRWTextureId::Invalid())
			{
				ShaderResourceSetReadWriteTexture(varid, state, script->GetReadWriteTexture(valStr));
			}
			else
			{
				const Ptr<Resources::ResourceManager>& resManager = Resources::ResourceManager::Instance();
				Resources::ResourceId id = resManager->CreateResource(valStr, nullptr, nullptr, true);
				if (id != Resources::ResourceId::Invalid())
				{
					TextureId tex = id;
					ShaderResourceSetTexture(varid, state, tex);
				}
			}
			break;
		}
		case ImageReadWriteVariableType:
			ShaderResourceSetReadWriteTexture(varid, state, script->GetReadWriteTexture(valStr));
			break;
		case BufferReadWriteVariableType:
			ShaderResourceSetReadWriteBuffer(varid, state, script->GetReadWriteBuffer(valStr));
			break;
		}
	}

}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseResourceDependencies(const Ptr<Frame::FrameScript>& script, Frame::FrameOp* op, JzonValue* node)
{
	for (uint i = 0; i < node->size; i++)
	{
		JzonValue* dep = node->array_values[i];
		const Util::String valstr = jzon_get(dep, "name")->string_value;
		CoreGraphics::BarrierAccess access = BarrierAccessFromString(jzon_get(dep, "access")->string_value);
		CoreGraphics::BarrierStage dependency = BarrierDependencyFromString(jzon_get(dep, "stage")->string_value);
		
		if (script->readWriteTexturesByName.Contains(valstr))
		{
			ImageLayout layout = ImageLayoutFromString(jzon_get(dep, "layout")->string_value);
			CoreGraphics::ImageSubresourceInfo subres;
			JzonValue* nd = nullptr;
			if ((nd = jzon_get(dep, "aspect")) != nullptr) subres.aspect			= ImageAspectFromString(nd->string_value);
			if ((nd = jzon_get(dep, "mip")) != nullptr) subres.mip					= nd->int_value;
			if ((nd = jzon_get(dep, "mipCount")) != nullptr) subres.mipCount		= nd->int_value;
			if ((nd = jzon_get(dep, "layer")) != nullptr) subres.layer				= nd->int_value;
			if ((nd = jzon_get(dep, "layerCount")) != nullptr) subres.layerCount	= nd->int_value;
			
			ShaderRWTextureId tex = script->readWriteTexturesByName[valstr];
			op->rwTextureDeps.Add(tex, std::make_tuple(access, dependency, subres, layout));
		}
		else if (script->readWriteBuffersByName.Contains(valstr))
		{
			ShaderRWBufferId buf = script->readWriteBuffersByName[valstr];
			op->rwBufferDeps.Add(buf, std::make_tuple(access, dependency));
		}
		else if (script->colorTexturesByName.Contains(valstr))
		{
			ImageLayout layout = ImageLayoutFromString(jzon_get(dep, "layout")->string_value);
			CoreGraphics::ImageSubresourceInfo subres;
			JzonValue* nd = nullptr;
			if ((nd = jzon_get(dep, "aspect")) != nullptr) subres.aspect = ImageAspectFromString(nd->string_value);
			if ((nd = jzon_get(dep, "mip")) != nullptr) subres.mip = nd->int_value;
			if ((nd = jzon_get(dep, "mipCount")) != nullptr) subres.mipCount = nd->int_value;
			if ((nd = jzon_get(dep, "layer")) != nullptr) subres.layer = nd->int_value;
			if ((nd = jzon_get(dep, "layerCount")) != nullptr) subres.layerCount = nd->int_value;

			RenderTextureId tex = script->colorTexturesByName[valstr];
			op->renderTextureDeps.Add(tex, std::make_tuple(access, dependency, subres, layout));
		}
		else
		{
			n_error("No resource named %s declared\n", valstr.AsCharPtr());
		}
	}
}

} // namespace Frame2