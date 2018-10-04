#pragma once
//------------------------------------------------------------------------------
/**
    @class OpenGL4::OGL4RenderDevice
  
    Implements a RenderDevice on top of OpenGL4.
        
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#include "coregraphics/base/renderdevicebase.h"
#include "coregraphics/pixelformat.h"
#include "coregraphics/imagefileformat.h"
#include "coregraphics/shaderstate.h"
#include "coregraphics/bufferlock.h"
#include "ogl4rendertarget.h"

//------------------------------------------------------------------------------
namespace OpenGL4
{

#define NEBULA_USE_FLOAT_DEPTHBUFFER 0
#if NEBULA_USE_FLOAT_DEPTHBUFFER
    #define DepthBits 24
    #define StencilBits 8
    #define ViewportMinZ 1
    #define ViewportMaxZ 0
    #define COREGRAPHICS_STANDARDCLEARDEPTHVALUE 0.0f
#else
    #define DepthBits 24
    #define StencilBits 8
    #define ViewportMinZ 0
    #define ViewportMaxZ 1
    #define COREGRAPHICS_STANDARDCLEARDEPTHVALUE 1.0f
#endif

class OGL4RenderDevice : public Base::RenderDeviceBase
{
    __DeclareClass(OGL4RenderDevice);
    __DeclareSingleton(OGL4RenderDevice);
public:
    /// constructor
    OGL4RenderDevice();
    /// destructor
    virtual ~OGL4RenderDevice();

    /// open the device
    bool Open();
    /// close the device
    void Close();

    /// begin complete frame
	bool BeginFrame(IndexT frameIndex);
    /// set the current vertex stream source
    void SetStreamSource(IndexT streamIndex, const Ptr<CoreGraphics::VertexBuffer>& vb, IndexT offsetVertexIndex);
    /// set current vertex layout
    void SetVertexLayout(const Ptr<CoreGraphics::VertexLayout>& vl);
    /// set current index buffer
    void SetIndexBuffer(const Ptr<CoreGraphics::IndexBuffer>& ib);	
    /// perform computation
	void Compute(int dimX, int dimY, int dimZ, uint flag = NoBarrier); // use MemoryBarrierFlag
    /// begins pass with single rendertarget
    void BeginPass(const Ptr<CoreGraphics::RenderTarget>& rt, const Ptr<CoreGraphics::Shader>& passShader);
    /// begins pass with multiple rendertarget
    void BeginPass(const Ptr<CoreGraphics::MultipleRenderTarget>& mrt, const Ptr<CoreGraphics::Shader>& passShader);
    /// begins pass with rendertarget cube
    void BeginPass(const Ptr<CoreGraphics::RenderTargetCube>& rtc, const Ptr<CoreGraphics::Shader>& passShader);
	/// begin rendering a transform feedback with a vertex buffer as target
    void BeginFeedback(const Ptr<CoreGraphics::FeedbackBuffer>& fb, CoreGraphics::PrimitiveTopology::Code primType, const Ptr<CoreGraphics::Shader>& shader);
	/// begin batch
	void BeginBatch(CoreGraphics::FrameBatchType::Code batchType);
	/// draw current primitives
	void Draw();
	/// draw indexed, instanced primitives (see method header for details)
	void DrawIndexedInstanced(SizeT numInstances, IndexT baseInstance);
	/// draw from stream output/transform feedback buffer
	void DrawFeedback(const Ptr<CoreGraphics::FeedbackBuffer>& fb);
	/// draw from stream output/transform feedback buffer, instanced
	void DrawFeedbackInstanced(const Ptr<CoreGraphics::FeedbackBuffer>& fb, SizeT numInstances);
	/// end batch
	void EndBatch();
    /// end current pass
    void EndPass();
	/// end current feedback
	void EndFeedback();
    /// end complete frame
    void EndFrame(IndexT frameIndex);
    /// present the rendered scene
    void Present();
    /// adds a render target
    void AddRenderTarget(const GLuint& renderTarget) const;
    /// adds a scissor rect
    void SetScissorRect(const Math::rectangle<int>& rect, int index);
	/// sets viewport
	void SetViewport(const Math::rectangle<int>& rect, int index);

    /// save a screenshot to the provided stream
    CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream);
	/// save a region of the screen to the provided stream
	CoreGraphics::ImageFileFormat::Code SaveScreenshot(CoreGraphics::ImageFileFormat::Code fmt, const Ptr<IO::Stream>& outStream, const Math::rectangle<int>& rect, int x, int y);

    /// sets whether or not the render device should tessellate
    void SetUsePatches(bool state);
    /// gets whether or not the render device should tessellate
    bool GetUsePatches();

    static const short MaxNumRenderTargets = 8;
    static const short MaxNumViewports = 16;

private:
    // give shape renderer, and text renderer exclusive access to set viewports and render targets
    friend class OGL4ShapeRenderer;
    friend class OGL4TextRenderer;
	friend class OGL4TransformDevice;
    
    /// open opengl4 device context
    bool OpenOpenGL4Context();
    /// close opengl4 device context
    void CloseOpenGL4Device();
    /// setup the requested adapter for the Direct3D device
    void SetupAdapter();
    /// select the requested buffer formats for the Direct3D device
    void SetupBufferFormats();
    /// setup the remaining presentation parameters
    void SetupPresentParams();
    /// set the initial Direct3D device state
    void SetInitialDeviceState();
	
    /// unbind gl resources
    void UnbindOGL4Resources();
    /// sync with gpu
    void SyncGPU();

        struct ScissorRect
        {
                GLint x;
                GLint y;
                GLsizei width;
                GLsizei height;
        };

        // we have a single frame buffer, a main back buffer texture, and a depth-stencil texture
        static GLuint depthStencilTexture;
        
        Util::FixedArray<ScissorRect> ogl4ScissorRects;
        OGL4RenderTarget::Viewport ogl4DefaultViewport;

#if WIN32
        static HGLRC context;
        static HDC deviceContext;
#else
        static void * context;
#endif

        GLsync gpuSyncQuery;
        GLuint vertexByteSize[MaxNumVertexStreams];
        GLuint vertexByteOffset[MaxNumVertexStreams];
        GLuint vertexBuffers[MaxNumVertexStreams];
        GLuint occlusionQuery;
		GLuint feedbackQuery;

        GLuint adapter;
        static const int numSyncQueries = 1;
        GLuint frameId;

		GLint maxNumTextures;
		GLint maxNumImageTextures;
		bool flushFrameBlock;

		bool useMultiDraw;
		Ptr<CoreGraphics::BufferLock> drawBufferLock;
		GLuint drawBuffers[2];							// 0 is for glDrawArrays, 1 is for glDrawElements
		GLchar *drawArraysPtr, *drawElementsPtr;
		GLuint nextDrawArraysIndex, nextDrawElementsIndex;
};



} // namespace OpenGL4
//------------------------------------------------------------------------------
