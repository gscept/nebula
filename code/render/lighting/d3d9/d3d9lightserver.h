#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::LightPrePassServer
    
    A LightServer which implements pre-light-pass rendering.
    Check here for details:
    http://diaryofagraphicsprogrammer.blogspot.com/2008/03/light-pre-pass-renderer.html    
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2015 Individual contributors, see AUTHORS file
*/
#include "lighting/base/lightserverbase.h"
#include "graphics/pointlightentity.h"
#include "graphics/spotlightentity.h"
#include "coregraphics/shaderstate.h"
#include "resources/managedmesh.h"
#include "resources/managedtexture.h"
#include "renderutil/drawfullscreenquad.h"

//------------------------------------------------------------------------------
namespace Lighting
{
class LightPrePassServer : public LightServerBase
{
    __DeclareClass(LightPrePassServer);
public:
    /// constructor
    LightPrePassServer();
    /// destructor
    virtual ~LightPrePassServer();

    /// open the light server
    void Open();
    /// close the light server
    void Close();
    /// pre-light-pass doesn't require light/model linking
    bool NeedsLightModelLinking() const;

    /// attach a visible light source
    void AttachVisibleLight(const Ptr<Graphics::AbstractLightEntity>& lightEntity);
    /// end lighting frame
    void EndFrame();

    /// render light pass
    void RenderLights();

protected:
    /// render the global light
    void RenderGlobalLight();
    /// render all point lights
    void RenderPointLights();
    /// render all spot lights
    void RenderSpotLights();
    /// assign render buffers to shaders (one time init)
    void AssignRenderBufferTextures();

    enum ShadowFlag
    {
        CastShadows = 0,
        NoShadows,

        NumShadowFlags
    };

    Util::Array<Ptr<Graphics::PointLightEntity> > pointLights[NumShadowFlags];
    Util::Array<Ptr<Graphics::SpotLightEntity> > spotLights[NumShadowFlags];        
    Ptr<Resources::ManagedTexture> lightProjMap; 

	CoreGraphics::ShaderFeature::Mask pointLightFeatureBits[NumShadowFlags];
    CoreGraphics::ShaderFeature::Mask spotLightFeatureBits[NumShadowFlags];
    CoreGraphics::ShaderFeature::Mask globalLightFeatureBits;
    RenderUtil::DrawFullScreenQuad fullScreenQuadRenderer;          // fs quad renderer
    Ptr<CoreGraphics::ShaderState> lightShader;  // light source shader
	Ptr<CoreGraphics::ShaderState> pointLightShaderInst;
	Ptr<CoreGraphics::ShaderState> spotLightShaderInst;
	Ptr<CoreGraphics::ShaderState> globalLightShaderInst;


	/// global light variables
	Ptr<CoreGraphics::ShaderVariable> globalLightDir;
	Ptr<CoreGraphics::ShaderVariable> globalLightColor;
	Ptr<CoreGraphics::ShaderVariable> globalBackLightColor;
	Ptr<CoreGraphics::ShaderVariable> globalAmbientLightColor;
	Ptr<CoreGraphics::ShaderVariable> globalBackLightOffset;

	Ptr<CoreGraphics::ShaderVariable> lightPosRange;
	Ptr<CoreGraphics::ShaderVariable> lightColor;
	Ptr<CoreGraphics::ShaderVariable> shadowProjTransform;
	Ptr<CoreGraphics::ShaderVariable> shadowOffsetScaleVar;
	Ptr<CoreGraphics::ShaderVariable> lightProjTransform; 
	Ptr<CoreGraphics::ShaderVariable> shadowFadeVar;
	Ptr<CoreGraphics::ShaderVariable> shadowConstants;


    Ptr<CoreGraphics::ShaderVariable> normalBufferVar;
    Ptr<CoreGraphics::ShaderVariable> dsfObjectDepthBufferVar;
    Ptr<CoreGraphics::ShaderVariable> lightBufferVar;       
    Ptr<CoreGraphics::ShaderVariable> lightProjMapVar;
    Ptr<CoreGraphics::ShaderVariable> shadowProjMapVar;      
    


	/// with the DirectX 11 implementation, it is required to be able to switch which texture the lighting shader instance should use...
	Ptr<CoreGraphics::Texture> normalBufferTexture;
	Ptr<CoreGraphics::Texture> dsfObjectDepthBufferTexture;
	Ptr<CoreGraphics::Texture> alphaNormalBufferTexture;
	Ptr<CoreGraphics::Texture> alphaDsfBufferTexture;
	
	bool alpha;

    Ptr<Resources::ManagedMesh> pointLightMesh;         // unit sphere for rendering
    Ptr<Resources::ManagedMesh> spotLightMesh;          // unit sphere for rendering
    bool renderBuffersAssigned;          
};

} // namespace Lighting
//------------------------------------------------------------------------------
