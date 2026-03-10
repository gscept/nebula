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
#include "coregraphics/texture.h"
#include <array>
//#include <render/system_shaders/lights_cluster.h>
#include "gpulang/render/system_shaders/lights_cluster.h"


namespace Frame
{
class FrameScript;
};

namespace Lighting
{
class LightContext : public Graphics::GraphicsContext
{
    __DeclareContext();
public:

    enum class LightType
    {
        DirectionalLightType,
        PointLightType,
        SpotLightType,
        AreaLightType
    };

    enum class AreaLightShape
    {
        Disk,
        Rectangle,
        Tube
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
    static void SetupDirectionalLight(
        const Graphics::GraphicsEntityId id
        , const Graphics::ViewId view // Camera used for shadows
        , const Math::vec3& color
        , const float intensity
        , const Math::vec3& ambient
        , const float zenith
        , const float azimuth
        , const Graphics::StageMask stageMask = 0xFFFF
        , bool castShadows = false
    );
    /// Setup entity as point light source
    static void SetupPointLight(
        const Graphics::GraphicsEntityId id
        , const Math::vec3& color
        , const float intensity
        , const float range
        , const Graphics::StageMask stageMask = Graphics::PRIMARY_STAGE_MASK
        , bool castShadows = false
        , const CoreGraphics::TextureId projection = CoreGraphics::InvalidTextureId
    );

    /// Setup entity as spot light
    static void SetupSpotLight(
        const Graphics::GraphicsEntityId id
        , const Math::vec3& color
        , const float intensity
        , const float innerConeAngle
        , const float outerConeAngle
        , const float range
        , const Graphics::StageMask stageMask = Graphics::PRIMARY_STAGE_MASK
        , bool castShadows = false
        , const CoreGraphics::TextureId projection = CoreGraphics::InvalidTextureId
    );

    /// Setup entity as area light
    static void SetupAreaLight(
        const Graphics::GraphicsEntityId id
        , const AreaLightShape shape
        , const Math::vec3& color
        , const float intensity
        , const float range
        , const Graphics::StageMask stageMask = Graphics::PRIMARY_STAGE_MASK
        , bool twoSided = false
        , bool castShadows = false
        , bool renderMesh = false
    );

    /// set color of light
    static void SetColor(const Graphics::GraphicsEntityId id, const Math::vec3& color);
    ///
    static Math::vec3 GetColor(const Graphics::GraphicsEntityId id);
    /// set range of light
    static void SetRange(const Graphics::GraphicsEntityId id, const float range);
    /// set intensity of light
    static void SetIntensity(const Graphics::GraphicsEntityId id, const float intensity);
    ///
    static float GetIntensity(const Graphics::GraphicsEntityId id);
    /// Set transform as angles
    static void SetTransform(const Graphics::GraphicsEntityId id, const float azimuth, const float zenith);
    /// get transform
    static const Math::mat4 GetTransform(const Graphics::GraphicsEntityId id);
    /// get the view transform including projections
    static const Math::mat4 GetObserverTransform(const Graphics::GraphicsEntityId id);

    /// Set light position
    static const void SetPosition(const Graphics::GraphicsEntityId id, const Math::point& position);
    /// Get light position
    static const Math::point GetPosition(const Graphics::GraphicsEntityId id);
    /// Set light rotation
    static const void SetRotation(const Graphics::GraphicsEntityId id, const Math::quat& rotation);
    /// Get light rotation
    static const Math::quat GetRotation(const Graphics::GraphicsEntityId id);
    /// Set light scale
    static const void SetScale(const Graphics::GraphicsEntityId id, const Math::vec3& scale);
    /// Get light scale
    static const Math::vec3 GetScale(const Graphics::GraphicsEntityId id);

    /// 
    static Math::vec3 GetAmbient(const Graphics::GraphicsEntityId id);
    ///
    static void SetAmbient(const Graphics::GraphicsEntityId id, Math::vec3& ambient);

    /// get the light type
    static LightType GetType(const Graphics::GraphicsEntityId id);

    /// get inner and outer angle for spotlights
    static void GetInnerOuterAngle(const Graphics::GraphicsEntityId id, float& inner, float& outer);
    /// set inner and outer angle for spotlights
    static void SetInnerOuterAngle(const Graphics::GraphicsEntityId id, float inner, float outer);

    /// prepare light visibility
    static void OnPrepareView(const Graphics::ViewId view, const Graphics::FrameContext& ctx);

    /// prepare light lists
    static void UpdateLights(const Graphics::FrameContext& ctx);
    /// run framescript when visibility is done
    static void RunFrameScriptJobs(const Graphics::FrameContext& ctx);
    /// React to window resize event
    static void Resize(const uint framescriptHash, SizeT width, SizeT height);
#ifndef PUBLIC_BUILD
    /// render debug
    static void OnRenderDebug(uint32_t flags);
#endif

    /// Setup terrain shadows
    static void SetupTerrainShadows(const CoreGraphics::TextureId terrainShadowMap, const uint worldSize);

    /// get light index lists buffer
    static const CoreGraphics::BufferId GetLightIndexBuffer();
    /// get light lists buffer
    static const CoreGraphics::BufferId GetLightsBuffer();
    /// get light uniforms
    static const LightsCluster::LightUniforms::STRUCT& GetLightUniforms();
    
private:

    /// Set global light transform
    static void SetGlobalLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform, const Math::vector& direction);
    /// Set global light shadow transform
    static void SetGlobalLightViewProjTransform(const Graphics::ContextEntityId id, const Math::mat4& transform);


    enum
    {
        Light_Entity,
        Light_Type,
        Light_Color,
        Light_Intensity,
        Light_ShadowCaster,
        Light_ShadowTile,
        Light_Range,
        Light_TypedLightId,
        Light_StageMask
    };

    typedef Ids::IdAllocator<
        Graphics::GraphicsEntityId,     // entity
        LightType,                      // type
        Math::vec3,                     // color
        float,                          // intensity
        bool,                           // shadow caster
        Math::rectangle<int>,           // shadow rendering tile
        float,                          // range
        Ids::Id32,                      // typed light id (index into pointlights, spotlights and globallights)
        Graphics::StageMask
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
        Math::transform44,          // transform
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
        Math::transform44,          // transform
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
        AreaLight_Transform,
        AreaLight_Shape,
        AreaLight_ConstantBufferSet,
        AreaLight_ShadowConstantBufferSet,
        AreaLight_DynamicOffsets,
        AreaLight_TwoSided,
        AreaLight_Observer,
        AreaLight_RenderMesh,
    };

    typedef Ids::IdAllocator<
        Math::transform44,          // transform
        AreaLightShape,             // shape of area light
        ConstantBufferSet,          // constant buffer binding for light
        ConstantBufferSet,          // constant buffer binding for shadows
        Util::FixedArray<uint>,     // dynamic offsets
        bool,                       // two sides
        Graphics::GraphicsEntityId, // graphics entity used for observer stuff
        bool                        // render mesh as well
    > AreaLightAllocator;
    static AreaLightAllocator areaLightAllocator;

    enum
    {
        DirectionalLight_View,
        DirectionalLight_Direction,
        DirectionalLight_Ambient,
        DirectionalLight_Transform,
        DirectionalLight_ViewProjTransform,
        DirectionalLight_CascadeObservers,
        DirectionalLight_CascadeTiles
    };

    struct ShadowData
    {
        CoreGraphics::TextureId shadowMap;
        CoreGraphics::TextureViewId shadowView;
        CoreGraphics::PassId shadowPass;
    };

    typedef Ids::IdAllocator<
        Graphics::ViewId,                           // camera used for shadow mapping
        Math::vector,                               // direction
        Math::vec3,                                 // ambient
        Math::mat4,                                 // transform (basically just a rotation in the direction)
        Math::mat4,                                 // transform for visibility and such
        Util::Array<Graphics::GraphicsEntityId>,    // view ids for cascades
        Util::Array<Math::rectangle<int>>           // cascade shadow tiles
    > DirectionalLightAllocator;
    static DirectionalLightAllocator directionalLightAllocator;


    enum
    {
        ShadowCaster_Transform
    };

    typedef Ids::IdAllocator<
        Math::mat4
    > ShadowCasterAllocator;
    
    static ShadowCasterAllocator shadowCasterAllocator;
    static Util::HashTable<Graphics::GraphicsEntityId, uint, 16, 1> shadowCasterSliceMap;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};
} // namespace Lighting
