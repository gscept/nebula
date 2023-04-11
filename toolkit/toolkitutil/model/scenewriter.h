#pragma once
//------------------------------------------------------------------------------
/**
    Base writer for scenes

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/platform.h"
#include "model/import/base/scene.h"
namespace ToolkitUtil
{

class N3Writer;
class ModelAttributes;
class ModelPhysics;
class ModelConstants;
class SceneWriter
{
public:

    /// uses FbxScene to write models
    static void GenerateModels(
        const Util::String& basePath
        , const Scene* scene
        , const Platform::Code platform
        , const Util::Array<SceneNode*>& graphicsNodes
        , const Util::Array<SceneNode*>& physicsNodes
        , const Util::Array<SceneNode*>& characterNodes
        , const Util::String& physicsMeshResource
        , const ToolkitUtil::ExportFlags& flags
    );
private:

    /// Create model
    static void CreateModel(
        const Util::String& file
        , const Scene* scene
        , const Util::String& category
        , const Ptr<ModelConstants>& constants
        , const Ptr<ModelAttributes>& attributes
        , const Ptr<ModelPhysics>& physics
        , const Util::Array<SceneNode*>& graphicsNodes
        , const Util::Array<SceneNode*>& physicsNodes
        , const Util::Array<SceneNode*>& characterNodes
    );

    /// convenience function for writing constants-files
    static void UpdateConstants(const Util::String& file, const Ptr<ToolkitUtil::ModelConstants>& constants);
    /// convenience function for writing attributes-files
    static void UpdateAttributes(const Util::String& file, const Ptr<ToolkitUtil::ModelAttributes>& attributes);
    /// convenience function for writing physics-files
    static void UpdatePhysics(const Util::String& file, const Ptr<ToolkitUtil::ModelPhysics>& physics);
};

} // namespace ToolkitUtil
