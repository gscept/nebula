#pragma once
//------------------------------------------------------------------------------
/**
    class Lighting::ShadowServerBase
    
    The ShadowServer setups and controls the global aspects of the dynamic
    shadow system.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"

    
//------------------------------------------------------------------------------
namespace Lighting
{
class ShadowServerBase : public Core::RefCounted
{
    __DeclareClass(ShadowServerBase);
    __DeclareSingleton(ShadowServerBase);
public:
    /// constructor
    ShadowServerBase();
    /// destructor
    virtual ~ShadowServerBase();

    /// open the shadow server
    void Open();
    /// close the shadow server
    void Close();
    /// return true if shadow server is open
    bool IsOpen() const;

    /// begin shadow frame
    void BeginFrame(const Ptr<Graphics::CameraEntity>& cameraEntity);
    /// begin attaching visible shadow casting light sources
    void BeginAttachVisibleLights();
    /// attach a visible shadow casting light source
    void AttachVisibleLight(const Ptr<Graphics::AbstractLightEntity>& lightEntity);
    /// end attaching visible shadow casting light sources
    void EndAttachVisibleLights();
    /// update shadow buffer
    void UpdateShadowBuffers();
	/// update spot light shadow buffers
	void UpdateSpotLightShadowBuffers();
	/// update point light shadow buffers
	void UpdatePointLightShadowBuffers();
	/// update global light shadow buffers
	void UpdateGlobalLightShadowBuffers();
    /// end lighting frame
    void EndFrame();      
    /// set PointOfInterest
    void SetPointOfInterest(const Math::point& val);

	/// gets array of global light split distances
	virtual const float* GetSplitDistances() const;
	/// gets array of global light split transforms
	virtual const Math::matrix44* GetSplitTransforms() const;
	/// gets global ligth shadow view
	virtual const Math::matrix44* GetShadowView() const;
	/// gets array of far plane global light frustum corners
	virtual const Math::float4* GetFarPlane() const;
	/// gets array of near plane global light frustum corners
	virtual const Math::float4* GetNearPlane() const;

    static const int MaxNumShadowSpotLights = 16;
	static const int MaxNumShadowPointLights = 4;
	static const int SplitsPerRow = 2;
	static const int SplitsPerColumn = 2;
	static const int ShadowLightsPerRow = 4;
	static const int ShadowLightsPerColumn = 4;

    static const int ShadowAtlasBorderPixels = 32;
    
protected:
    /// sort local lights by priority
    virtual void SortLights();

    bool isOpen;
    bool inBeginFrame;
    bool inBeginAttach;
    Ptr<Graphics::CameraEntity> cameraEntity;
    Ptr<Graphics::GlobalLightEntity> globalLightEntity;
    Util::Array<Ptr<Graphics::SpotLightEntity> > spotLightEntities;
	Util::Array<Ptr<Graphics::PointLightEntity> > pointLightEntities;
    Math::point pointOfInterest;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
ShadowServerBase::IsOpen() const
{
    return this->isOpen;
}
//------------------------------------------------------------------------------
/**
*/
inline void 
ShadowServerBase::SetPointOfInterest(const Math::point& val)
{
    this->pointOfInterest = val;
}
} // namespace Lighting
//------------------------------------------------------------------------------

