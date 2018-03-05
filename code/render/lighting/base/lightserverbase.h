#pragma once
//------------------------------------------------------------------------------
/**
    @class Lighting::LightServerBase
    
    Base class for the light server. The light server collects all lights
    contributing to the scene. Subclasses of LightServerBase implement 
    specific lighting techniques.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "coregraphics/shader.h"
#include "graphics/camera.h"
#include "graphics/modelentity.h"
#include "graphics/globallightentity.h"
#include "graphics/lightprobeentity.h"

//------------------------------------------------------------------------------
namespace Lighting
{
class LightServerBase : public Core::RefCounted
{
    __DeclareClass(LightServerBase);
    __DeclareSingleton(LightServerBase);
public:
    /// constructor
    LightServerBase();
    /// destructor
    virtual ~LightServerBase();
    
    /// open the light server
    void Open();
    /// close the light server
    void Close();
    /// return true if light server is open
    bool IsOpen() const;
    /// indicate whether the light server requires light/model linking
    bool NeedsLightModelLinking() const;

    /// begin lighting frame
    void BeginFrame(const Ptr<Graphics::CameraEntity>& cameraEntity);
    /// begin attaching visible light sources
    void BeginAttachVisibleLights();
    /// attach a visible light source
    void AttachVisibleLight(const Ptr<Graphics::AbstractLightEntity>& lightEntity);
	/// attach a visible light probe
	void AttachVisibleLightProbe(const Ptr<Graphics::LightProbeEntity>& lightProbe);
    /// end attaching visible light sources
    void EndAttachVisibleLights();
    /// apply lighting parameters for a visible model entity 
    void ApplyModelEntityLights(const Ptr<Graphics::ModelEntity>& modelEntity);
    /// render light pass
    void RenderLights();
	/// render light probes
	void RenderLightProbes();
    /// end lighting frame
    void EndFrame();

	/// get global light entity
	const Ptr<Graphics::GlobalLightEntity>& GetGlobalLight();

protected:
    bool isOpen;
    bool inBeginFrame;
    bool inBeginAttach;
    Ptr<Graphics::CameraEntity> cameraEntity;
    Ptr<Graphics::GlobalLightEntity> globalLightEntity;
    Util::Array<Ptr<Graphics::AbstractLightEntity> > visibleLightEntities;
	Util::Array<Ptr<Graphics::LightProbeEntity> > visibleLightProbes;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
LightServerBase::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<Graphics::GlobalLightEntity>& 
LightServerBase::GetGlobalLight()
{
	return this->globalLightEntity;
}

} // namespace Lighting
//------------------------------------------------------------------------------

    