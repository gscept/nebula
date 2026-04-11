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

    struct DirectionalLightSetupInfo
    {
        Graphics::ViewId view;
        Math::vec3 color = Math::vec3(1);
        float intensity = 50.0f;
        float zenith = 45_rad, azimuth = 0;
        Graphics::StageMask stageMask = 0xFFFF;
        bool castShadows = false;
    };

    /// setup entity as global light
    static void SetupDirectionalLight(const Graphics::GraphicsEntityId id, const DirectionalLightSetupInfo& info);

    struct PointLightSetupInfo
    {
        Math::vec3 color = Math::vec3(1);
        float intensity = 1.0f;
        float range = 10.0f;
        Graphics::StageMask stageMask = 0xFFFF;
        bool castShadows = false;
        CoreGraphics::TextureId projection = CoreGraphics::InvalidTextureId;
    };
    /// Setup entity as point light source
    static void SetupPointLight(const Graphics::GraphicsEntityId id, const PointLightSetupInfo& info);

    struct SpotLightSetupInfo
    {
        Math::vec3 color = Math::vec3(1);
        float intensity = 1.0f;
        float innerConeAngle = 30_rad;
        float outerConeAngle = 45_rad;
        float range = 10.0f;
        Graphics::StageMask stageMask = 0xFFFF;
        bool castShadows = false;
        CoreGraphics::TextureId projection = CoreGraphics::InvalidTextureId;
    };

    /// Setup entity as spot light
    static void SetupSpotLight(
        const Graphics::GraphicsEntityId id
        , const SpotLightSetupInfo& info
    );

    struct AreaLightSetupInfo
    {   
        AreaLightShape shape;
        Math::vec3 color = Math::vec3(1);
        float intensity = 1.0f;
        float range = 10.0f;
        Graphics::StageMask stageMask = 0xFFFF;
        bool twoSided = false;
        bool castShadows = false;
        bool renderMesh = false;
    };
    /// Setup entity as area light
    static void SetupAreaLight(const Graphics::GraphicsEntityId id, const AreaLightSetupInfo& info);

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
    static void SetDirectionalLightTransform(const Graphics::ContextEntityId id, const Math::mat4& transform, const Math::vector& direction);

    enum
    {
        Light_Entity,
        Light_Type,
        Light_Color,
        Light_Intensity,
        Light_ShadowCaster,
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
        PointLight_ProjectionTexture,
        PointLight_Observers,
        PointLight_ShadowTiles,
        PointLight_ShadowProjectionTransforms
    };

    typedef Ids::IdAllocator<
        Math::transform44,                              // transform
        CoreGraphics::TextureId,                        // projection (if invalid, don't use)
        std::array<Graphics::GraphicsEntityId, 6>,      // graphics entity used for observer stuff
        std::array<Math::rectangle<int>, 6>,            // cascade shadow tiles
        std::array<Math::mat4, 6>                       // cascade shadow tiles
    > PointLightAllocator;
    static PointLightAllocator pointLightAllocator;

    enum
    {
        SpotLight_Transform,
        SpotLight_ConeAngles,
        SpotLight_ProjectionTexture,
        SpotLight_ProjectionTransform,
        SpotLight_ShadowProjectionTransform,
        SpotLight_ShadowTile,
        SpotLight_Observer
    };

    typedef Ids::IdAllocator<
        Math::transform44,          // transform
        std::array<float, 2>,       // cone angle
        CoreGraphics::TextureId,    // projection (if invalid, don't use)
        Math::mat4,                 // projection matrix
        Math::mat4,                 // shadow projection transform
        Math::rectangle<int>,       // shadow tile          
        Graphics::GraphicsEntityId  // graphics entity used for observer stuff
    > SpotLightAllocator;
    static SpotLightAllocator spotLightAllocator;

    enum
    {
        AreaLight_Transform,
        AreaLight_Shape,
        AreaLight_TwoSided,
        AreaLight_Observers,
        AreaLight_ShadowProjectionTransforms,
        AreaLight_ShadowTiles,
        AreaLight_RenderMesh,
    };

    typedef Ids::IdAllocator<
        Math::transform44,                                  // transform
        AreaLightShape,                                     // shape of area light
        bool,                                               // two sides
        std::array<Graphics::GraphicsEntityId, 2>,          // graphics entity used for observer stuff
        std::array<Math::mat4, 2>,                          // shadow rendering projection transforms for front and back
        std::array<Math::rectangle<int>, 2>,                // shadow rendering tiles for front and back
        bool                                                // render mesh as well
    > AreaLightAllocator;
    static AreaLightAllocator areaLightAllocator;

    enum
    {
        DirectionalLight_View,
        DirectionalLight_Direction,
        DirectionalLight_Transform,
        DirectionalLight_CascadeObservers,
        DirectionalLight_CascadeTiles,
        DirectionalLight_CascadeDistances,
        DirectionalLight_CascadeTransforms,
    };

    struct ShadowData
    {
        CoreGraphics::TextureId shadowMap;
        CoreGraphics::TextureViewId shadowView;
        CoreGraphics::PassId shadowPass;
    };

    typedef Ids::IdAllocator<
        Graphics::ViewId,                               // camera used for shadow mapping
        Math::vector,                                   // direction
        Math::mat4,                                     // transform (basically just a rotation in the direction)
        Util::FixedArray<Graphics::GraphicsEntityId>,   // view ids for cascades
        Util::FixedArray<Math::rectangle<int>>,         // cascade shadow tiles
        Util::FixedArray<float>,                        // cascade distances       
        Util::FixedArray<Math::mat4>                    // cascade transforms
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
    static Util::HashTable<Graphics::GraphicsEntityId, uint, 16, 1> shadowCasterIndexMap;

    /// allocate a new slice for this context
    static Graphics::ContextEntityId Alloc();
    /// deallocate a slice
    static void Dealloc(Graphics::ContextEntityId id);
};
} // namespace Lighting
