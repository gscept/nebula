#pragma once
//------------------------------------------------------------------------------
/**
    The transform node contains just a hierarchical transform
    
    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "modelnode.h"
#include "math/mat4.h"
#include "math/transform44.h"
#include "math/quat.h"
namespace Models
{
class TransformNode : public ModelNode
{
public:
    /// constructor
    TransformNode();
    /// destructor
    virtual ~TransformNode();

    /// Get LOD distances
    void GetLODDistances(float& minDistance, float& maxDistance);

protected:
    friend class StreamModelCache;
    friend class ModelContext;

    /// load transform
    virtual bool Load(const Util::FourCC& fourcc, const Util::StringAtom& tag, const Ptr<IO::BinaryReader>& reader, bool immediate) override;

    Math::vec3 position;
    Math::quat rotate;
    Math::vec3 scale;
    Math::vec3 rotatePivot;
    Math::vec3 scalePivot;
    bool isInViewSpace;
    float minDistance;
    float maxDistance;
    bool useLodDistances;
    bool lockedToViewer;
};

} // namespace Models
