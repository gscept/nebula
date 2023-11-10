#include "application/stdneb.h"
#include "velocity.h"
#include "util/stringatom.h"
#include "memdb/attributeregistry.h"
#include "game/componentserialization.h"
#include "game/componentinspection.h"
#include "pjson/pjson.h"
//------------------------------------------------------------------------------
namespace IO
{
template<> void JsonReader::Get<Game::Velocity>(Game::Velocity& ret, const char* attr)
{
    ret = Game::Velocity();
    const pjson::value_variant* node = this->GetChild(attr);
    if (node->is_object())
    {
        this->SetToNode(attr);
        if (this->HasAttr("x")) this->Get<float>(ret.x, "x");
        if (this->HasAttr("y")) this->Get<float>(ret.y, "y");
        if (this->HasAttr("z")) this->Get<float>(ret.z, "z");
        this->SetToParent();
    }
    else if (node->is_array())
    {
        this->Get<Math::vec3>(ret, attr);
    }
}

template<> void JsonWriter::Add<Game::Velocity>(Game::Velocity const& value, Util::String const& attr)
{
    this->Add<Math::vec3>(value, attr);
}

template<> void JsonReader::Get<Game::AngularVelocity>(Game::AngularVelocity& ret, const char* attr)
{
    ret = Game::AngularVelocity();
    const pjson::value_variant* node = this->GetChild(attr);
    if (node->is_object())
    {
        this->SetToNode(attr);
        if (this->HasAttr("x")) this->Get<float>(ret.x, "x");
        if (this->HasAttr("y")) this->Get<float>(ret.y, "y");
        if (this->HasAttr("z")) this->Get<float>(ret.z, "z");
        this->SetToParent();
    }
    else if (node->is_array())
    {
        this->Get<Math::vec3>(ret, attr);
    }
}

template<> void JsonWriter::Add<Game::AngularVelocity>(Game::AngularVelocity const& value, Util::String const& attr)
{
    this->Add<Math::vec3>(value, attr);
}
} // namespace IO
