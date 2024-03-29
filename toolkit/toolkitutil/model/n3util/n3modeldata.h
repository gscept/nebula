#pragma once
//------------------------------------------------------------------------------
/**
    Collection of different data types used to represent a model node
*/
#include "util/stringatom.h"
#include "util/variant.h"
#include "util/string.h"
#include "util/keyvaluepair.h"

namespace ToolkitUtil
{
struct Joint
{
    Util::String name;
    Math::mat4 bind;
    Math::vec3 translation, scale;
    Math::quat rotation;
    int parent;
    int index;
};

struct JointMask
{
    Util::String name;
    Util::FixedArray<float> weights;

    bool operator==(const JointMask& rhs)
    {
        return this->name == rhs.name;
    }
};

struct Variable
{
    // default constructor
    Variable()
    {
        // empty
    }

    // constructor
    Variable(const Util::String& varName, const Util::Variant& varVal)
    {
        this->variableName = varName;
        this->variableValue = varVal;
        this->limits = Util::KeyValuePair<Util::Variant, Util::Variant>(varVal, varVal);
    }

    // equality operator
    bool operator==(const Variable& other)
    {
        return (variableName == other.variableName);
    }

    Util::KeyValuePair<Util::Variant, Util::Variant> limits;
    Util::String variableName;
    Util::Variant variableValue;
};

struct Texture
{
    Texture()
    {
        // empty, needed for arrays
    }
    Texture(const Util::String& varName, const Util::String& varVal)
    {
        this->textureName = varName;
        this->textureResource = varVal;
    }

    // equality operator
    bool operator==(const Texture& other)
    {
        return (textureName == other.textureName);
    }
    Util::String textureName;
    Util::String textureResource;
};

struct Transform
{
    Transform()
    {
        this->position = Math::point(0,0,0);
        this->rotation = Math::quat(0,0,0,1);
        this->scale = Math::vector::onevec();
        this->scalePivot = Math::point(0,0,0);
        this->rotatePivot = Math::point(0,0,0);
    }
    Transform(const Transform& rhs) = default;
    Math::transform44 GetTransform44() const;
    Math::point position;
    Math::quat rotation;
    Math::vector scale;
    Math::point scalePivot;
    Math::point rotatePivot;
};

//------------------------------------------------------------------------------
/**
*/
inline Math::transform44
Transform::GetTransform44() const
{
    Math::transform44 t;
    t.setposition(position);
    t.setrotate(Math::quatyawpitchroll(rotation.y, rotation.x, rotation.z));
    t.setscale(scale);
    t.setrotatepivot(rotatePivot.vec);
    t.setscalepivot(scalePivot.vec);
    return t;
}

struct State
{
    Util::String material;

    void Clear()
    {
        this->material.Clear();
    }
};
}