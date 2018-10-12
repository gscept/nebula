//------------------------------------------------------------------------------
//  ogl4renderdevice.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/config.h"
#include "coregraphics/ogl4/ogl4renderdevice.h"
#include "coregraphics/displaydevice.h"
#include "coregraphics/vertexbuffer.h"
#include "coregraphics/indexbuffer.h"
#include "coregraphics//feedbackbuffer.h"
#include "coregraphics/shaderserver.h"
#include "coregraphics/ogl4/ogl4types.h"
#include "framesync/framesynctimer.h"
#include <IL/ilu.h>

#include "GLFW/glfw3native.h"



namespace OpenGL4
{
__ImplementClass(OpenGL4::OGL4RenderDevice, 'O4RD', Base::RenderDeviceBase);
__ImplementSingleton(OpenGL4::OGL4RenderDevice);

using namespace OpenGL4;
using namespace CoreGraphics;


#if __WIN32__
HGLRC OGL4RenderDevice::context;
HDC	OGL4RenderDevice::deviceContext;
#else
void* OGL4RenderDevice::context = 0;
#endif

struct DrawElementsCommand
{
	GLuint indices;
	GLuint instances;
	GLuint baseIndex;
	GLuint baseVertex;
	GLuint baseInstance;
};

struct DrawArraysCommand
{
	GLuint vertices;
	GLuint instances;
	GLuint baseVertex;
	GLuint baseInstance;
};

#if NEBULA_OPENGL4_DEBUG

// thank you.
#ifdef __WIN32__
#define APICALLTYPE __stdcall
#else
#define APICALLTYPE
#endif

// uncomment this for extra spammy opengl info output
//#define NEBULA_OPENGL4_VERBOSE_DEBUG 1
//------------------------------------------------------------------------------
/**
*/
static void APICALLTYPE
NebulaGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	Util::String msg("[OPENGL DEBUG MESSAGE] ");

	// print error severity
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_LOW:
		msg.Append("<Low severity> ");
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		msg.Append("<Medium severity> ");
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		msg.Append("<High severity> ");
		break;
	}

	// append message to output
	msg.Append(message);

	// print message
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:
        n_warning("Error: %s\n", msg.AsCharPtr());
        break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		n_warning("Undefined behavior: %s\n", msg.AsCharPtr());
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		n_warning("Performance issue: %s\n", msg.AsCharPtr());
		break;
	default:		// Portability, Deprecated, Other
#ifdef NEBULA_OPENGL4_VERBOSE_DEBUG
		n_printf("%s\n", msg.AsCharPtr());
#endif
		break;
	}
}
#endif

//------------------------------------------------------------------------------
/**
*/
OGL4RenderDevice::OGL4RenderDevice() :
    adapter(0),
    frameId(0),
	flushFrameBlock(false),
	useMultiDraw(false),
	nextDrawArraysIndex(0),
	nextDrawElementsIndex(0)	
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
OGL4RenderDevice::~OGL4RenderDevice()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Open the render device. When successful, the RenderEvent::DeviceOpen
    will be sent to all registered event handlers after the Direct3D
    device has been opened.
*/
bool
OGL4RenderDevice::Open()
{
    n_assert(!this->IsOpen());
    bool success = false;
    if (this->OpenOpenGL4Context())
    {
        // hand to parent class, this will notify event handlers
        success = RenderDeviceBase::Open();
    }
//    this->context = success;
    return success;
}

//------------------------------------------------------------------------------
/**
    Close the render device. The RenderEvent::DeviceClose will be sent to
    all registered event handlers.
*/
void
OGL4RenderDevice::Close()
{
    n_assert(this->IsOpen());
	this->CloseOpenGL4Device();
    RenderDeviceBase::Close();
}

//------------------------------------------------------------------------------
/**
    This selects the adapter to use for the render device. This
    method will set the "adapter" member and the ogl4 device caps
    member.
*/
void
OGL4RenderDevice::SetupAdapter()
{
    DisplayDevice* displayDevice = DisplayDevice::Instance();
    Adapter::Code requestedAdapter = displayDevice->GetAdapter();
    n_assert(displayDevice->AdapterExists(requestedAdapter));

	this->adapter = (GLuint)requestedAdapter;
}

//------------------------------------------------------------------------------
/**
    Select the display, back buffer and depth buffer formats and update
    the presentParams member. 
*/
void
OGL4RenderDevice::SetupBufferFormats()
{
	// do nothing! GLFW handles this!
}

//------------------------------------------------------------------------------
/**
    Initialize the OpenGL4 device with initial device state.
*/
void
OGL4RenderDevice::SetInitialDeviceState()
{
	DisplayDevice* displayDevice = DisplayDevice::Instance();
	this->ogl4DefaultViewport.x = displayDevice->GetCurrentWindow()->GetDisplayMode().GetXPos();
	this->ogl4DefaultViewport.y = displayDevice->GetCurrentWindow()->GetDisplayMode().GetYPos();
	this->ogl4DefaultViewport.width = displayDevice->GetCurrentWindow()->GetDisplayMode().GetWidth();
	this->ogl4DefaultViewport.height = displayDevice->GetCurrentWindow()->GetDisplayMode().GetHeight();

	IndexT i;
	for (i = 0; i < MaxNumViewports; i++)
	{
		glViewportIndexedf(i, (float)this->ogl4DefaultViewport.x, (float)this->ogl4DefaultViewport.y, (float)this->ogl4DefaultViewport.width, (float)this->ogl4DefaultViewport.height);
		glDepthRangeIndexed(i, ViewportMinZ, ViewportMaxZ);
	}
}

//------------------------------------------------------------------------------
/**
    Open the OpenGL device. This will completely setup the device
    into a usable state.

	Opening OpenGL seems a bit too openly funny.
*/
bool
OGL4RenderDevice::OpenOpenGL4Context()
{    
    DisplayDevice* displayDevice = DisplayDevice::Instance();
    n_assert(displayDevice->IsOpen());

    // setup device creation parameters
    this->SetupAdapter();

#if __WIN32__
	glfwMakeContextCurrent(displayDevice->currentWindow->window);
	this->context = glfwGetWGLContext(displayDevice->currentWindow->window);
#else
	glfwMakeContextCurrent(displayDevice->currentWindow->window);
	this->context = glfwGetGLXContext(displayDevice->currentWindow->window);
#endif

	GLenum status = glewInit();
	if (status != GLEW_OK)
	{
		n_error("OGL4RenderDevice::OpenOpenGL4Context(): failed to initialized GLEW\n");
	}

	// assert that we have GL version 4.3
	n_assert(GLEW_VERSION_4_3);

#if NEBULA_OPENGL4_DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(NebulaGLDebugCallback, NULL);
	GLuint unusedIds;
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
#endif

	// assume everything works fine
	GLenum err = glGetError();
	n_assert(GLSUCCESS);

	// enable seamless cubemaps and SRGB framebuffers
    glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_MULTISAMPLE);
    glDisable(GL_DITHER);
	glEnable(GL_LINE_SMOOTH);
    //glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	//glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    //glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);

	// setup vsync
	this->SetupBufferFormats();

    // set initial device state
    this->SetInitialDeviceState();

	// get number of texture slots available
	glGetIntegerv(GL_MAX_IMAGE_UNITS, &this->maxNumImageTextures);
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &this->maxNumTextures);


#if OGL4_DRAW_INDIRECT
	glGenBuffers(2, this->drawBuffers);
	GLenum flags = GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;

	// setup buffer for draw arrays
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->drawBuffers[0]);
	glBufferStorage(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawArraysCommand) * 1024, NULL, flags);
	this->drawArraysPtr = (GLchar*)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawArraysCommand) * 1024, flags);

	// setup buffer for draw elements
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->drawBuffers[1]);
	glBufferStorage(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawElementsCommand) * 1024, NULL, flags);
	this->drawElementsPtr = (GLchar*)glMapBufferRange(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawElementsCommand) * 1024, flags);

	// unbind buffer
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
#endif

	// assert everything works fine
	n_assert(GLSUCCESS);

	this->ogl4ScissorRects.Resize(16);
    IndexT i;
	for (i = 0; i < this->ogl4ScissorRects.Size(); i++)
	{
		this->ogl4ScissorRects[i].x = 0;
		this->ogl4ScissorRects[i].y = 0;
		this->ogl4ScissorRects[i].width = 0;
		this->ogl4ScissorRects[i].height = 0;
	}

	// create occlusion query
	glGenQueries(1, &this->occlusionQuery);
	glGenQueries(1, &this->feedbackQuery);
	n_assert(GLSUCCESS);

    return true;
}

//------------------------------------------------------------------------------
/**
    Close the OpenGL device.
*/
void
OGL4RenderDevice::CloseOpenGL4Device()
{
	n_assert(0 != this->context);
	glDeleteQueries(1, &this->occlusionQuery);
	glDeleteQueries(1, &this->feedbackQuery);
	//n_assert(GLSUCCESS);

    this->UnbindOGL4Resources();       
}

//------------------------------------------------------------------------------
/**
Sets the vertex buffer to use for the next Draw().
*/
void
OGL4RenderDevice::SetStreamSource(IndexT streamIndex, const Ptr<VertexBuffer>& vb, IndexT offsetVertexIndex)
{
	n_assert((streamIndex >= 0) && (streamIndex < MaxNumVertexStreams));
	n_assert(this->inBeginPass);
	n_assert(vb.isvalid());

	// in GL, this is handled inside the VAO's, so all of this is just to keep track of DX vertex streaming
	if ((this->streamVertexBuffers[streamIndex] != vb) || (this->streamVertexOffsets[streamIndex] != offsetVertexIndex))
	{
		vertexByteSize[streamIndex] = vb->GetVertexLayout()->GetVertexByteSize();
		vertexByteOffset[streamIndex] = offsetVertexIndex * vb->GetVertexLayout()->GetVertexByteSize();
		vertexBuffers[streamIndex] = vb->GetOGL4VertexBuffer();

#if __OGL43__
		// bind vertex buffer with individual stream offset
		glBindVertexBuffer(streamIndex, vertexBuffers[streamIndex], vertexByteOffset[streamIndex], vertexByteSize[streamIndex]);
#endif
	}
	RenderDeviceBase::SetStreamVertexBuffer(streamIndex, vb, offsetVertexIndex);
}

//------------------------------------------------------------------------------
/**
	Sets the vertex layout for the next Draw().
	In OpenGL, the vertex layout and vertex buffers are paired, so glBindVertexArray binds both the buffer and the layout.
*/
void
OGL4RenderDevice::SetVertexLayout(const Ptr<VertexLayout>& vl)
{
	n_assert(vl.isvalid());

	if (this->vertexLayout != vl)
	{
		glBindVertexArray(vl->GetOGL4VertexArrayObject());
	}
	RenderDeviceBase::SetVertexLayout(vl);
}

//------------------------------------------------------------------------------
/**
	Sets the index buffer to use for the next Draw().
*/
void
OGL4RenderDevice::SetIndexBuffer(const Ptr<IndexBuffer>& ib)
{
	// only apply if the currently bound index buffer is not equal to the one being passed
	//if (this->indexBuffer != ib)
	{
		if (!ib.isvalid())
		{
			// unbind buffer if we pass NULL
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		else
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib->GetOGL4IndexBuffer());
		}
	}
	RenderDeviceBase::SetIndexBuffer(ib);
}

//------------------------------------------------------------------------------
/**
    Begin a complete frame. Call this once per frame before any rendering 
    happens. If rendering is not possible for some reason (e.g. a lost
    device) the method will return false. This method may also send
    the DeviceLost and DeviceRestored RenderEvents to attached event
    handlers.
*/
bool
OGL4RenderDevice::BeginFrame(IndexT frameIndex)
{
	n_assert(0 != this->context);

	// reset viewport to original
	glViewportIndexedf(0,
			(float)this->ogl4DefaultViewport.x,
			(float)this->ogl4DefaultViewport.y,
			(float)this->ogl4DefaultViewport.width,
			(float)this->ogl4DefaultViewport.height
	);

	return RenderDeviceBase::BeginFrame(frameIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::BeginPass(const Ptr<CoreGraphics::RenderTarget>& rt, const Ptr<CoreGraphics::Shader>& passShader)
{
	n_assert(rt.isvalid());

	// begin pass by binding framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rt->GetFramebuffer());

	// if we have a resolve rect array, use that one first, otherwise fall back to the standard rect
	if (rt->IsResolveRectArrayValid())
	{
		const Util::Array<Math::rectangle<int> >& resolveRects = rt->GetResolveRectArray();
        
		IndexT i;
		for (i = 0; i < resolveRects.Size(); i++)
		{
			const Math::rectangle<int>& rect = resolveRects[i];
			glViewportIndexedf(i, (float)rect.left, (float)rect.top, (float)rect.width(), (float)rect.height());
		}
        
        //glViewportArrayv(0, resolveRects.Size(), (float*)&resolveRects[0]);
	}
	else
	{
		// assign render target view rect
		const ScissorRect& scissor = this->ogl4ScissorRects[0];
		const Math::rectangle<int>& viewportRect = rt->GetResolveRect();
		glViewport(viewportRect.left, viewportRect.top, viewportRect.width(), viewportRect.height());
	}

	RenderDeviceBase::BeginPass(rt, passShader);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::BeginPass(const Ptr<CoreGraphics::MultipleRenderTarget>& mrt, const Ptr<CoreGraphics::Shader>& passShader)
{
	n_assert(mrt.isvalid());

	// begin pass by binding framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mrt->GetFramebuffer());

	if (this->ogl4ScissorRects.Size())
	{
		const ScissorRect& scissor = this->ogl4ScissorRects[0];
		const Math::rectangle<int>& rect = mrt->GetRenderTarget(0)->GetResolveRect();
		glViewport(rect.left, rect.top, rect.width(), rect.height());
	}

	RenderDeviceBase::BeginPass(mrt, passShader);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::BeginPass(const Ptr<CoreGraphics::RenderTargetCube>& rtc, const Ptr<CoreGraphics::Shader>& passShader)
{
	n_assert(rtc.isvalid());

	// begin pass by binding framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rtc->GetOGL4Framebuffer());

	// assign render target view rect
	const Math::rectangle<int>& viewportRect = rtc->GetResolveRect();
	glViewportIndexedf(0, (float)viewportRect.left, (float)viewportRect.top, (float)viewportRect.width(), (float)viewportRect.height());
	RenderDeviceBase::BeginPass(rtc, passShader);
}

//------------------------------------------------------------------------------
/**
	Begins transform feedback by disabling the rasterizer and starting the GL process of transform feedbacking
*/
void
OGL4RenderDevice::BeginFeedback(const Ptr<CoreGraphics::FeedbackBuffer>& fb, CoreGraphics::PrimitiveTopology::Code primType, const Ptr<CoreGraphics::Shader>& shader)
{
	n_assert(fb.isvalid());
	RenderDeviceBase::BeginFeedback(fb, primType, shader);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, fb->GetOGL4TransformFeedback());
	glBeginTransformFeedback(primType);	
	glEnable(GL_RASTERIZER_DISCARD);
	glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, this->feedbackQuery);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::BeginBatch(CoreGraphics::FrameBatchType::Code batchType)
{
	RenderDeviceBase::BeginBatch(batchType);
    const Ptr<ShaderServer>& shdServer = ShaderServer::Instance();
    this->usePatches = shdServer->GetActiveShader()->GetActiveVariation()->UsePatches();
#if OGL4_DRAW_INDIRECT
	this->useMultiDraw = true;
#endif

	this->nextDrawArraysIndex = 0;
	this->nextDrawElementsIndex = 0;
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::EndBatch()
{
	RenderDeviceBase::EndBatch();

	if (this->useMultiDraw)
	{
		// get index type
		GLenum indexType = OGL4Types::IndexTypeAsOGL4Format(this->indexBuffer->GetIndexType());

		// bind buffers and do multi-draw!
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->drawBuffers[0]);
		glMultiDrawArraysIndirect(GL_TRIANGLES, NULL, this->nextDrawArraysIndex, sizeof(DrawArraysCommand));
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->drawBuffers[1]);
		glMultiDrawElementsIndirect(GL_TRIANGLES, indexType, NULL, this->nextDrawElementsIndex, sizeof(DrawElementsCommand));
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	}

#if OGL4_DRAW_INDIRECT
	this->useMultiDraw = false;
#endif

	this->usePatches = false;
}


//------------------------------------------------------------------------------
/**
End the current rendering pass. This will flush all texture stages
in order to keep the ogl4 resource reference counts consistent without too
much hassle.
*/
void
OGL4RenderDevice::EndPass()
{
	RenderDeviceBase::EndPass();
	this->UnbindOGL4Resources();
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::EndFeedback()
{
	glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

	// NO, THIS SHOULD NOT BE REQUIRED!
	//GLint numPrimitivesWritten;	
	//glGetQueryObjectiv(this->feedbackQuery, GL_QUERY_RESULT_NO_WAIT, &numPrimitivesWritten);
	glDisable(GL_RASTERIZER_DISCARD);
	glEndTransformFeedback();	
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
	
	this->UnbindOGL4Resources();
	RenderDeviceBase::EndFeedback();
}

//------------------------------------------------------------------------------
/**
    End a complete frame. Call this once per frame after rendering
    has happened and before Present(), and only if BeginFrame() returns true.
*/
void
OGL4RenderDevice::EndFrame(IndexT frameIndex)
{
    RenderDeviceBase::EndFrame(frameIndex);
}

//------------------------------------------------------------------------------
/**
    NOTE: Present() should be called as late as possible after EndFrame()
    to improve parallelism between the GPU and the CPU.
*/
void
OGL4RenderDevice::Present()
{
    RenderDeviceBase::Present();	

	// wait for all GL-calls to finish
    // we need this in order to ensure this frame is done before we start working on the next one
    this->frameId++;
	
	// swap buffers on main window
	
}

//------------------------------------------------------------------------------
/**
    Draw the current primitive group. Requires a vertex buffer, an
    optional index buffer and a primitive group to be set through the
    respective methods. To use non-indexed rendering, set the number
    of indices in the primitive group to 0.
*/
void
OGL4RenderDevice::Draw()
{
	n_assert(this->inBeginPass);
	n_assert(0 != this->context);

	GLenum primType = OGL4Types::AsOGL4PrimitiveType(this->primitiveGroup.GetPrimitiveTopology());

	// if we are going to tessellate the surface, we will need another type of primitive
	if (this->usePatches)
	{
		primType = GL_PATCHES;
	}

	if (this->useMultiDraw)
	{
		if (this->primitiveGroup.GetNumIndices() > 0)
		{
			// draw arrays
			DrawArraysCommand command;
			command.baseVertex = this->primitiveGroup.GetBaseVertex();
			command.vertices = this->primitiveGroup.GetNumVertices();
			command.baseInstance = 0;
			command.instances = 1;
			memcpy(this->drawArraysPtr + this->nextDrawArraysIndex * sizeof(DrawArraysCommand), &command, sizeof(DrawArraysCommand));
			this->nextDrawArraysIndex++;
		}
		else
		{
			// draw elements
			DrawElementsCommand command;
			command.baseIndex = this->primitiveGroup.GetBaseIndex();
			command.indices = this->primitiveGroup.GetNumIndices();
			command.baseVertex = this->primitiveGroup.GetBaseVertex();
			command.baseInstance = 0;
			command.instances = 1;
			memcpy(this->drawElementsPtr + this->nextDrawElementsIndex * sizeof(DrawElementsCommand), &command, sizeof(DrawElementsCommand));
			this->nextDrawElementsIndex++;
		}
	}
	else
	{
		if (this->primitiveGroup.GetNumIndices() > 0)
		{
			// draw indexed geometry
			GLenum indexType = OGL4Types::IndexTypeAsOGL4Format(this->indexBuffer->GetIndexType());
			GLuint indexSize = indexType == GL_UNSIGNED_SHORT ? 2 : 4;
			glDrawElementsBaseVertex(
				primType,															// primitive type
				this->primitiveGroup.GetNumIndices(),								// number of primitives
				indexType,															// type of index				
				(GLvoid*)(this->primitiveGroup.GetBaseIndex() * indexSize),			// offset into index buffer
				this->primitiveGroup.GetBaseVertex()								// vertex offset
			);				
		}
		else
		{
			glDrawArrays(primType, this->primitiveGroup.GetBaseVertex(), this->primitiveGroup.GetNumVertices());
		}
	}

	// empty buffer lock queues
	this->DequeueBufferLocks();

    // update debug stats
    _incr_counter(RenderDeviceNumPrimitives, this->primitiveGroup.GetNumPrimitives());
    _incr_counter(RenderDeviceNumDrawCalls, 1);
}

//------------------------------------------------------------------------------
/**
    Draw N instances of the current primitive group. Requires the following
    setup:
        
        - vertex stream 0: vertex buffer with instancing data, one vertex per instance
        - vertex stream 1: vertex buffer with instance geometry data
        - index buffer: index buffer for geometry data
        - primitive group: the primitive group which describes one instance
        - vertex declaration: describes a combined vertex from stream 0 and stream 1
*/
void
OGL4RenderDevice::DrawIndexedInstanced(SizeT numInstances, IndexT baseInstance)
{
	n_assert(this->inBeginPass);
	n_assert(numInstances > 0);
	n_assert(0 != this->context);

	GLenum primType = OGL4Types::AsOGL4PrimitiveType(this->primitiveGroup.GetPrimitiveTopology());

	// if we are going to tessellate the surface, we will need another type of primitive
	if (this->usePatches)
	{
		primType = GL_PATCHES;
	}

	if (this->useMultiDraw)
	{
		if (this->primitiveGroup.GetNumIndices() > 0)
		{
			DrawArraysCommand command;
			command.baseVertex = this->primitiveGroup.GetBaseVertex();
			command.vertices = this->primitiveGroup.GetNumVertices();
			command.baseInstance = baseInstance;
			command.instances = numInstances;
			memcpy(this->drawArraysPtr + this->nextDrawArraysIndex * sizeof(DrawArraysCommand), &command, sizeof(DrawArraysCommand));
			this->nextDrawArraysIndex++;
		}
		else
		{
			DrawElementsCommand command;
			command.baseIndex = this->primitiveGroup.GetBaseIndex();
			command.indices = this->primitiveGroup.GetNumIndices();
			command.baseVertex = this->primitiveGroup.GetBaseVertex();
			command.baseInstance = baseInstance;
			command.instances = numInstances;
			memcpy(this->drawElementsPtr + this->nextDrawElementsIndex * sizeof(DrawElementsCommand), &command, sizeof(DrawElementsCommand));
			this->nextDrawElementsIndex++;
		}
	}
	else
	{
		if (this->primitiveGroup.GetNumIndices() > 0)
		{
			// draw instanced elements
			GLenum indexType = OGL4Types::IndexTypeAsOGL4Format(this->indexBuffer->GetIndexType());
			GLuint indexSize = indexType == GL_UNSIGNED_SHORT ? 2 : 4;
			glDrawElementsInstancedBaseVertexBaseInstance(
				primType,															// primitive type
				this->primitiveGroup.GetNumIndices(),								// amount of indices
				indexType,															// type of index to use
				(GLvoid*)(this->primitiveGroup.GetBaseIndex() * indexSize),			// pointer to indices (NULL since we use VAOs or IBOs)
				numInstances,														// amount of instances to render
				this->primitiveGroup.GetBaseVertex(),								// start index of vertex
				baseInstance														// first instance
			);
		}
		else
		{
			// draw instanced arrays
			glDrawArraysInstancedBaseInstance(primType, this->primitiveGroup.GetBaseVertex(), this->primitiveGroup.GetNumVertices(), numInstances, baseInstance);
		}
	}

	// empty buffer lock queues
	this->DequeueBufferLocks();

    // update debug stats
    _incr_counter(RenderDeviceNumPrimitives, this->primitiveGroup.GetNumPrimitives() * numInstances);
    _incr_counter(RenderDeviceNumDrawCalls, 1);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::DrawFeedback(const Ptr<CoreGraphics::FeedbackBuffer>& fb)
{
	n_assert(this->inBeginPass);
	n_assert(0 != this->context);

	GLenum primType = OGL4Types::AsOGL4PrimitiveType(this->primitiveGroup.GetPrimitiveTopology());
	glDrawTransformFeedback(primType, fb->GetOGL4TransformFeedback());

	// empty buffer lock queues
	this->DequeueBufferLocks();

	// update debug stats
	_incr_counter(RenderDeviceNumPrimitives, this->primitiveGroup.GetNumPrimitives());
	_incr_counter(RenderDeviceNumDrawCalls, 1);
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::DrawFeedbackInstanced(const Ptr<CoreGraphics::FeedbackBuffer>& fb, SizeT numInstances)
{
	n_assert(this->inBeginPass);
	n_assert(numInstances > 0);
	n_assert(0 != this->context);

	GLenum primType = OGL4Types::AsOGL4PrimitiveType(this->primitiveGroup.GetPrimitiveTopology());
	glDrawTransformFeedbackInstanced(primType, fb->GetOGL4TransformFeedback(), numInstances);

	// empty buffer lock queues
	this->DequeueBufferLocks();

	// update debug stats
	_incr_counter(RenderDeviceNumPrimitives, this->primitiveGroup.GetNumPrimitives() * numInstances);
	_incr_counter(RenderDeviceNumDrawCalls, numInstances);
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderDevice::Compute(int dimX, int dimY, int dimZ, uint flag)
{
    n_assert(0 != this->context);

	// put in memory barrier flags
	if (flag != RenderDeviceBase::NoBarrier)
	{
		GLbitfield flags = 0;
		if (flag & RenderDevice::ImageAccessBarrier)		flags |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
		if (flag & RenderDevice::BufferAccessBarrier)		flags |= GL_SHADER_STORAGE_BARRIER_BIT;
		if (flag & RenderDevice::SamplerAccessBarrier)		flags |= GL_TEXTURE_FETCH_BARRIER_BIT;
		if (flag & RenderDevice::RenderTargetAccessBarrier) flags |= GL_FRAMEBUFFER_BARRIER_BIT;
		glMemoryBarrier(flags);
	}

	// run computation
    glDispatchCompute(dimX, dimY, dimZ);

	// empty buffer lock queues
	this->DequeueBufferLocks();

    // update debug stats
    _incr_counter(RenderDeviceNumComputes, dimX * dimY * dimZ);
}

//------------------------------------------------------------------------------
/**
    Save the backbuffer to the provided stream.
*/
ImageFileFormat::Code
OGL4RenderDevice::SaveScreenshot(ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream)
{
	n_assert(!this->inBeginFrame);
    n_assert(0 != this->context);

	DisplayMode mode = DisplayDevice::Instance()->GetCurrentWindow()->GetDisplayMode();
	GLuint size = mode.GetWidth() * mode.GetHeight() * 4;

	// set a proper byte alignment for the read-back
	GLuint pixelAlignment = OGL4Types::AsOGL4PixelByteAlignment(PixelFormat::SRGBA8);
	glPixelStorei(GL_PACK_ALIGNMENT, pixelAlignment);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	GLubyte* data = n_new_array(GLubyte, size);
	glReadPixels(0, 0, mode.GetWidth(), mode.GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, data);	

	ILint image = ilGenImage();
	ilBindImage(image);

	ILboolean result;
	result = ilTexImage(mode.GetWidth(), mode.GetHeight(), 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, data);
	iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);
	ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);

	// now save as bmp
	size = ilGetInteger(IL_IMAGE_WIDTH) * ilGetInteger(IL_IMAGE_HEIGHT) * 3;
	result = ilSaveL(IL_PNG, data, size);
	outStream->SetAccessMode(IO::Stream::WriteAccess);
    if (outStream->Open())
    {
        outStream->Write(data, size);
        outStream->Close();
        outStream->SetMediaType(ImageFileFormat::ToMediaType(ImageFileFormat::PNG));
    }

	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	// cleanup image
	ilDeleteImage(image);
	n_delete_array(data);

	return ImageFileFormat::PNG;
}

//------------------------------------------------------------------------------
/**
*/
ImageFileFormat::Code
OGL4RenderDevice::SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y)
{
	n_assert(!this->inBeginFrame);
	n_assert(0 != this->context);

	DisplayMode mode = DisplayDevice::Instance()->GetCurrentWindow()->GetDisplayMode();
	GLuint size = mode.GetWidth() * mode.GetHeight() * 3;

	// set a proper byte alignment for the read-back
	GLuint pixelAlignment = OGL4Types::AsOGL4PixelByteAlignment(PixelFormat::SRGBA8);
	glPixelStorei(GL_PACK_ALIGNMENT, pixelAlignment);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	GLubyte* data = n_new_array(GLubyte, size);
	glReadPixels(rect.left, rect.top, rect.width(), rect.height(), GL_RGBA, GL_UNSIGNED_BYTE, data);

	ILint image = ilGenImage();
	ilBindImage(image);

	ILboolean result;
	result = ilTexImage(rect.width(), rect.height(), 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, data);
	iluImageParameter(ILU_PLACEMENT, ILU_UPPER_LEFT);
	ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
	result = iluScale(x, y, 1);

	// now save as bmp
	size = ilGetInteger(IL_IMAGE_WIDTH) * ilGetInteger(IL_IMAGE_HEIGHT) * 3;
	result = ilSaveL(IL_PNG, data, size);
	outStream->SetAccessMode(IO::Stream::WriteAccess);
	if (outStream->Open())
	{
		outStream->Write(data, size);
		outStream->Close();
		outStream->SetMediaType(ImageFileFormat::ToMediaType(ImageFileFormat::PNG));
	}

	glPixelStorei(GL_PACK_ALIGNMENT, 4);

	// cleanup image
	ilDeleteImage(image);
	n_delete_array(data);

	return ImageFileFormat::PNG;
}

//------------------------------------------------------------------------------
/**
    Unbind all ogl4 resources from the device, this is necessary to keep the
    resource reference counts consistent. Should be called at the
    end of each rendering pass.
*/
void
OGL4RenderDevice::UnbindOGL4Resources()
{
	this->vertexLayout = 0;
	this->indexBuffer = 0;
	IndexT i;
	for (i = 0; i < MaxNumVertexStreams; i++)
	{
		this->streamVertexBuffers[i] = 0;
	}
    this->primitiveGroup = PrimitiveGroup();
}

//------------------------------------------------------------------------------
/**
*/
void 
OGL4RenderDevice::SyncGPU()
{	
	this->frameId++;
	while ( GL_CONDITION_SATISFIED == glClientWaitSync(this->gpuSyncQuery, GL_SYNC_FLUSH_COMMANDS_BIT, 0))
	{
		// loop until this is reached, then return
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::SetScissorRect(const Math::rectangle<int>& rect, int index)
{
	// Hmm, a bit iffy, the scissor rect is only defined for the back buffer since we use the display as a basis for our offsets
	Window* wnd = DisplayDevice::Instance()->GetCurrentWindow();
	int bottom = wnd->GetDisplayMode().GetHeight() - rect.bottom;
	glScissorIndexed(index, rect.left, bottom, rect.width(), rect.height());	
}

//------------------------------------------------------------------------------
/**
*/
void
OGL4RenderDevice::SetViewport(const Math::rectangle<int>& rect, int index)
{
	glViewportIndexedf(index, (float)rect.left, (float)rect.top, (float)rect.width(), (float)rect.height());
}

} // namespace CoreGraphics
