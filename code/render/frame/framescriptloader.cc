//------------------------------------------------------------------------------
// framescriptloader.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
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

using namespace CoreGraphics;
using namespace IO;
namespace Frame
{

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

		// make sure last byte is 0, since jzon doesn't care about input size
		((char*)data)[stream->GetSize()] = '\0';
		JzonParseResult result = jzon_parse((const char*)data);
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

		// run parser entry point
		json = jzon_get(json, "framescript");
		ParseFrameScript(script, json);

		// clear jzon
		jzon_free(json);

		stream->Unmap();
		stream->Close();
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
		else if (name == "events")				ParseEventList(script, cur);
		else if (name == "shaderStates")		ParseShaderStateList(script, cur);
		else if (name == "globalState")			ParseGlobalState(script, cur);
		else if (name == "blit")				ParseBlit(script, cur);
		else if (name == "copy")				ParseCopy(script, cur);
		else if (name == "event")				ParseEvent(script, cur);
		else if (name == "compute")				ParseCompute(script, cur);
		else if (name == "computeAlgorithm")	ParseComputeAlgorithm(script, cur);
		else if (name == "swapbuffers")			ParseSwapbuffers(script, cur);
		else if (name == "pass")				ParsePass(script, cur);
		else if (name == "barrier")				ParseBarrier(script, cur);
		
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
			if (jzon_get(cur, "dynamic"))	info.dynamicSize = jzon_get(cur, "dynamic")->bool_value;
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
		if (jzon_get(cur, "dynamic"))	info.dynamicSize = jzon_get(cur, "dynamic")->bool_value;
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
			1, 1,
			(float)width->float_value, (float)height->float_value, 1,
		};

		if (jzon_get(cur, "relative")) info.relativeSize = jzon_get(cur, "relative")->bool_value;
		if (jzon_get(cur, "dynamic")) info.dynamicSize = jzon_get(cur, "dynamic")->bool_value;
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

		// get type size so we can preallocate the algorithm type
		const Core::Rtti* rtti = Core::Factory::Instance()->GetClassRtti(clazz->string_value);
		void* mem = script->GetAllocator().Alloc(rtti->GetInstanceSize());

		// create algorithm
		Ptr<Algorithms::Algorithm> alg = (Algorithms::Algorithm*)Core::Factory::Instance()->CreateInplace(clazz->string_value, mem);

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
FrameScriptLoader::ParseEventList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != NULL);

		// create event
		CoreGraphics::EventCreateInfo info;
		if (jzon_get(cur, "signaled")) info.createSignaled = jzon_get(cur, "signaled")->bool_value;

		// create barrier info, which is used to parse the barrier part of the event
		CoreGraphics::BarrierCreateInfo barrier;

		// call internal parser for barrier
		JzonValue* bar = jzon_get(cur, "barrier");
		n_assert(bar != nullptr);
		BarrierCreateInfo barrierInfo;
		ParseBarrierInternal(script, bar, barrier);

		info.leftDependency = barrier.leftDependency;
		info.rightDependency = barrier.rightDependency;
		info.renderTextureBarriers = barrier.renderTextureBarriers;
		info.shaderRWTextures = barrier.shaderRWTextures;
		info.shaderRWBuffers = barrier.shaderRWBuffers;

#ifdef CreateEvent
#pragma push_macro("CreateEvent")
#undef CreateEvent
#endif

		EventId ev = CoreGraphics::CreateEvent(info);

#pragma pop_macro("CreateEvent")

		script->AddEvent(name->string_value, ev);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseShaderStateList(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != NULL);

		bool createResources = false;
		JzonValue* create = jzon_get(cur, "createResourceSet");
		if (create != NULL) createResources = create->bool_value;

		JzonValue* shader = jzon_get(cur, "shader");
		n_assert(shader != NULL);
		CoreGraphics::ShaderStateId state = ShaderServer::Instance()->ShaderCreateState(shader->string_value, { NEBULAT_DEFAULT_GROUP }, createResources);

		JzonValue* vars = jzon_get(cur, "variables");
		if (vars != NULL) ParseShaderVariables(script, state, vars);

		// add state
		script->AddShaderState(name->string_value, state);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseGlobalState(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameGlobalState>();
	Ptr<FrameGlobalState> op = FrameGlobalState::CreateInplace(mem);

	// set name of op
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// create shared state, this will be set while running the script and update the shared state
	CoreGraphics::ShaderStateId state = ShaderServer::Instance()->ShaderCreateSharedState("shd:shared", { NEBULAT_FRAME_GROUP });
	
	state->SetApplyShared(true);
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

	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseBlit(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameBlit>();
	Ptr<FrameBlit> op = FrameBlit::CreateInplace(mem);

	// set name of op
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != NULL);
	const CoreGraphics::RenderTextureId& fromTex = script->GetColorTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != NULL);
	const CoreGraphics::RenderTextureId& toTex = script->GetColorTexture(to->string_value);

	// setup blit operation
	op->from = fromTex;
	op->to = toTex;
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseCopy(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameCopy>();
	Ptr<Frame::FrameCopy> op = Frame::FrameCopy::CreateInplace(mem);

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != NULL);
	const CoreGraphics::RenderTextureId& fromTex = script->GetColorTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != NULL);
	const CoreGraphics::RenderTextureId& toTex = script->GetColorTexture(to->string_value);

	// setup copy operation
	op->from = fromTex;
	op->to = toTex;
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseCompute(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameCompute>();
	Ptr<FrameCompute> op = FrameCompute::CreateInplace(mem);

	// get name of compute sequence
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// create shader state
	JzonValue* shader = jzon_get(node, "shaderState");
	n_assert(shader != NULL);
	ShaderStateId state = script->GetShaderState(shader->string_value);
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
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseComputeAlgorithm(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameComputeAlgorithm>();
	Ptr<FrameComputeAlgorithm> op = FrameComputeAlgorithm::CreateInplace(mem);

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* alg = jzon_get(node, "algorithm");
	n_assert(alg != NULL);
	JzonValue* function = jzon_get(node, "function");
	n_assert(function != NULL);

	// get algorithm
	const Ptr<Algorithms::Algorithm>& algorithm = script->GetAlgorithm(alg->string_value);
	op->alg = algorithm;
	op->funcName = function->string_value;

	// add to script
	op->Setup();
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSwapbuffers(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameSwapbuffers>();
	Ptr<FrameSwapbuffers> op = FrameSwapbuffers::CreateInplace(mem);

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* texture = jzon_get(node, "texture");
	n_assert(texture != NULL);
	const CoreGraphics::RenderTextureId& tex = script->GetColorTexture(texture->string_value);
	op->tex = tex;

	// add operation
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseEvent(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameEvent>();
	Ptr<FrameEvent> op = FrameEvent::CreateInplace(mem);
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	const CoreGraphics::EventId& event = script->GetEvent(name->string_value);

	// go through ops
	JzonValue* ops = jzon_get(node, "actions");
	uint i;
	for (i = 0; i < ops->size; i++)
	{
		JzonValue* action = ops->array_values[i];
		Util::String str(action->string_value);
		FrameEvent::Action a;
		if (str == "wait")			a = FrameEvent::Wait;
		else if (str == "set")		a = FrameEvent::Set;
		else if (str == "reset")	a = FrameEvent::Reset;
		else
		{
			n_error("Event has no operation named '%s'", str.AsCharPtr());
		}

		// add action to op
		op->actions.Append(a);
	}

	JzonValue* waitDependency = jzon_get(node, "dependency");
	Util::String str(waitDependency->string_value);
	op->dependency = BarrierDependencyFromString(str);

	// set event in op
	op->event = event;
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseBarrier(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameBarrier>();
	Ptr<FrameBarrier> op = FrameBarrier::CreateInplace(mem);
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// create barrier
	CoreGraphics::BarrierCreateInfo info;

	// call internal parser
	ParseBarrierInternal(script, node, info);

	BarrierId barrier = CreateBarrier(info);
	op->barrier = barrier;
	script->AddOp(op.upcast<FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseBarrierInternal(const Ptr<Frame::FrameScript>& script, JzonValue* node, CoreGraphics::BarrierCreateInfo& barrier)
{
	Util::Array<Util::String> flags;
	BarrierDependency dep;
	IndexT i;

	// get left side dependency flags
	JzonValue* leftDep = jzon_get(node, "leftDependency");
	n_assert(leftDep != nullptr);
	flags = Util::String(leftDep->string_value).Tokenize("|");
	dep = BarrierDependency::Bottom;
	for (i = 0; i < flags.Size(); i++) dep |= BarrierDependencyFromString(flags[i]);
	barrier.leftDependency = dep;

	// get right side dependency flags
	JzonValue* rightDep = jzon_get(node, "rightDependency");
	n_assert(rightDep != nullptr);
	flags = Util::String(rightDep->string_value).Tokenize("|");
	dep = BarrierDependency::Top;
	for (i = 0; i < flags.Size(); i++) dep |= BarrierDependencyFromString(flags[i]);
	barrier.rightDependency = dep;

	JzonValue* textures = jzon_get(node, "renderTextures");
	if (textures != NULL)
	{
		uint i;
		for (i = 0; i < textures->size; i++)
		{
			Util::Array<Util::String> flags;
			CoreGraphics::BarrierAccess leftAccessFlags, rightAccessFlags;
			IndexT j;

			JzonValue* nd = textures->array_values[i];
			JzonValue* res = jzon_get(nd, "name");
			n_assert(res != nullptr);

			// get left access flags
			JzonValue* leftAccess = jzon_get(nd, "leftAccess");
			n_assert(leftAccess != nullptr);
			flags = Util::String(leftAccess->string_value).Tokenize("|");
			leftAccessFlags = BarrierAccess::NoAccess;
			for (j = 0; j < flags.Size(); j++) leftAccessFlags |= BarrierAccessFromString(flags[i]);

			// get left access flags
			JzonValue* rightAccess = jzon_get(nd, "rightAccess");
			n_assert(rightAccess != nullptr);
			flags = Util::String(rightAccess->string_value).Tokenize("|");
			rightAccessFlags = BarrierAccess::NoAccess;
			for (j = 0; j < flags.Size(); j++) rightAccessFlags |= BarrierAccessFromString(flags[i]);

			// set resource
			Util::String str(res->string_value);
			const CoreGraphics::RenderTextureId& rt = script->GetColorTexture(str);
			barrier.renderTextureBarriers.Append(std::make_tuple(rt, leftAccessFlags, rightAccessFlags));
		}
	}

	JzonValue* images = jzon_get(node, "readWriteTextures");
	if (images != NULL)
	{
		uint i;
		for (i = 0; i < images->size; i++)
		{
			Util::Array<Util::String> flags;
			CoreGraphics::BarrierAccess leftAccessFlags, rightAccessFlags;
			IndexT j;

			JzonValue* nd = images->array_values[i];
			JzonValue* res = jzon_get(nd, "name");
			n_assert(res != nullptr);

			// get left access flags
			JzonValue* leftAccess = jzon_get(nd, "leftAccess");
			n_assert(leftAccess != nullptr);
			flags = Util::String(leftAccess->string_value).Tokenize("|");
			leftAccessFlags = BarrierAccess::NoAccess;
			for (j = 0; j < flags.Size(); j++) leftAccessFlags |= BarrierAccessFromString(flags[i]);

			// get left access flags
			JzonValue* rightAccess = jzon_get(nd, "rightAccess");
			n_assert(rightAccess != nullptr);
			flags = Util::String(rightAccess->string_value).Tokenize("|");
			rightAccessFlags = BarrierAccess::NoAccess;
			for (j = 0; j < flags.Size(); j++) rightAccessFlags |= BarrierAccessFromString(flags[i]);

			// set resource
			Util::String str(res->string_value);
			const CoreGraphics::ShaderRWTextureId& rt = script->GetReadWriteTexture(str);
			barrier.shaderRWTextures.Append(std::make_tuple(rt, leftAccessFlags, rightAccessFlags));
		}
	}

	JzonValue* buffers = jzon_get(node, "readWriteBuffers");
	if (buffers != NULL)
	{
		uint i;
		for (i = 0; i < buffers->size; i++)
		{
			Util::Array<Util::String> flags;
			CoreGraphics::BarrierAccess leftAccessFlags, rightAccessFlags;
			IndexT j;

			JzonValue* nd = buffers->array_values[i];
			JzonValue* res = jzon_get(nd, "name");
			n_assert(res != nullptr);

			// get left access flags
			JzonValue* leftAccess = jzon_get(nd, "leftAccess");
			n_assert(leftAccess != nullptr);
			flags = Util::String(leftAccess->string_value).Tokenize("|");
			leftAccessFlags = BarrierAccess::NoAccess;
			for (j = 0; j < flags.Size(); j++) leftAccessFlags |= BarrierAccessFromString(flags[i]);

			// get left access flags
			JzonValue* rightAccess = jzon_get(nd, "rightAccess");
			n_assert(rightAccess != nullptr);
			flags = Util::String(rightAccess->string_value).Tokenize("|");
			rightAccessFlags = BarrierAccess::NoAccess;
			for (j = 0; j < flags.Size(); j++) rightAccessFlags |= BarrierAccessFromString(flags[i]);

			// set resource
			Util::String str(res->string_value);
			const CoreGraphics::ShaderRWBufferId& rt = script->GetReadWriteBuffer(str);
			barrier.shaderRWBuffers.Append(std::make_tuple(rt, leftAccessFlags, rightAccessFlags));
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParsePass(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	// create pass
	void* mem = script->GetAllocator().Alloc<FramePass>();
	Ptr<FramePass> op = FramePass::CreateInplace(mem);
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
			if (ld != NULL && ld->int_value == 1)
			{
				n_assert2(cd == NULL, "Can't load depth from previous pass AND clear.");				
				depthStencilClearFlags |= Load;
			}

			JzonValue* ls = jzon_get(cur, "loadStencil");
			if (ls != NULL && ls->int_value == 1)
			{
				// can't really load and store
				n_assert2(cs == NULL, "Can't load stenil from previous pass AND clear.");
				depthStencilClearFlags |= LoadStencil;
			}

			JzonValue* sd = jzon_get(cur, "store");
			if (sd != NULL && sd->int_value == 1)
			{
				depthStencilClearFlags |= Store;
			}

			JzonValue* ss = jzon_get(cur, "storeStencil");
			if (ss != NULL && ss->int_value == 1)
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
	script->AddOp(op.upcast<Frame::FrameOp>());
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

		// set if attachment should store at the end of the pass
		JzonValue* store = jzon_get(cur, "store");
		if (store && store->int_value == 1)
		{
			flags |= Store;
		}

		JzonValue* load = jzon_get(cur, "load");
		if (load && load->int_value == 1)
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
FrameScriptLoader::ParseSubpass(const Ptr<Frame::FrameScript>& script, CoreGraphics::PassCreateInfo& pass, const Ptr<Frame::FramePass>& framePass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameSubpass>();
	Ptr<FrameSubpass> frameSubpass = FrameSubpass::CreateInplace(mem);
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
		else if (name == "viewports")			ParseSubpassViewports(script, frameSubpass, cur);
		else if (name == "scissors")			ParseSubpassScissors(script, frameSubpass, cur);
		else if (name == "subpassAlgorithm")	ParseSubpassAlgorithm(script, frameSubpass, cur);
		else if (name == "batch")				ParseSubpassBatch(script, frameSubpass, cur);
		else if (name == "sortedBatch")			ParseSubpassSortedBatch(script, frameSubpass, cur);
		else if (name == "fullscreenEffect")	ParseSubpassFullscreenEffect(script, frameSubpass, cur);
		else if (name == "event")				ParseSubpassEvent(script, frameSubpass, cur);
		else if (name == "barrier")				ParseSubpassBarrier(script, frameSubpass, cur);
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
FrameScriptLoader::ParseSubpassDependencies(const Ptr<Frame::FramePass>& pass, CoreGraphics::Subpass& subpass, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		if (cur->is_int)			subpass.dependencies.Append(cur->int_value);
		else if (cur->is_string)
		{
			Util::String id(cur->string_value);
			const Util::Array<Ptr<FrameSubpass>>& subpasses = pass->GetSubpasses();
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
FrameScriptLoader::ParseSubpassAttachments(const Ptr<Frame::FramePass>& pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
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
FrameScriptLoader::ParseSubpassInputs(const Ptr<Frame::FramePass>& pass, CoreGraphics::Subpass& subpass, Util::Array<Resources::ResourceName>& attachmentNames, JzonValue* node)
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
FrameScriptLoader::ParseSubpassViewports(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
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
FrameScriptLoader::ParseSubpassScissors(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
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
FrameScriptLoader::ParseSubpassAlgorithm(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameSubpassAlgorithm>();
	Ptr<FrameSubpassAlgorithm> op = FrameSubpassAlgorithm::CreateInplace(mem);

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* alg = jzon_get(node, "algorithm");
	n_assert(alg != NULL);
	JzonValue* function = jzon_get(node, "function");
	n_assert(function != NULL);

	// get algorithm
	const Ptr<Algorithms::Algorithm>& algorithm = script->GetAlgorithm(alg->string_value);
	op->alg = algorithm;
	op->funcName = function->string_value;

	// add to script
	op->Setup();
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassBatch(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameSubpassBatch>();
	Ptr<FrameSubpassBatch> op = FrameSubpassBatch::CreateInplace(mem);
	op->batch = CoreGraphics::BatchGroup::FromName(node->string_value);
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassSortedBatch(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameSubpassOrderedBatch>();
	Ptr<FrameSubpassOrderedBatch> op = FrameSubpassOrderedBatch::CreateInplace(mem);
	op->batch = CoreGraphics::BatchGroup::FromName(node->string_value);
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassFullscreenEffect(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameSubpassFullscreenEffect>();
	Ptr<FrameSubpassFullscreenEffect> op = FrameSubpassFullscreenEffect::CreateInplace(mem);

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);
	
	// create shader state
	JzonValue* shaderState = jzon_get(node, "shaderState");
	n_assert(shaderState != NULL);
	op->shaderState = script->GetShaderState(shaderState->string_value);

	// get texture
	JzonValue* texture = jzon_get(node, "sizeFromTexture");
	n_assert(texture != NULL);
	op->tex = script->GetColorTexture(texture->string_value);
	
	// add op to subpass
	op->Setup();
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassEvent(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameEvent>();
	Ptr<FrameEvent> op = FrameEvent::CreateInplace(mem);
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	CoreGraphics::EventId event = script->GetEvent(name->string_value);

	// go through ops
	JzonValue* ops = jzon_get(node, "actions");
	uint i;
	for (i = 0; i < ops->size; i++)
	{
		JzonValue* action = ops->array_values[i];
		Util::String str(action->string_value);
		FrameEvent::Action a;
		if (str == "wait")			a = FrameEvent::Wait;
		else if (str == "set")		a = FrameEvent::Set;
		else if (str == "reset")	a = FrameEvent::Reset;
		else
		{
			n_error("Event has no operation named '%s'", str.AsCharPtr());
		}

		// add action to op
		op->actions.Append(a);
	}

	JzonValue* waitDependency = jzon_get(node, "dependency");
	Util::String str(waitDependency->string_value);
	op->dependency = BarrierDependencyFromString(str);

	// set event in op
	op->event = event;
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassBarrier(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameBarrier>();
	Ptr<FrameBarrier> op = FrameBarrier::CreateInplace(mem);
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// setup barrier
	BarrierCreateInfo info;
	info.domain = BarrierDomain::Pass;

	// call internal parser
	ParseBarrierInternal(script, node, info);

	op->barrier = CreateBarrier(info);
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassSystem(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameSubpassSystem>();
	Ptr<FrameSubpassSystem> op = FrameSubpassSystem::CreateInplace(mem);

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
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassPlugins(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	void* mem = script->GetAllocator().Alloc<FrameSubpassPlugins>();
	Ptr<FrameSubpassPlugins> op = FrameSubpassPlugins::CreateInplace(mem);
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* filter = jzon_get(node, "filter");
	n_assert(filter != NULL);
	op->SetPluginFilter(filter->string_value);
	op->Setup();
	subpass->AddOp(op.upcast<Frame::FrameOp>());
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
	}
}

} // namespace Frame2