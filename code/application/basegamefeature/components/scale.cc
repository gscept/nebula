#include "application/stdneb.h"
#include "scale.h"
#include "util/stringatom.h"
#include "memdb/attributeregistry.h"
#include "game/componentserialization.h"
#include "game/componentinspection.h"
#include "pjson/pjson.h"
//------------------------------------------------------------------------------
namespace IO
{
template<> void JsonReader::Get<Game::Scale>(Game::Scale& ret, const char* attr)
{
    ret = Game::Scale();
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

template<> void JsonWriter::Add<Game::Scale>(Game::Scale const& value, Util::String const& attr)
{
    this->Add<Math::vec3>(value, attr);
}
} // namespace IO
