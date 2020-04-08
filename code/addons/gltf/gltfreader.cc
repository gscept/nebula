//------------------------------------------------------------------------------
/**
    @file gltf/gltfreader.cc

    GLTF parser based on jsonreader

    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
#include "foundation/stdneb.h"
#include "io/uri.h"
#include "gltfdata.h"
#include "io/jsonreader.h"
#include "io/ioserver.h"
#include "io/filestream.h"

// "forward" all current specialization for the json reader, so that we don't get collisions
#include "io/jsonreader.cc"

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "pjson/pjson.h"

//------------------------------------------------------------------------------
/** 
    some constexpr string switch magic
*/
constexpr unsigned int chash(const char * c, int h = 0)
{
    return !c[h] ? 5381 : (chash(c, h + 1) * 33) ^ c[h];
}

//------------------------------------------------------------------------------
/**
*/
void
ReadExtensionsAndExtras(Gltf::GltfBase& base, const pjson::value_variant * object)
{
    int extsIdx = object->find_key("extensions");
    if (extsIdx >= 0)
    {
        //FIXME overload this with native nebula type
        pjson::char_vec_t vec;
        object->get_value_at_index(extsIdx).serialize(vec);
        base.extensions = &vec[0];
    }
    int extrasIdx = object->find_key("extras");
    if (extrasIdx >= 0)
    {
        //FIXME overload this with native nebula type
        pjson::char_vec_t vec;
        object->get_value_at_index(extrasIdx).serialize(vec);
        base.extras = &vec[0];
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Accessor::Type>(Gltf::Accessor::Type & item, const char * key)
{
    Util::String type = this->GetString(key);    
    switch (chash(type.AsCharPtr()))
    {
        case chash("SCALAR"): item = Gltf::Accessor::Type::Scalar; break;
        case chash("VEC2"): item = Gltf::Accessor::Type::Vec2; break;
        case chash("VEC3"): item = Gltf::Accessor::Type::Vec3; break;
        case chash("VEC4"): item = Gltf::Accessor::Type::Vec4; break;
        case chash("MAT2"): item = Gltf::Accessor::Type::Mat2; break;
        case chash("MAT3"): item = Gltf::Accessor::Type::Mat3; break;
        case chash("MAT4"): item = Gltf::Accessor::Type::Mat4; break;
        default:
            n_assert("Unknown accessor type");
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Accessor::Sparse::Indices>(Gltf::Accessor::Sparse::Indices& item, const char * key)
{
    n_assert(this->SetToFirstChild(key));
    this->Get(item.bufferView, "bufferView");
    this->GetOpt(item.byteOffset, "byteOffset");
    item.componentType = static_cast<Gltf::Accessor::ComponentType>(this->GetInt("componentType"));
    ReadExtensionsAndExtras(item, this->curNode);
    this->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Accessor::Sparse::Values>(Gltf::Accessor::Sparse::Values& item, const char * key)
{
    n_assert(this->SetToFirstChild(key));
    this->Get(item.bufferView, "bufferView");
    this->GetOpt(item.byteOffset, "byteOffset");    
    ReadExtensionsAndExtras(item, this->curNode);
    this->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Gltf::Accessor::Sparse>(Gltf::Accessor::Sparse& item, const char * key)
{
    n_assert(key != nullptr);
    if (this->SetToFirstChild(key))
    {
        this->Get(item.count, "count");        
        this->Get(item.indices, "indices");
        this->Get(item.values, "values");        
        ReadExtensionsAndExtras(item, this->curNode);
        this->SetToParent();
        return true;
    }    
    return false;
}

//------------------------------------------------------------------------------
/**
*/
Base::VertexComponentBase::Format
AccessorTypeToNebula(Gltf::Accessor::ComponentType component, Gltf::Accessor::Type type)
{
    using Type = Gltf::Accessor::Type;
    using Component = Gltf::Accessor::ComponentType;
    Base::VertexComponentBase::Format format = Base::VertexComponentBase::Format::InvalidFormat;
    switch (type)
    {    
        case Type::Scalar:
        {
            switch (component)
            {
            case Component::Float: return Base::VertexComponentBase::Float;
            case Component::UnsignedShort: return Base::VertexComponentBase::UShort;
            }
        }
        break;
        case Type::Vec2:
        {
            switch (component)
            {
                case Component::Float: return Base::VertexComponentBase::Float2;
                case Component::Short: return Base::VertexComponentBase::Short2;                
            }
        }
        break;
        case Type::Vec3:
        {
            switch (component)
            {
                case Component::Float: return Base::VertexComponentBase::Float3;
            }
        }
        break;
        case Type::Vec4:
        {
            switch (component)
            {
                case Component::Byte: return Base::VertexComponentBase::Byte4;
                case Component::UnsignedByte: return Base::VertexComponentBase::UByte4;
                case Component::Float: return Base::VertexComponentBase::Float4;
                case Component::Short: return Base::VertexComponentBase::Short4;
                case Component::UnsignedShort: return Base::VertexComponentBase::UShort4;
            }
        }
        break;
        case Type::Mat2:
        case Type::Mat3:
        case Type::Mat4:
		case Type::None: break;
    }
    //n_assert2(format != Base::VertexComponentBase::Format::InvalidFormat, "undefined component type");
    return format;
}
//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Accessor>>(Util::Array<Gltf::Accessor> & items, const char* key)
{    
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            Gltf::Accessor & item = items[i++];
            item.componentType = static_cast<Gltf::Accessor::ComponentType>(this->GetInt("componentType"));
            this->Get(item.count, "count");
            this->Get(item.type, "type");
            item.format = AccessorTypeToNebula(item.componentType, item.type);
            this->GetOpt(item.bufferView, "bufferView", -1);
            this->GetOpt(item.byteOffset, "byteOffset");
            this->GetOpt(item.max, "max");
            this->GetOpt(item.min, "min");
            this->GetOpt(item.name, "name");
            this->GetOpt(item.normalized, "normalized", false);
            this->GetOpt(item.sparse, "sparse");
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Asset>(Gltf::Asset& item, const char * key)
{
    this->SetToFirstChild("asset");
    this->Get(item.version,"version");
    this->GetOpt(item.copyright, "copyright");
    this->GetOpt(item.generator, "generator");
    this->GetOpt(item.minVersion, "minVersion");
    this->SetToParent();
}


//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Gltf::Animation::Sampler::Type>(Gltf::Animation::Sampler::Type & item, const char * key)
{
    Util::String type = this->GetString(key);
    unsigned int foo = chash(type.AsCharPtr());
    switch (chash(type.AsCharPtr()))
    {
        case chash("LINEAR"): item = Gltf::Animation::Sampler::Type::Linear; break;
        case chash("STEP"): item = Gltf::Animation::Sampler::Type::Step; break;
        case chash("CUBICSPLINE"): item = Gltf::Animation::Sampler::Type::CubicSpline; break;
        default:
            n_assert("Unknown accessor type");
            return false;
    }
    return true;
}
//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Util::Array<Gltf::Animation::Sampler>>(Util::Array<Gltf::Animation::Sampler> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    this->SetToFirstChild(key);
    
    items.Resize(this->CurrentSize());
    // set to first array item
    this->SetToFirstChild();
    do {
        Gltf::Animation::Sampler & item = items[i++];
        this->Get(item.input, "input");
        this->Get(item.output, "output");
        this->GetOpt(item.interpolation, "interpolation");
        ReadExtensionsAndExtras(item, this->curNode);
    } while (this->SetToNextChild());
    this->SetToParent();
}



//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Animation::Channel::Target>(Gltf::Animation::Channel::Target& item, const char * key)
{
    this->SetToFirstChild(key);
    this->GetOpt(item.node, "node");
    this->Get(item.path, "path");
    ReadExtensionsAndExtras(item, this->curNode);
    this->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Util::Array<Gltf::Animation::Channel>>(Util::Array<Gltf::Animation::Channel> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    this->SetToFirstChild(key);
    
    items.Resize(this->CurrentSize());
    // set to first array item
    this->SetToFirstChild();
    do {
        Gltf::Animation::Channel & item = items[i++];
        this->Get(item.sampler, "sampler");
        this->Get(item.target, "target");            
        ReadExtensionsAndExtras(item, this->curNode);
    } while (this->SetToNextChild());
    this->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Animation>>(Util::Array<Gltf::Animation> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            Gltf::Animation & item = items[i++];
            this->Get(item.channels, "channels");
            this->Get(item.samplers, "samplers");
            this->GetOpt(item.name, "name");
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Buffer>>(Util::Array<Gltf::Buffer> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            this->GetOpt(item.name, "name");
            this->GetOpt(item.uri, "uri");
            this->GetOpt(item.name, "name");
            SizeT byteLength = this->GetInt("byteLength");
            item.data.Reserve(byteLength);
            Util::String folder = this->stream->GetURI().AsString().ExtractDirName();            
            item.Load(folder);
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::BufferView>>(Util::Array<Gltf::BufferView> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            this->Get(item.buffer, "buffer");
            this->Get(item.byteLength, "byteLength");
            this->GetOpt(item.byteOffset, "byteOffset");
            this->GetOpt(item.byteStride, "byteStride");
            this->GetOpt(item.name, "name");
            uint16_t target;
            if (this->GetOpt(target, "target"))
            {
                item.target = static_cast<Gltf::BufferView::TargetType>(target);
            }                    
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Camera::Perspective>(Gltf::Camera::Perspective & item, const char * key)
{
    this->SetToFirstChild(key);
    this->Get(item.yfov, "yfov");
    this->Get(item.znear, "znear");
    this->GetOpt(item.zfar, "zfar");
    this->GetOpt(item.aspectRatio, "aspectRatio");
    ReadExtensionsAndExtras(item, this->curNode);
    this->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Camera::Orthographic>(Gltf::Camera::Orthographic & item, const char * key)
{
    this->SetToFirstChild(key);
    this->Get(item.xmag, "xmag");
    this->Get(item.ymag, "ymag");
    this->Get(item.zfar, "zfar");
    this->Get(item.znear, "znear");
    ReadExtensionsAndExtras(item, this->curNode);
    this->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Camera::Type>(Gltf::Camera::Type & item, const char * key)
{
    Util::String type = this->GetString(key);
    unsigned int foo = chash(type.AsCharPtr());
    switch (chash(type.AsCharPtr()))
    {
    case chash("perspective"): item = Gltf::Camera::Type::Perspective; break;
    case chash("orthographic"): item = Gltf::Camera::Type::Orthographic; break;    
    default:
        n_assert("Unknown camera type");
    }
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Camera>>(Util::Array<Gltf::Camera> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            this->Get(item.type, "type");
            this->GetOpt(item.name, "name");
            switch (item.type)
            {
            case Gltf::Camera::Type::Perspective: this->Get(item.perspective, "perspective"); break;
            case Gltf::Camera::Type::Orthographic: this->Get(item.orthographic, "orthographic"); break;
            }
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Gltf::Material::AlphaMode>(Gltf::Material::AlphaMode & item, const char * key)
{
    if (this->HasAttr(key))
    {
        Util::String type = this->GetString(key);

        switch (chash(type.AsCharPtr()))
        {
        case chash("OPAQUE"): item = Gltf::Material::AlphaMode::Opaque; break;
        case chash("MASK"): item = Gltf::Material::AlphaMode::Mask; break;
        case chash("BLEND"): item = Gltf::Material::AlphaMode::Blend; break;
        default:
            n_assert("Unknown alpha mode type");
            return false;
        }
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Gltf::Material::Texture>(Gltf::Material::Texture& item, const char * key)
{
    if (this->SetToFirstChild(key))
    {
        this->Get(item.index, "index");
        this->GetOpt(item.texCoord, "texCoord");
        ReadExtensionsAndExtras(item, this->curNode);
        this->SetToParent();
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Gltf::Material::NormalTexture>(Gltf::Material::NormalTexture& item, const char * key)
{
    if (this->GetOpt(static_cast<Gltf::Material::Texture>(item), key))
    {
        this->SetToFirstChild(key);
        this->GetOpt(item.scale, "scale");
        ReadExtensionsAndExtras(item, this->curNode);
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Gltf::Material::OcclusionTexture>(Gltf::Material::OcclusionTexture& item, const char * key)
{
    if (this->GetOpt(static_cast<Gltf::Material::Texture>(item), key))
    {
        this->SetToFirstChild(key);
        this->GetOpt(item.strength, "strength");
        ReadExtensionsAndExtras(item, this->curNode);
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Gltf::Material::PBRMetallicRoughness>(Gltf::Material::PBRMetallicRoughness& item, const char * key)
{    
    if(this->SetToFirstChild(key))
    {
        this->GetOpt(item.baseColorFactor, "baseColorFactor");
        this->GetOpt(item.baseColorTexture, "baseColorTexture");
        this->GetOpt(item.metallicFactor, "metallicFactor");
        this->GetOpt(item.metallicRoughnessTexture, "metallicRoughnessTexture");
        this->GetOpt(item.roughnessFactor, "roughnessFactor");    
        ReadExtensionsAndExtras(item, this->curNode);
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Material>>(Util::Array<Gltf::Material> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            this->GetOpt(item.alphaMode, "alphaMode");
            this->GetOpt(item.alphaCutoff, "alphaCutoff");
            this->GetOpt(item.doubleSided, "doubleSided");
            this->GetOpt(item.emissiveFactor, "emissiveFactor");
            this->GetOpt(item.emissiveTexture, "emissiveTexture");
            this->GetOpt(item.name, "name");
            this->GetOpt(item.normalTexture, "normalTexture");
            this->GetOpt(item.occlusionTexture, "occlusionTexture");
            this->GetOpt(item.pbrMetallicRoughness, "pbrMetallicRoughness");            

            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------
/**
*/
static
Gltf::Primitive::Attribute
StringToPrimAttribute(const Util::String & attr)
{
    if (attr == "POSITION") return Gltf::Primitive::Attribute::Position;
    if (attr == "NORMAL") return Gltf::Primitive::Attribute::Normal;
    if (attr == "TANGENT") return Gltf::Primitive::Attribute::Tangent;
    if (attr.BeginsWithString("TEXCOORD_"))
    {
        char idx = attr[9] - '0';
        return (Gltf::Primitive::Attribute)((uint8_t)Gltf::Primitive::Attribute::TexCoord0 + idx);
    }
    if (attr.BeginsWithString("COLOR_"))
    {
        char idx = attr[6] - '0';
        return (Gltf::Primitive::Attribute)((uint8_t)Gltf::Primitive::Attribute::Color0 + idx);
    }
    if (attr.BeginsWithString("JOINTS_"))
    {
        char idx = attr[7] - '0';
        return (Gltf::Primitive::Attribute)((uint8_t)Gltf::Primitive::Attribute::Joints0 + idx);
    }
    if (attr.BeginsWithString("WEIGHTS_"))
    {
        char idx = attr[8] - '0';
        return (Gltf::Primitive::Attribute)((uint8_t)Gltf::Primitive::Attribute::Weights0 + idx);
    }
    n_assert("unknown primitive attribute");
    return Gltf::Primitive::Attribute::Position;
}

//------------------------------------------------------------------------------
/**
*/
static
Base::VertexComponentBase::SemanticName
AttributeToNebula(Gltf::Primitive::Attribute attr)
{
    switch (attr)
    {
        case Gltf::Primitive::Attribute::Position: return Base::VertexComponentBase::Position;
        case Gltf::Primitive::Attribute::Normal: return Base::VertexComponentBase::Normal;
        case Gltf::Primitive::Attribute::Tangent: return Base::VertexComponentBase::Tangent;        
        case Gltf::Primitive::Attribute::TexCoord0: return Base::VertexComponentBase::TexCoord1;
        case Gltf::Primitive::Attribute::TexCoord1: return Base::VertexComponentBase::TexCoord2;
        case Gltf::Primitive::Attribute::Color0: return Base::VertexComponentBase::Color;
        case Gltf::Primitive::Attribute::Weights0: return Base::VertexComponentBase::SkinWeights;
        default:
            n_warning("unhandled primitive attribute type");
        
    }
    return Base::VertexComponentBase::Invalid;
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Util::Dictionary<Gltf::Primitive::Attribute, uint32_t>>(Util::Dictionary<Gltf::Primitive::Attribute, uint32_t> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    this->SetToFirstChild(key);
    Util::Array<Util::String> attrs = this->GetAttrs();
    for (auto const & s : attrs)
    {
        items.Add(StringToPrimAttribute(s), this->GetInt(s.AsCharPtr()));
    }    
    this->SetToParent();
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Util::Array<Gltf::Primitive>>(Util::Array<Gltf::Primitive> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    this->SetToFirstChild(key);

    items.Resize(this->CurrentSize());
    // set to first array item
    this->SetToFirstChild();
    do {
        auto & item = items[i++];
        this->Get(item.attributes, "attributes");
        for (auto const& a : item.attributes)
        {
            item.nebulaAttributes.Add(AttributeToNebula(a.Key()), a.Value());
        }
        this->GetOpt(item.indices, "indices");
        this->GetOpt(item.material, "material");
        item.mode = static_cast<Gltf::Primitive::Mode>(this->GetOptInt("mode", (int)Gltf::Primitive::Mode::Triangles));
        switch (item.mode)
        {
            case Gltf::Primitive::Mode::Points: item.nebulaMode = CoreGraphics::PrimitiveTopology::Code::PointList; break;
            case Gltf::Primitive::Mode::Lines: item.nebulaMode = CoreGraphics::PrimitiveTopology::Code::LineList; break;
            case Gltf::Primitive::Mode::LineLoop:  item.nebulaMode = CoreGraphics::PrimitiveTopology::Code::InvalidPrimitiveTopology; n_warning("unsupported primitive topology");  break;
            case Gltf::Primitive::Mode::LineStrip: item.nebulaMode = CoreGraphics::PrimitiveTopology::Code::InvalidPrimitiveTopology; n_warning("unsupported primitive topology");  break;
            case Gltf::Primitive::Mode::Triangles: item.nebulaMode = CoreGraphics::PrimitiveTopology::Code::TriangleList; break;
            case Gltf::Primitive::Mode::TriangleStrip: item.nebulaMode = CoreGraphics::PrimitiveTopology::Code::TriangleStrip; break;
            case Gltf::Primitive::Mode::TriangleFan:  item.nebulaMode = CoreGraphics::PrimitiveTopology::Code::InvalidPrimitiveTopology; n_warning("unsupported primitive topology");  break;
        }
        if (this->SetToFirstChild("targets"))
        {
            IndexT j = 0;
            item.targets.Resize(this->CurrentSize());
            if (this->SetToFirstChild())
            {
                
                do
                {
                    auto & target = item.targets[j++];
                    Util::Array<Util::String> attrs = this->GetAttrs();
                    for (auto const & s : attrs)
                    {
                        target.Add(StringToPrimAttribute(s), this->GetInt(s.AsCharPtr()));
                    }
                } while (this->SetToNextChild());
                this->SetToParent();
            }
            this->SetToParent();
        }        
        ReadExtensionsAndExtras(item, this->curNode);
    } while (this->SetToNextChild());
    this->SetToParent();
}


//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Mesh>>(Util::Array<Gltf::Mesh> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            this->Get(item.primitives, "primitives"); 
            this->GetOpt(item.name, "name");
            this->GetOpt(item.weights, "weights");
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Node>>(Util::Array<Gltf::Node> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            item.type = Gltf::Node::Type::Transform;
            this->GetOpt(item.name, "name");

            if (this->GetOpt(item.camera, "camera"))
            {
                item.type = Gltf::Node::Type::Camera;
            }
            if (this->GetOpt(item.mesh, "mesh"))
            {
                item.type = Gltf::Node::Type::Mesh;
            }
            if (this->GetOpt(item.skin, "skin"))
            {
                item.type = Gltf::Node::Type::Skin;
            }

            this->GetOpt(item.children, "children");

            this->GetOpt(item.matrix, "matrix");
                        
            item.hasTRS |= this->GetOpt(item.rotation, "rotation");
            item.hasTRS |= this->GetOpt(item.scale, "scale");
            item.hasTRS |= this->GetOpt(item.translation, "translation");

            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Image>>(Util::Array<Gltf::Image> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            this->GetOpt(item.bufferView, "bufferView");
            Util::String mimeType;
            if (this->GetOpt(mimeType, "mimeType"))
            {
                if (mimeType == "image/jpeg") { item.type = Gltf::Image::Type::Jpg; }
                else if (mimeType == "image/png") { item.type = Gltf::Image::Type::Png; }
                else { n_assert("unkown mimeType"); }
            }            
            this->GetOpt(item.name, "name");
            this->GetOpt(item.uri, "uri");
            if (item.bufferView >= 0)
            {
                item.embedded = true;
            }
            else if(!item.uri.IsEmpty())
            {
                if (item.uri.BeginsWithString("data:"))
                {
                    // datauri with encoded image buffer
                    item.embedded = true;
                    Util::String mimeType = item.uri.ExtractRange(5, item.uri.FindCharIndex(';', 5) - 5);
                    if (mimeType == "image/jpeg") { item.type = Gltf::Image::Type::Jpg; }
                    else if (mimeType == "image/png") { item.type = Gltf::Image::Type::Png; }
                    else { n_assert("unkown mimeType"); }
                    IndexT start = item.uri.FindCharIndex(',', 5) + 1;
                    const char * buffer = item.uri.AsCharPtr() + start;
                    item.data.SetFromBase64(buffer, item.uri.Length() - start);
                }
                else
                {
                    item.embedded = false;
                    Util::String fullPath = this->stream->GetURI().AsString().ExtractDirName() + item.uri;
                    item.data.SetFromFile(fullPath);
                }                
            }
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Sampler>>(Util::Array<Gltf::Sampler> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];            
            uint16_t filter;
            if (this->GetOpt(filter, "magFilter")) item.magFilter = static_cast<Gltf::Sampler::MagFilter>(filter);
            if (this->GetOpt(filter, "minFilter")) item.minFilter = static_cast<Gltf::Sampler::MinFilter>(filter);
            if (this->GetOpt(filter, "wrapS")) item.wrapS = static_cast<Gltf::Sampler::WrappingMode>(filter);
            if (this->GetOpt(filter, "wrapT")) item.wrapT = static_cast<Gltf::Sampler::WrappingMode>(filter);                        
            this->GetOpt(item.name, "name");
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Scene>>(Util::Array<Gltf::Scene> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            this->GetOpt(item.nodes, "nodes");
            this->GetOpt(item.name, "name");
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Skin>>(Util::Array<Gltf::Skin> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];
            this->Get(item.joints, "joints");
            this->GetOpt(item.inverseBindMatrices, "inverseBindMatrices");
            this->GetOpt(item.skeleton, "skeleton");
            this->GetOpt(item.name, "name");
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> bool
IO::JsonReader::GetOpt<Util::Array<Gltf::Texture>>(Util::Array<Gltf::Texture> & items, const char* key)
{
    n_assert(key != nullptr);
    IndexT i = 0;
    if (this->SetToFirstChild(key))
    {
        items.Resize(this->CurrentSize());
        // set to first array item
        this->SetToFirstChild();
        do {
            auto & item = items[i++];            
            this->GetOpt(item.sampler, "sampler");
            this->GetOpt(item.source, "source");
            this->GetOpt(item.name, "name");
            ReadExtensionsAndExtras(item, this->curNode);
        } while (this->SetToNextChild());
        this->SetToParent();
        this->SetToParent();
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
template<> void
IO::JsonReader::Get<Gltf::Document>(Gltf::Document& item, const char * key)
{
    this->Get(item.asset, "asset");
    this->GetOpt(item.accessors, "accessors");
    this->GetOpt(item.animations, "animations");
    this->GetOpt(item.buffers, "buffers");
    this->GetOpt(item.bufferViews, "bufferViews");
    this->GetOpt(item.cameras, "cameras");
    this->GetOpt(item.materials, "materials");
    this->GetOpt(item.meshes, "meshes");
    this->GetOpt(item.nodes, "nodes");
    this->GetOpt(item.images, "images");
    this->GetOpt(item.samplers, "samplers");
    this->GetOpt(item.scene, "scene");
    this->GetOpt(item.scenes, "scenes");
    this->GetOpt(item.skins, "skins");
    this->GetOpt(item.textures, "textures");
    
    ReadExtensionsAndExtras(item, this->curNode);
}
namespace Gltf
{

//------------------------------------------------------------------------------
/**
*/
void
Gltf::Buffer::Load(Util::String const & folder)
{
    if (this->uri.BeginsWithString("data:"))
    {
        this->mimeType = uri.ExtractRange(5, this->uri.FindCharIndex(';', 5) - 5);
        // we dont deal with mime type here, we just load the raw buffer
        IndexT start = this->uri.FindCharIndex(',', 5) + 1;
        const char * buffer = this->uri.AsCharPtr() + start;
        this->data.SetFromBase64(buffer, this->uri.Length() - start);
        this->embedded = true;
#ifndef _DEBUG
        // dont keep encoded base64 buffer around
        this->uri.Clear();
#endif
    }
    else
    {
        Util::String fullPath = folder + "/" + this->uri;        
        this->data.SetFromFile(fullPath);
        this->embedded = false;
        this->mimeType = "";
    }
}


//------------------------------------------------------------------------------
/**
*/
bool 
Document::Deserialize(const IO::URI & uri)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(uri);
    return this->Deserialize(stream);    
}
//------------------------------------------------------------------------------
/**
*/
bool 
Document::Deserialize(Ptr<IO::Stream> const& stream)
{
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();    
    reader->SetStream(stream);
    if (reader->Open())
    {
        reader->Get(*this);
        reader->Close();
        return true;
    }
    return false;
}
}
