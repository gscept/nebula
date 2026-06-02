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

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/scene.h"
#include "flat/physics.h"

namespace ToolkitUtil
{

class N3Writer;
class SceneWriter
{
public:

    /// Output graphics model asset
    static std::unique_ptr<SceneResourceT> GenerateGraphicsModel(
        const Util::String& basePath
        , const Scene* scene
        , const Platform::Code platform
        , const Util::Array<SceneNode*>& graphicsNodes
        , const ToolkitUtil::ImportFlags& flags
    );

    /// Output physics model asset
    static std::unique_ptr<PhysicsResourceT> GeneratePhysicsModel(
        const Util::String& basePath
        , const Scene* scene
        , const Platform::Code platform
        , const Util::Array<SceneNode*>& physicsNodes
        , const ToolkitUtil::ImportFlags& flags
    );
};

} // namespace ToolkitUtil
