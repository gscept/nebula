#pragma once
//------------------------------------------------------------------------------
/**
    @class GraphicsFeature::TerrainSession
    One terrain being edited: the Terrain component of an entity in a scene, the terrain file it
    names and the maps that file points to.

    The maps are kept on the CPU, one texel per meter of the world: the heightmap, normalized from
    the terrain's lowest to its highest point, a mask per biome and a mask per plant. A brush
    changes them there, and the texels it touched are uploaded to the textures the renderer draws
    the terrain with. That is what makes a stroke undoable, a ray can find the ground without
    reading the GPU back, and saving writes images from what is already in memory.

    The renderer's terrain belongs to the sim world. When the component is set up again, by an undo
    or a scene switch, the session builds the terrain again from what it holds, so the edits stay
    on screen. The structure of the terrain, its size, biomes and plants, is applied the same way.

    Saving writes the maps as images to the source folder, imports them as textures next to the
    terrain file, writes the terrain file and batches them all. The component names the terrain
    file; one that names none gets the file the session saves to.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#if WITH_NEBULA_EDITOR
#include "core/refcounted.h"
#include "game/entity.h"
#include "graphics/graphicsentity.h"
#include "coregraphics/texture.h"
#include "math/line.h"
#include "math/rectangle.h"
#include "math/vec3.h"
#include "util/fixedarray.h"
#include "util/guid.h"
#include "util/string.h"
#include "ids/id.h"
#include "resources/resourceid.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/render/terrainresource.h"

namespace Editor
{
class Scene;
}

namespace GraphicsFeature
{

/// what a brush is set to
struct TerrainBrush
{
    enum Tool
    {
        Raise,
        Lower,
        Flatten,
        Smooth,
        Paint,
        Erase,
        NumTools
    };
    enum Shape
    {
        Circle,
        Square,
    };
    int tool = Raise;
    int shape = Circle;
    /// half the width of the brush, in meters
    float radius = 16.0f;
    /// the part of the radius at full strength, the rest falls off
    float hardness = 0.5f;
    /// how fast the brush works: meters a second for raise and lower, the share of the way to the
    /// target a second for the others
    float strength = 0.5f;
    /// for raise and lower, meters a second at full strength
    float rate = 8.0f;
};

class TerrainSession : public Core::RefCounted
{
    __DeclareClass(TerrainSession)
public:
    /// which map a brush paints, the heights or a mask
    enum class Map
    {
        Height,
        Biome,
        Plant
    };

    TerrainSession();
    virtual ~TerrainSession();

    /// start editing the Terrain of an entity. Reads its terrain file, the work file when it has
    /// one, and the maps it names
    bool Open(Editor::Scene* scene, Game::Entity entity);
    /// stop editing, the textures are let go. With restore the renderer's terrain is set up from
    /// its file again, which needs the scene to still be there
    void Close(bool restore = true);
    /// true while open
    bool IsOpen() const;

    /// once a frame: find the renderer's terrain, build it again when it is new, upload what was
    /// painted. Returns false when the entity or its Terrain is gone
    bool Update();

    /// the scene and the entity
    Editor::Scene* GetScene() const;
    Game::Entity GetEntity() const;
    const Util::Guid& GetEntityGuid() const;
    /// the renderer's terrain, invalid before the first update found it
    Graphics::GraphicsEntityId GetGraphicsEntity() const;

    /// the terrain file the component names, empty when it names none yet
    const Util::String& GetFile() const;
    /// the file Save writes to when the component names none
    Util::String SuggestFile() const;

    /// what the terrain file holds, as edited. A change to it shows after ApplyStructure
    Render::TerrainResourceT& GetResource();
    /// build the renderer's terrain again from the resource: its size, biomes and plants. A change
    /// of the world size resamples the maps
    void ApplyStructure();
    /// the biome rules and layers and the plant settings, which don't need the terrain rebuilt
    void ApplyBiome(IndexT biome);
    void ApplyPlant(IndexT plant);
    /// add or remove a biome or a plant, with its mask
    void AddBiome();
    void RemoveBiome(IndexT biome);
    void AddPlant();
    void RemovePlant(IndexT plant);

    /// the map texture of a mask or the heightmap, for a preview
    CoreGraphics::TextureId GetMapTexture(Map map, IndexT index) const;
    /// map size in texels, which is the world size in meters
    int GetMapWidth() const;
    int GetMapHeight() const;

    /// the ground under a point of the world, false outside the terrain
    bool HeightAt(float x, float z, float& outHeight) const;
    /// where a ray meets the ground. False when it misses
    bool Raycast(const Math::line& ray, Math::vec3& outHit) const;

    /// start a stroke on a map. The stroke is one undo step
    void BeginStroke(Map map, IndexT index);
    /// one frame of the brush at a point of the world
    void Stroke(const TerrainBrush& brush, const Math::vec3& center, float seconds);
    /// end the stroke and put it on the scene's undo stack
    void EndStroke();
    /// true while a stroke is going on
    bool IsStroking() const;

    /// put texels into a map, what an undo of a stroke does
    void WriteRegion(Map map, IndexT index, const Math::rectangle<int>& region, const Util::FixedArray<float>& texels);

    /// true when there are changes the files don't have
    bool IsDirty() const;
    /// mark a change the panel made to the resource
    void MarkDirty();
    /// write the maps and the terrain file and batch them. The file is the component's, or the one
    /// given when it names none, which the component then gets. False and why when it failed
    bool Save(const Util::String& file, Util::String& outError);

private:
    struct EditMap
    {
        Util::FixedArray<float> texels;
        CoreGraphics::TextureId texture = CoreGraphics::InvalidTextureId;
        Math::rectangle<int> dirty;
        bool hasDirty = false;
    };

    /// the map a reference names, null when there is none
    EditMap* GetMap(Map map, IndexT index);
    const EditMap* GetMap(Map map, IndexT index) const;
    /// make the texture of a map from its texels
    void CreateMapTexture(EditMap& map, bool height, const char* name);
    /// let the texture go
    static void DestroyMapTexture(EditMap& map);
    /// read a map from a texture the file names, resampled to the map size
    void LoadMap(EditMap& map, const Util::String& texture, float fallback, bool height, const char* name);
    /// change the map size, the maps are resampled
    void Resize(int width, int height);
    /// mark texels of a map to upload
    static void Touch(EditMap& map, const Math::rectangle<int>& region);
    /// upload what was touched
    static void Upload(EditMap& map, bool height);
    /// build the renderer's terrain from what the session holds
    void Rebuild();
    /// the masks and the heightmap the renderer reads are the session's. A load the renderer
    /// started can replace them, so this is done every frame
    void AttachMaps();
    /// write one map as an image and import it as a texture. Returns the texture's URN
    bool SaveMap(const EditMap& map, bool height, const Util::String& folder, const Util::String& name, Util::String& outUrn, Util::String& outError);

    Editor::Scene* scene;
    Game::Entity entity;
    Util::Guid entityGuid;
    Graphics::GraphicsEntityId graphicsEntity;
    Util::String file;
    Render::TerrainResourceT resource;

    int width, height;
    EditMap heights;
    Util::Array<EditMap> biomes;
    Util::Array<EditMap> plants;

    struct StrokeState
    {
        bool active = false;
        Map map = Map::Height;
        IndexT index = 0;
        Util::FixedArray<float> before;
        Math::rectangle<int> region;
        bool touched = false;
        float flattenTarget = 0.0f;
        bool hasFlattenTarget = false;
    } stroke;

    bool open;
    bool dirty;
    bool terrainChanged;
};

} // namespace GraphicsFeature
#endif // WITH_NEBULA_EDITOR
//------------------------------------------------------------------------------
