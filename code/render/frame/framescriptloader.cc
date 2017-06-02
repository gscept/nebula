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
			Ptr<CoreGraphics::RenderTexture> tex = FrameServer::Instance()->GetWindowTexture();
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
			Ptr<CoreGraphics::RenderTexture> tex = CoreGraphics::RenderTexture::Create();
			tex->SetResourceId(name->string_value);
			tex->SetPixelFormat(fmt);
			tex->SetUsage(RenderTexture::ColorAttachment);
			tex->SetTextureType(Texture::Texture2D);

			JzonValue* layers = jzon_get(cur, "layers");
			if (layers != nullptr) tex->SetLayers(layers->int_value);

			// set relative, dynamic or msaa if defined
			if (jzon_get(cur, "relative"))	tex->SetIsScreenRelative(jzon_get(cur, "relative")->bool_value);
			if (jzon_get(cur, "dynamic"))	tex->SetIsDynamicScaled(jzon_get(cur, "dynamic")->bool_value);
			if (jzon_get(cur, "msaa"))		tex->SetEnableMSAA(jzon_get(cur, "msaa")->bool_value);

			// if cube, use 6 layers
			float depth = 1;
			if (jzon_get(cur, "cube"))
			{
				bool isCube = jzon_get(cur, "cube")->bool_value;
				if (isCube)
				{
					tex->SetTextureType(Texture::TextureCube);
					depth = 6;
				}
			}

			// set dimension after figuring out if the texture is a cube
			tex->SetDimensions((float)width->float_value, (float)height->float_value, depth);

			// add to script
			tex->Setup();
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
		Ptr<CoreGraphics::RenderTexture> tex = CoreGraphics::RenderTexture::Create();
		tex->SetResourceId(name->string_value);
		tex->SetPixelFormat(fmt);
		tex->SetUsage(RenderTexture::DepthStencilAttachment);
		tex->SetTextureType(Texture::Texture2D);

		JzonValue* layers = jzon_get(cur, "layers");
		if (layers != NULL) tex->SetLayers(layers->int_value);

		// set relative, dynamic or msaa if defined
		if (jzon_get(cur, "relative")) tex->SetIsScreenRelative(jzon_get(cur, "relative")->bool_value);
		if (jzon_get(cur, "dynamic")) tex->SetIsDynamicScaled(jzon_get(cur, "dynamic")->bool_value);
		if (jzon_get(cur, "msaa")) tex->SetEnableMSAA(jzon_get(cur, "msaa")->bool_value);

		// if cube, use 6 layers
		float depth = 1;
		if (jzon_get(cur, "cube"))
		{
			bool isCube = jzon_get(cur, "cube")->bool_value;
			if (isCube)
			{
				tex->SetTextureType(Texture::TextureCube);
				depth = 6;
			}
		}

		// set dimension after figuring out if the texture is a cube
		tex->SetDimensions((float)width->float_value, (float)height->float_value, depth);

		// add to script
		tex->Setup();
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
		Ptr<ShaderReadWriteTexture> tex = ShaderReadWriteTexture::Create();
		CoreGraphics::PixelFormat::Code fmt = CoreGraphics::PixelFormat::FromString(format->string_value);
		
		bool relativeSize = false;
		bool dynamicSize = false;
		bool msaa = false;
		if (jzon_get(cur, "relative")) relativeSize = jzon_get(cur, "relative")->bool_value;
		if (jzon_get(cur, "dynamic")) dynamicSize = jzon_get(cur, "dynamic")->bool_value;
		if (jzon_get(cur, "msaa")) msaa = jzon_get(cur, "msaa")->bool_value;
		if (relativeSize)
		{
			tex->SetupWithRelativeSize((float)width->float_value, (float)height->float_value, fmt, name->string_value);
		}
		else
		{
			tex->Setup((SizeT)width->float_value, (SizeT)height->float_value, fmt, name->string_value);
		}

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
		Ptr<ShaderReadWriteBuffer> buffer = ShaderReadWriteBuffer::Create();

		bool relativeSize = false;
		if (jzon_get(cur, "relative")) buffer->SetIsRelativeSize(jzon_get(cur, "relative")->bool_value);

		// setup buffer
		buffer->SetSize(size->int_value);
		buffer->Setup(1);

		// add to script
		script->AddReadWriteBuffer(name->string_value, buffer);
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
		Ptr<Algorithms::Algorithm> alg = (Algorithms::Algorithm*)Core::Factory::Instance()->Create(clazz->string_value);

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
		Ptr<CoreGraphics::Event> ev = CoreGraphics::Event::Create();
		if (jzon_get(cur, "signaled")) ev->SetSignaled(jzon_get(cur, "signaled")->bool_value);

		// create barrier
		Ptr<CoreGraphics::Barrier> barrier = CoreGraphics::Barrier::Create();
		barrier->SetDomain(Barrier::Domain::Global);

		// call internal parser for barrier
		JzonValue* bar = jzon_get(cur, "barrier");
		n_assert(bar != nullptr);
		ParseBarrierInternal(script, bar, barrier);
		barrier->Setup();
		ev->SetBarrier(barrier);

		// add event
		ev->Setup();
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
		Ptr<CoreGraphics::ShaderState> state = ShaderServer::Instance()->CreateShaderState(shader->string_value, { NEBULAT_DEFAULT_GROUP }, createResources);

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
	Ptr<FrameGlobalState> op = FrameGlobalState::Create();

	// set name of op
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// create shared state, this will be set while running the script and update the shared state
	Ptr<CoreGraphics::ShaderState> state = ShaderServer::Instance()->CreateSharedShaderState("shd:shared", { NEBULAT_FRAME_GROUP });
	state->SetApplyShared(true);
	op->SetShaderState(state);

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
			const Ptr<ShaderVariableInstance>& variable = state->GetVariableByName(sem->string_value)->CreateInstance();
			switch (variable->GetShaderVariable()->GetType())
			{
			case ShaderVariable::IntType:
				variable->SetInt(valStr.AsInt());
				break;
			case ShaderVariable::FloatType:
				variable->SetFloat(valStr.AsFloat());
				break;
			case ShaderVariable::VectorType:
				variable->SetFloat4(valStr.AsFloat4());
				break;
			case ShaderVariable::Vector2Type:
				variable->SetFloat2(valStr.AsFloat2());
				break;
			case ShaderVariable::MatrixType:
				variable->SetMatrix(valStr.AsMatrix44());
				break;
			case ShaderVariable::BoolType:
				variable->SetBool(valStr.AsBool());
				break;
			case ShaderVariable::SamplerType:
			case ShaderVariable::TextureType:
			{
				const Ptr<Resources::ResourceManager>& resManager = Resources::ResourceManager::Instance();
				if (resManager->HasResource(valStr))
				{
					const Ptr<Resources::Resource>& resource = resManager->LookupResource(valStr);
					variable->SetTexture(resource.downcast<Texture>());
				}
				break;
			}
			case ShaderVariable::ImageReadWriteType:
				variable->SetShaderReadWriteTexture(script->GetReadWriteTexture(valStr));
				break;
			case ShaderVariable::BufferReadWriteType:
				variable->SetShaderReadWriteBuffer(script->GetReadWriteBuffer(valStr));
				break;
			}

			// add variable to op
			op->AddVariableInstance(variable);
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
	Ptr<FrameBlit> op = FrameBlit::Create();

	// set name of op
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != NULL);
	const Ptr<CoreGraphics::RenderTexture>& fromTex = script->GetColorTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != NULL);
	const Ptr<CoreGraphics::RenderTexture>& toTex = script->GetColorTexture(to->string_value);

	// setup blit operation
	op->SetFromTexture(fromTex);
	op->SetToTexture(toTex);
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseCopy(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	Ptr<Frame::FrameCopy> op = Frame::FrameCopy::Create();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* from = jzon_get(node, "from");
	n_assert(from != NULL);
	const Ptr<CoreGraphics::RenderTexture>& fromTex = script->GetColorTexture(from->string_value);

	JzonValue* to = jzon_get(node, "to");
	n_assert(to != NULL);
	const Ptr<CoreGraphics::RenderTexture>& toTex = script->GetColorTexture(to->string_value);

	// setup copy operation
	op->SetFromTexture(fromTex);
	op->SetToTexture(toTex);
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseCompute(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	Ptr<FrameCompute> op = FrameCompute::Create();

	// get name of compute sequence
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// create shader state
	JzonValue* shader = jzon_get(node, "shaderState");
	n_assert(shader != NULL);
	Ptr<ShaderState> state = script->GetShaderState(shader->string_value);
	op->SetShaderState(state);

	JzonValue* variation = jzon_get(node, "variation");
	n_assert(variation != NULL);
	CoreGraphics::ShaderFeature::Mask mask = ShaderServer::Instance()->FeatureStringToMask(variation->string_value);
	op->SetVariation(mask);

	// dimensions, must be 3
	JzonValue* dims = jzon_get(node, "dimensions");
	n_assert(dims != NULL);
	n_assert(dims->size == 3);
	op->SetInvocations(dims->array_values[0]->int_value, dims->array_values[1]->int_value, dims->array_values[2]->int_value);

	// add op to script
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseComputeAlgorithm(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	Ptr<FrameComputeAlgorithm> op = FrameComputeAlgorithm::Create();

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
	op->SetAlgorithm(algorithm);
	op->SetFunction(function->string_value);

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
	Ptr<FrameSwapbuffers> op = FrameSwapbuffers::Create();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	JzonValue* texture = jzon_get(node, "texture");
	n_assert(texture != NULL);
	const Ptr<CoreGraphics::RenderTexture>& tex = script->GetColorTexture(texture->string_value);
	op->SetTexture(tex);

	// add operation
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseEvent(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	Ptr<FrameEvent> op = FrameEvent::Create();
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	const Ptr<CoreGraphics::Event>& event = script->GetEvent(name->string_value);

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
		op->AddAction(a);
	}

	// set event in op
	op->SetEvent(event);
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseBarrier(const Ptr<Frame::FrameScript>& script, JzonValue* node)
{
	Ptr<FrameBarrier> op = FrameBarrier::Create();
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// create barrier
	Ptr<CoreGraphics::Barrier> barrier = CoreGraphics::Barrier::Create();
	barrier->SetDomain(Barrier::Domain::Global);

	// call internal parser
	ParseBarrierInternal(script, node, barrier);

	op->SetBarrier(barrier);
	script->AddOp(op.upcast<FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseBarrierInternal(const Ptr<Frame::FrameScript>& script, JzonValue* node, const Ptr<CoreGraphics::Barrier>& barrier)
{
	Util::Array<Util::String> flags;
	Barrier::Dependency dep;
	IndexT i;

	// get left side dependency flags
	JzonValue* leftDep = jzon_get(node, "leftDependency");
	n_assert(leftDep != nullptr);
	flags = Util::String(leftDep->string_value).Tokenize("|");
	dep = Barrier::Dependency::NoDependencies;
	for (i = 0; i < flags.Size(); i++) dep |= Barrier::DependencyFromString(flags[i]);
	barrier->SetLeftDependency(dep);

	// get right side dependency flags
	JzonValue* rightDep = jzon_get(node, "rightDependency");
	n_assert(rightDep != nullptr);
	flags = Util::String(rightDep->string_value).Tokenize("|");
	dep = Barrier::Dependency::NoDependencies;
	for (i = 0; i < flags.Size(); i++) dep |= Barrier::DependencyFromString(flags[i]);
	barrier->SetRightDependency(dep);

	JzonValue* textures = jzon_get(node, "renderTextures");
	if (textures != NULL)
	{
		uint i;
		for (i = 0; i < textures->size; i++)
		{
			Util::Array<Util::String> flags;
			CoreGraphics::Barrier::Access leftAccessFlags, rightAccessFlags;
			IndexT j;

			JzonValue* nd = textures->array_values[i];
			JzonValue* res = jzon_get(nd, "name");
			n_assert(res != nullptr);

			// get left access flags
			JzonValue* leftAccess = jzon_get(nd, "leftAccess");
			n_assert(leftAccess != nullptr);
			flags = Util::String(leftAccess->string_value).Tokenize("|");
			leftAccessFlags = Barrier::Access::NoAccess;
			for (j = 0; j < flags.Size(); j++) leftAccessFlags |= Barrier::AccessFromString(flags[i]);

			// get left access flags
			JzonValue* rightAccess = jzon_get(nd, "rightAccess");
			n_assert(rightAccess != nullptr);
			flags = Util::String(rightAccess->string_value).Tokenize("|");
			rightAccessFlags = Barrier::Access::NoAccess;
			for (j = 0; j < flags.Size(); j++) rightAccessFlags |= Barrier::AccessFromString(flags[i]);

			// set resource
			Util::String str(res->string_value);
			const Ptr<CoreGraphics::RenderTexture>& rt = script->GetColorTexture(str);
			barrier->AddRenderTexture(rt, leftAccessFlags, rightAccessFlags);
		}
	}

	JzonValue* images = jzon_get(node, "readWriteTextures");
	if (images != NULL)
	{
		uint i;
		for (i = 0; i < images->size; i++)
		{
			Util::Array<Util::String> flags;
			CoreGraphics::Barrier::Access leftAccessFlags, rightAccessFlags;
			IndexT j;

			JzonValue* nd = images->array_values[i];
			JzonValue* res = jzon_get(nd, "name");
			n_assert(res != nullptr);

			// get left access flags
			JzonValue* leftAccess = jzon_get(nd, "leftAccess");
			n_assert(leftAccess != nullptr);
			flags = Util::String(leftAccess->string_value).Tokenize("|");
			leftAccessFlags = Barrier::Access::NoAccess;
			for (j = 0; j < flags.Size(); j++) leftAccessFlags |= Barrier::AccessFromString(flags[i]);

			// get left access flags
			JzonValue* rightAccess = jzon_get(nd, "rightAccess");
			n_assert(rightAccess != nullptr);
			flags = Util::String(rightAccess->string_value).Tokenize("|");
			rightAccessFlags = Barrier::Access::NoAccess;
			for (j = 0; j < flags.Size(); j++) rightAccessFlags |= Barrier::AccessFromString(flags[i]);

			// set resource
			Util::String str(res->string_value);
			const Ptr<CoreGraphics::ShaderReadWriteTexture>& rt = script->GetReadWriteTexture(str);
			barrier->AddReadWriteTexture(rt, leftAccessFlags, rightAccessFlags);
		}
	}

	JzonValue* buffers = jzon_get(node, "readWriteBuffers");
	if (buffers != NULL)
	{
		uint i;
		for (i = 0; i < buffers->size; i++)
		{
			Util::Array<Util::String> flags;
			CoreGraphics::Barrier::Access leftAccessFlags, rightAccessFlags;
			IndexT j;

			JzonValue* nd = buffers->array_values[i];
			JzonValue* res = jzon_get(nd, "name");
			n_assert(res != nullptr);

			// get left access flags
			JzonValue* leftAccess = jzon_get(nd, "leftAccess");
			n_assert(leftAccess != nullptr);
			flags = Util::String(leftAccess->string_value).Tokenize("|");
			leftAccessFlags = Barrier::Access::NoAccess;
			for (j = 0; j < flags.Size(); j++) leftAccessFlags |= Barrier::AccessFromString(flags[i]);

			// get left access flags
			JzonValue* rightAccess = jzon_get(nd, "rightAccess");
			n_assert(rightAccess != nullptr);
			flags = Util::String(rightAccess->string_value).Tokenize("|");
			rightAccessFlags = Barrier::Access::NoAccess;
			for (j = 0; j < flags.Size(); j++) rightAccessFlags |= Barrier::AccessFromString(flags[i]);

			// set resource
			Util::String str(res->string_value);
			const Ptr<CoreGraphics::ShaderReadWriteBuffer>& rt = script->GetReadWriteBuffer(str);
			barrier->AddReadWriteBuffer(rt, leftAccessFlags, rightAccessFlags);
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
	Ptr<FramePass> op = FramePass::Create();
	Ptr<Pass> pass = Pass::Create();
	op->SetPass(pass);

	// get name of pass
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		Util::String name(cur->key);
		if (name == "name")					op->SetName(cur->string_value);
		else if (name == "attachments")		ParseAttachmentList(script, pass, cur);
		else if (name == "depthStencil")
		{
			float clearDepth = 1;
			uint clearStencil = 0;
			uint depthStencilClearFlags = 0;
			JzonValue* cd = jzon_get(cur, "clear");
			if (cd != NULL)
			{
				depthStencilClearFlags |= Pass::Clear;
				clearDepth = (float)cd->float_value;
			}

			JzonValue* cs = jzon_get(cur, "clearStencil");
			if (cs != NULL)
			{
				depthStencilClearFlags |= Pass::ClearStencil;
				clearStencil = cs->int_value;
			}

			JzonValue* ld = jzon_get(cur, "load");
			if (ld != NULL && ld->int_value == 1)
			{
				n_assert2(cd == NULL, "Can't load depth from previous pass AND clear.");				
				depthStencilClearFlags |= Pass::Load;
			}

			JzonValue* ls = jzon_get(cur, "loadStencil");
			if (ls != NULL && ls->int_value == 1)
			{
				// can't really load and store
				n_assert2(cs == NULL, "Can't load stenil from previous pass AND clear.");
				depthStencilClearFlags |= Pass::LoadStencil;
			}

			JzonValue* sd = jzon_get(cur, "store");
			if (sd != NULL && sd->int_value == 1)
			{
				depthStencilClearFlags |= Pass::Store;
			}

			JzonValue* ss = jzon_get(cur, "storeStencil");
			if (ss != NULL && ss->int_value == 1)
			{
				depthStencilClearFlags |= Pass::StoreStencil;
			}

			// set attachment in framebuffer
			JzonValue* ds = jzon_get(cur, "name");
			pass->SetDepthStencilAttachment(script->GetDepthStencilTexture(ds->string_value));
			pass->SetDepthStencilClear(clearDepth, clearStencil);
			pass->SetDepthStencilFlags((Pass::AttachmentFlagBits)depthStencilClearFlags);
		}
		else if (name == "subpass")				ParseSubpass(script, pass, op, cur);
		else
		{
			n_error("Passes don't support operations, and '%s' is no exception.\n", name.AsCharPtr());
		}
	}

	// setup framebuffer and bind to pass
	pass->Setup();
	script->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseAttachmentList(const Ptr<Frame::FrameScript>& script, const Ptr<CoreGraphics::Pass>& pass, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		JzonValue* name = jzon_get(cur, "name");
		n_assert(name != NULL);
		const Ptr<RenderTexture>& tex = script->GetColorTexture(name->string_value);

		// add texture to pass
		pass->AddColorAttachment(tex);

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
			pass->SetColorAttachmentClear(i, clearValue);
			flags |= Pass::Clear;
		}

		// set if attachment should store at the end of the pass
		JzonValue* store = jzon_get(cur, "store");
		if (store && store->int_value == 1)
		{
			flags |= Pass::Store;
		}

		JzonValue* load = jzon_get(cur, "load");
		if (load && load->int_value == 1)
		{
			// we can't really load and clear
			n_assert2(clear == NULL, "Can't load color if it's being cleared.");
			flags |= Pass::Load;
		}
		pass->SetColorAttachmentFlags(i, (Pass::AttachmentFlagBits)flags);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpass(const Ptr<Frame::FrameScript>& script, const Ptr<CoreGraphics::Pass>& pass, const Ptr<Frame::FramePass>& framePass, JzonValue* node)
{
	Ptr<Frame::FrameSubpass> frameSubpass = Frame::FrameSubpass::Create();
	Pass::Subpass subpass;
	subpass.resolve = false;
	subpass.bindDepth = false;
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		Util::String name(cur->key);
		if (name == "name")						frameSubpass->SetName(cur->string_value);
		else if (name == "dependencies")		ParseSubpassDependencies(framePass, subpass, cur);
		else if (name == "attachments")			ParseSubpassAttachments(framePass, subpass, cur);
		else if (name == "inputs")				ParseSubpassInputs(framePass, subpass, cur);
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
	pass->AddSubpass(subpass);
	framePass->AddSubpass(frameSubpass);
}

//------------------------------------------------------------------------------
/**
	Extend to use subpass names
*/
void
FrameScriptLoader::ParseSubpassDependencies(const Ptr<Frame::FramePass>& pass, CoreGraphics::Pass::Subpass& subpass, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		if (cur->is_int)			subpass.dependencies.Append(cur->int_value);
		else if (cur->is_string)
		{
			Util::String id(cur->string_value);
			const Util::Array<Ptr<Frame::FrameSubpass>>& subpasses = pass->GetSubpasses();
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
FrameScriptLoader::ParseSubpassAttachments(const Ptr<Frame::FramePass>& pass, CoreGraphics::Pass::Subpass& subpass, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		if (cur->is_int)		subpass.attachments.Append(cur->int_value);
		else if (cur->is_string)
		{
			Util::String id(cur->string_value);
			const Ptr<CoreGraphics::Pass>& corepass = pass->GetPass();
			IndexT j;
			for (j = 0; j < corepass->GetNumColorAttachments(); j++)
			{
				const Ptr<CoreGraphics::RenderTexture>& tex = corepass->GetColorAttachment(j);
				if (tex->GetResourceId() == id)
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
FrameScriptLoader::ParseSubpassInputs(const Ptr<Frame::FramePass>& pass, CoreGraphics::Pass::Subpass& subpass, JzonValue* node)
{
	uint i;
	for (i = 0; i < node->size; i++)
	{
		JzonValue* cur = node->array_values[i];
		if (cur->is_int)	subpass.inputs.Append(cur->int_value);
		else if (cur->is_string)
		{
			Util::String id(cur->string_value);
			const Ptr<CoreGraphics::Pass>& corepass = pass->GetPass();
			IndexT j;
			for (j = 0; j < corepass->GetNumColorAttachments(); j++)
			{
				const Ptr<CoreGraphics::RenderTexture>& tex = corepass->GetColorAttachment(j);
				if (tex->GetResourceId() == id)
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
	Ptr<Frame::FrameSubpassAlgorithm> op = Frame::FrameSubpassAlgorithm::Create();

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
	op->SetAlgorithm(algorithm);
	op->SetFunction(function->string_value);

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
	Ptr<Frame::FrameSubpassBatch> op = Frame::FrameSubpassBatch::Create();
	Graphics::BatchGroup::Code code = Graphics::BatchGroup::FromName(node->string_value);
	op->SetBatchCode(code);
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassSortedBatch(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	Ptr<Frame::FrameSubpassOrderedBatch> op = Frame::FrameSubpassOrderedBatch::Create();
	Graphics::BatchGroup::Code code = Graphics::BatchGroup::FromName(node->string_value);
	op->SetBatchCode(code);
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassFullscreenEffect(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	Ptr<Frame::FrameSubpassFullscreenEffect> op = Frame::FrameSubpassFullscreenEffect::Create();

	// get function and name
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);
	
	// create shader state
	JzonValue* shaderState = jzon_get(node, "shaderState");
	n_assert(shaderState != NULL);
	Ptr<ShaderState> state = script->GetShaderState(shaderState->string_value);
	op->SetShaderState(state);

	// get texture
	JzonValue* texture = jzon_get(node, "sizeFromTexture");
	n_assert(texture != NULL);
	Ptr<CoreGraphics::RenderTexture> tex = script->GetColorTexture(texture->string_value);
	op->SetRenderTexture(tex);
	
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
	Ptr<FrameEvent> op = FrameEvent::Create();
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != nullptr);
	const Ptr<CoreGraphics::Event>& event = script->GetEvent(name->string_value);

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
		op->AddAction(a);
	}

	// set event in op
	op->SetEvent(event);
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassBarrier(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	Ptr<FrameBarrier> op = FrameBarrier::Create();
	JzonValue* name = jzon_get(node, "name");
	n_assert(name != NULL);
	op->SetName(name->string_value);

	// setup barrier
	Ptr<CoreGraphics::Barrier> barrier = CoreGraphics::Barrier::Create();
	barrier->SetDomain(Barrier::Domain::Pass);

	// call internal parser
	ParseBarrierInternal(script, node, barrier);

	op->SetBarrier(barrier);
	subpass->AddOp(op.upcast<Frame::FrameOp>());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameScriptLoader::ParseSubpassSystem(const Ptr<Frame::FrameScript>& script, const Ptr<Frame::FrameSubpass>& subpass, JzonValue* node)
{
	Ptr<Frame::FrameSubpassSystem> op = Frame::FrameSubpassSystem::Create();

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
	Ptr<Frame::FrameSubpassPlugins> op = Frame::FrameSubpassPlugins::Create();
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
FrameScriptLoader::ParseShaderVariables(const Ptr<Frame::FrameScript>& script, const Ptr<CoreGraphics::ShaderState>& state, JzonValue* node)
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
		const Ptr<ShaderVariable>& variable = state->GetVariableByName(sem->string_value);
		switch (variable->GetType())
		{
		case ShaderVariable::IntType:
			variable->SetInt(valStr.AsInt());
			break;
		case ShaderVariable::FloatType:
			variable->SetFloat(valStr.AsFloat());
			break;
		case ShaderVariable::VectorType:
			variable->SetFloat4(valStr.AsFloat4());
			break;
		case ShaderVariable::Vector2Type:
			variable->SetFloat2(valStr.AsFloat2());
			break;
		case ShaderVariable::MatrixType:
			variable->SetMatrix(valStr.AsMatrix44());
			break;
		case ShaderVariable::BoolType:
			variable->SetBool(valStr.AsBool());
			break;
		case ShaderVariable::SamplerType:
		case ShaderVariable::TextureType:
		{
			const Ptr<Resources::ResourceManager>& resManager = Resources::ResourceManager::Instance();
			if (resManager->HasResource(valStr))
			{
				const Ptr<Resources::Resource>& resource = resManager->LookupResource(valStr);
				variable->SetTexture(resource.downcast<Texture>());
			}
			break;
		}
		case ShaderVariable::ImageReadWriteType:
			variable->SetShaderReadWriteTexture(script->GetReadWriteTexture(valStr));
			break;
		case ShaderVariable::BufferReadWriteType:
			variable->SetShaderReadWriteBuffer(script->GetReadWriteBuffer(valStr));
			break;
		}
	}
}

} // namespace Frame2