#include "application/stdneb.h"
#include "orientation.h"
#include "util/stringatom.h"
#include "memdb/attributeregistry.h"
#include "game/componentserialization.h"
#include "game/componentinspection.h"
#include "pjson/pjson.h"
//------------------------------------------------------------------------------
namespace IO
{
template<> void JsonReader::Get<Game::Orientation>(Game::Orientation& ret, const char* attr)
{
    ret = Game::Orientation();
    const pjson::value_variant* node = this->GetChild(attr);
    if (node->is_object())
    {
        this->SetToNode(attr);
        if (this->HasAttr("x")) this->Get<float>(ret.x, "x");
        if (this->HasAttr("y")) this->Get<float>(ret.y, "y");
        if (this->HasAttr("z")) this->Get<float>(ret.z, "z");
        if (this->HasAttr("w")) this->Get<float>(ret.w, "w");
        this->SetToParent();
    }
    else if (node->is_array())
    {
        this->Get<Math::quat>(ret, attr);
    }
}

template<> void JsonWriter::Add<Game::Orientation>(Game::Orientation const& value, Util::String const& attr)
{
    this->Add<Math::quat>(value, attr);
}
} // namespace IO
