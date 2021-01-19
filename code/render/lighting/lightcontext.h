#pragma once
//------------------------------------------------------------------------------
/**
    Adds a light representation to the graphics entity
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "coregraphics/shader.h"
#include "coregraphics/buffer.h"
#include <array>

namespace Lighting
{
class LightContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    enum LightType
    {
        GlobalLightType,
        PointLightType,
        SpotLightType,
        AreaLightType
    };

    /// constructor
    LightContext();
    /// destructor
    virtual ~LightContext();

    /// setup light context
    static void Create();
    /// discard light context
    static void Discard();

    /// setup entity as global light
    static void SetupGlobalLight(const Graphics::GraphicsEntityId id, const Math::vec3& color, const float intensity, const Math::vec3& ambient, const Math::vec3& backlight, const float backlightFactor, const Math::vector& direction, bool castShadows = false);
    /// setup entity as point light source
    static void SetupPointLight(const Graphics::GraphicsEntityId id, 
        const Math::vec3& color,
        const float intensity, 
        const Math::mat4& transform,
        const float range, 
        bool castShadows = false, 
        const CoreGraphics::TextureId projection = CoreGraphics::InvalidTextureId);

    /// setup entity as spot light
    static void SetupSpotLight(const Graphics::GraphicsEntityId id, 
        const Math::vec3& color,
        const float intensity, 
        const float innerConeAngle,
        const float outerConeAngle,
        const Math::mat4& transform,
        const float range,
        bool castShadows = false, 
        const CoreGraphics::TextureId projection = CoreGraphics::InvalidTextureId);

    /// set color of light
    static void SetColor(const Graphics::GraphicsEntityId id, const Math::vec3& color);
    /// set range of light
    static void SetRange(const Graphics::GraphicsEntityId id, const float range);
    /// set intensity of light
    static void SetIntensity(const Graphics::GraphicsEntityId id, const float intensity);
    /// get transform
    static const Math::mat4 GetTransform(const Graphics::GraphicsEntityId id);
    /// set transform depending on type
    static void SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& transform);
    /// get the view transform including projections
    static const Math::mat4 GetObserverTransform(const Graphics::GraphicsEntityId id);

    /// get the light type
    static LightType GetType(const Graphics::GraphicsEntityId id);

    /// get inner and outer angle for spotlights
    static void GetInnerOuterAngle(const Graphics::GraphicsEntityId id, float& inner, float& outer);
    /// set inner and outer angle for spotlights
    static void SetInnerOuterAngle(const Graphics::GraphicsEntityId id, float inner, float outer);

    /// prepare light visibility
    static void OnPrepareView(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);

    /// prepare light lists
    static void UpdateViewDependentResources(const Ptr<Graphics::View>& view, const Graphics::FrameContext& ctx);
    /// run framescript when visibility is done
    static void RunFrameScriptJobs(const Graphics::FrameContext& ctx);
#ifndef PUBLIC_BUILD
    /// render debug
    static void OnRenderDebug(uint32_t flags);
#endif


    /// get lighting texture
    static const CoreGraphics::TextureId GetLightingTexture();
    /// get light index lists buffer
    static const CoreGraphics::BufferId GetLightIndexBuffer();
    /// get light lists buffer
    static const CoreGraphics::BufferId GetLightsBuffer();
private:

    /// set transform, type must match the type the entity was created with
    static void SetSpotLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform);
    /// set transform, type must match the type the entity was created with
    static void SetPointLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform);
    /// set global light transform
    static void SetGlobalLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform);
    /// set global light shadow transform
    static void SetGlobalLightViewProjTransform(const Graphics::ContextEntityId id, const Math::mat4& transform);

    /// run light classification compute
    static void CullAndClassify();
    /// run lighting combination pass
    static void CombineLighting();
    /// run shadow map blurring
    static void BlurGlobalShadowMap();

    /// update global shadows
    static void UpdateGlobalShadowMap();
    /// update spotlight shadows
    static void UpdateSpotShadows();
    /// update pointligt shadows
    static void UpdatePointShadows();

    enum
    {
        Type,
        Color,
        Intensity,
        ShadowCaster,
        Range,
        TypedLightId
    };

    typedef Ids::IdAllocator<
        LightType,              // type
        Math::vec3,             // color
        float,                  // intensity
        bool,                   // shadow caster
        float,
        Ids::Id32               // typed light id (index into pointlights, spotlights and globallights)
    > GenericLightAllocator;
    static GenericLightAllocator genericLightAllocator;

    struct ConstantBufferSet
    {
        uint offset, slice;
    };

    enum
    {
        PointLight_Transform,
        PointLight_ConstantBufferSet,
        PointLight_ShadowConstantBufferSet,
        PointLight_DynamicOffsets,
        PointLight_ProjectionTexture,
        PointLight_Observer
    };

    typedef Ids::IdAllocator<
        Math::mat4,                 // transform
        ConstantBufferSet,          // constant buffer binding for light
        ConstantBufferSet,          // constant buffer binding for shadows
        Util::FixedArray<uint>,     // dynamic offsets
        CoreGraphics::TextureId,    // projection (if invalid, don't use)
        Graphics::GraphicsEntityId  // graphics entity used for observer stuff
    > PointLightAllocator;
    static PointLightAllocator pointLightAllocator;

    enum
    {
        SpotLight_Transform,
        SpotLight_ConstantBufferSet,
        SpotLight_ShadowConstantBufferSet,
        SpotLight_DynamicOffsets,
        SpotLight_ConeAngles,
        SpotLight_ProjectionTexture,
        SpotLight_ProjectionTransform,
        SpotLight_Observer
    };

    typedef Ids::IdAllocator<
        Math::mat4,                 // transform
        ConstantBufferSet,          // constant buffer binding for light
        ConstantBufferSet,          // constant buffer binding for shadows
        Util::FixedArray<uint>,     // dynamic offsets
        std::array<float, 2>,       // cone angle
        CoreGraphics::TextureId,    // projection (if invalid, don't use)
        Math::mat4,                 // projection matrix
        Graphics::GraphicsEntityId  // graphics entity used for observer stuff
    > SpotLightAllocator;
    static SpotLightAllocator spotLightAllocator;

    enum
    {
        GlobalLight_Direction,
        GlobalLight_Backlight,
        GlobalLight_BacklightOffset,
        GlobalLight_Ambient,
        GlobalLight_Transform,
        GlobalLight_ViewProjTransform,
        GlobalLight_CascadeObservers
    };
    typedef Ids::IdAllocator<
        Math::vector,                               // direction
        Math::vec3,                                 // backlight color
        float,                                      // backlight offset
        Math::vec3,                                 // ambient
        Math::mat4,                                 // transform (basically just a rotation in the direction)
        Math::mat4,                                 // transform for visibility and such
        Util::Array<Graphics::GraphicsEntityId>     // view ids for cascades
    > GlobalLightAllocator;
    static GlobalLightAllocator globalLightAllocator;


    enum
    {
        ShadowCaster_Transform
    };
    typedef Ids::IdAllocator<
        Math::mat4
    > ShadowCasterAllocator;
    static ShadowCasterAllocator shadowCasterAllocator;
    static Util::HashTable<Graphics::GraphicsEntityId, Graphics::ContextEntityId, 6, 1> shadowCasterSliceMap;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};
} // namespace Lighting