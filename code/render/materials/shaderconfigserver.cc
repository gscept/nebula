//------------------------------------------------------------------------------
//  materialserver.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "shaderconfigserver.h"
#include "resources/resourceserver.h"
#include "materialcache.h"
#include "shaderconfig.h"
#include "io/ioserver.h"
#include "io/jsonreader.h"
#include "coregraphics/batchgroup.h"
#include "coregraphics/shaderserver.h"

namespace Materials
{
__ImplementClass(Materials::ShaderConfigServer, 'MASV', Core::RefCounted)
__ImplementInterfaceSingleton(Materials::ShaderConfigServer)
//------------------------------------------------------------------------------
/**
*/
ShaderConfigServer::ShaderConfigServer() :
    isOpen(false),
    currentType(nullptr)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ShaderConfigServer::~ShaderConfigServer()
{
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConfigServer::Open()
{
    n_assert(!this->isOpen);
    this->isOpen = true;

    // load base materials first
    this->LoadShaderConfigs("mat:base.json");

    // okay, now load the rest of the materials
    Util::Array<Util::String> materialTables = IO::IoServer::Instance()->ListFiles("mat:", "*.json");
    for (IndexT i = 0; i < materialTables.Size(); i++)
    {
        if (materialTables[i] == "base.json") continue;
        this->LoadShaderConfigs("mat:" + materialTables[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ShaderConfigServer::Close()
{
    this->surfaceAllocator.Release();
}

//------------------------------------------------------------------------------
/**
*/
bool
ShaderConfigServer::LoadShaderConfigs(const IO::URI& file)
{
    Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(file);
    Ptr<IO::JsonReader> reader = IO::JsonReader::Create();
    reader->SetStream(stream);

    if (reader->Open())
    {
        // check to see it's a valid Nebula materials file
        if (!reader->HasNode("/Nebula/Materials"))
        {
            n_error("MaterialLoader: '%s' is not a valid material XML!", file.AsString().AsCharPtr());
            return false;
        }
        reader->SetToNode("/Nebula/Materials");

        // parse materials
        if (reader->SetToFirstChild()) do
        {
            ShaderConfig* type = this->surfaceAllocator.Alloc<ShaderConfig>();
            this->shaderConfigs.Append(type);

            type->name = reader->GetString("name");
            type->description = reader->GetOptString("desc", "");
            type->group = reader->GetOptString("group", "Ungrouped");
            shaderConfigsByName.Add(type->name, type);

            n_assert2(!type->name.ContainsCharFromSet("|"), "Name of material may not contain character '|' since it's used to denote multiple inheritance");

            bool isVirtual = reader->GetOptBool("virtual", false);
            if (isVirtual)
            {
                if (reader->HasAttr("vertexType"))
                {
                    n_error("Material '%s' is virtual and is not allowed to have a type defined", type->name.AsCharPtr());
                }
            }
            else
            {
                Util::String vtype = reader->GetString("vertexType");
                type->vertexType = vtype.HashCode();
            }
            type->isVirtual = isVirtual;

            Util::String inherits = reader->GetOptString("inherits", "");

            // load inherited materialx
            if (!inherits.IsEmpty())
            {
                Util::Array<Util::String> inheritances = inherits.Tokenize("|");
                IndexT i;
                for (i = 0; i < inheritances.Size(); i++)
                {
                    Util::String otherMat = inheritances[i];
                    IndexT index = this->shaderConfigsByName.FindIndex(otherMat);
                    if (index == InvalidIndex)
                        n_error("Material '%s' is not defined or loaded yet.", otherMat.AsCharPtr());
                    else
                    {
                        ShaderConfig* mat = this->shaderConfigsByName.ValueAtIndex(index);

                        type->textures.Merge(mat->textures);
                        type->constants.Merge(mat->constants);

                        // merge materials by adding new entry
                        auto it = mat->batchToIndexMap.Begin();
                        while (it != mat->batchToIndexMap.End())
                        {
                            IndexT idx = type->batchToIndexMap.FindIndex(*it.key);
                            if (idx == InvalidIndex)
                            {
                                // if new entry, add mapping and entry in lists
                                type->batchToIndexMap.Add(*it.key, type->programs.Size());
                                type->programs.Append(mat->programs[*it.val]);
                                type->texturesByBatch.Append(mat->texturesByBatch[*it.val]);
                                type->constantsByBatch.Append(mat->constantsByBatch[*it.val]);
                            }
                            else
                            {
                                // if entry exists, merge constants and texture dictionaries
                                idx = type->batchToIndexMap.FindIndex(idx);
                                type->programs[idx] = mat->programs[*it.val]; // this actually replaces the program, perhaps we want this to overload shaders
                                type->texturesByBatch[idx].Merge(mat->texturesByBatch[*it.val]);
                                type->constantsByBatch[idx].Merge(mat->constantsByBatch[*it.val]);
                            }
                            it++;
                        }
                    }
                }
            }

            // parse passes
            uint passIndex = 0;
            if (reader->SetToFirstChild("passes"))
            {
                if (reader->SetToFirstChild()) do
                {
                    // get batch name
                    Util::String batchName = reader->GetString("batch");
                    Util::String shaderFeatures = reader->GetString("variation");

                    // convert batch name to model node type
                    CoreGraphics::BatchGroup::Code code = CoreGraphics::BatchGroup::FromName(batchName);

                    //get shader
                    Util::String shaderName = reader->GetString("shader");
                    Resources::ResourceName shaderResId = Resources::ResourceName("shd:" + shaderName + ".fxb");
                    CoreGraphics::ShaderId shd = CoreGraphics::ShaderServer::Instance()->GetShader(shaderResId);
                    CoreGraphics::ShaderFeature::Mask mask = CoreGraphics::ShaderServer::Instance()->FeatureStringToMask(shaderFeatures);
                    CoreGraphics::ShaderProgramId program = CoreGraphics::ShaderGetProgram(shd, mask);

                    if (program == CoreGraphics::InvalidShaderProgramId)
                    {
                        n_warning("WARNING: Material '%s' failed to load program with features '%s'\n", type->name.AsCharPtr(), shaderFeatures.AsCharPtr());
                    }
                    else
                    {
                        n_assert(!type->batchToIndexMap.Contains(code));
                        type->batchToIndexMap.Add(code, passIndex++);
                        type->programs.Append(program);
                        type->texturesByBatch.Append({});
                        type->constantsByBatch.Append({});

                        // add material to server
                        Util::Array<Materials::ShaderConfig*>& mats = this->shaderConfigsByBatch.AddUnique(code);
                        mats.Append(type);
                    }
                } while (reader->SetToNextChild());
                reader->SetToParent();
            }
            
            // parse parameters
            if (reader->SetToFirstChild("variables"))
            {
                if (reader->SetToFirstChild()) do
                {
                    // parse parameters
                    Util::String name = reader->GetString("name");
                    Util::String ptype = reader->GetString("type");
                    Util::String desc = reader->GetOptString("desc", "");
                    Util::String editType = reader->GetOptString("edit", "raw");
                    bool system = reader->GetOptBool("system", false);

                    if (ptype.BeginsWithString("textureHandle"))
                    {
                        ShaderConfigConstant constant;
                        constant.type = Util::Variant::UInt64;
                        auto res = Resources::CreateResource(reader->GetString("defaultValue") + NEBULA_TEXTURE_EXTENSION, "material types", nullptr, nullptr, true);
                        constant.defaultValue = res.HashCode64();
                        constant.min = -1;
                        constant.max = -1;

                        constant.system = system;
                        constant.name = name;
                        constant.offset = InvalidIndex;
                        constant.slot = InvalidIndex;
                        constant.group = InvalidIndex;
                        type->constants.Add(name, constant);
                    }
                    else if (ptype.BeginsWithString("texture"))
                    {
                        ShaderConfigTexture texture;
                        if (ptype == "texture1d") texture.type = CoreGraphics::Texture1D;
                        else if (ptype == "texture2d") texture.type = CoreGraphics::Texture2D;
                        else if (ptype == "texture3d") texture.type = CoreGraphics::Texture3D;
                        else if (ptype == "texturecube") texture.type = CoreGraphics::TextureCube;
                        else if (ptype == "texture1darray") texture.type = CoreGraphics::Texture1DArray;
                        else if (ptype == "texture2darray") texture.type = CoreGraphics::Texture2DArray;
                        else if (ptype == "texturecubearray") texture.type = CoreGraphics::TextureCubeArray;
                        else
                        {
                            n_error("Invalid texture type %s\n", ptype.AsCharPtr());
                        }
                        auto res = Resources::CreateResource(reader->GetString("defaultValue") + NEBULA_TEXTURE_EXTENSION, "material types", nullptr, nullptr, true);
                        texture.defaultValue = res.As<CoreGraphics::TextureId>();
                        texture.system = system;
                        texture.name = name;
                        texture.slot = InvalidIndex;
                        type->textures.Add(name, texture);
                    }
                    else
                    {
                        ShaderConfigConstant constant;
                        constant.type = Util::Variant::StringToType(ptype);
                        switch (constant.type)
                        {
                        case Util::Variant::Float:
                            constant.defaultValue.SetFloat(reader->GetOptFloat("defaultValue", 0.0f));
                            constant.min.SetFloat(reader->GetOptFloat("min", 0.0f));
                            constant.max.SetFloat(reader->GetOptFloat("max", 1.0f));
                            break;
                        case Util::Variant::Int:
                            constant.defaultValue.SetInt(reader->GetOptInt("defaultValue", 0));
                            constant.min.SetInt(reader->GetOptInt("min", 0));
                            constant.max.SetInt(reader->GetOptInt("max", 1));
                            break;
                        case Util::Variant::Bool:
                            constant.defaultValue.SetBool(reader->GetOptBool("defaultValue", false));
                            constant.min.SetBool(false);
                            constant.max.SetBool(true);
                            break;
                        case Util::Variant::Vec4:
                            constant.defaultValue.SetVec4(reader->GetOptVec4("defaultValue", Math::vec4(0, 0, 0, 0)));
                            constant.min.SetVec4(reader->GetOptVec4("min", Math::vec4(0, 0, 0, 0)));
                            constant.max.SetVec4(reader->GetOptVec4("max", Math::vec4(1, 1, 1, 1)));
                            break;
                        case Util::Variant::Vec2:
                            constant.defaultValue.SetVec2(reader->GetOptVec2("defaultValue", Math::vec2(0, 0)));
                            constant.min.SetVec2(reader->GetOptVec2("min", Math::vec2(0, 0)));
                            constant.max.SetVec2(reader->GetOptVec2("max", Math::vec2(1, 1)));
                            break;
                        case Util::Variant::Mat4:
                            constant.defaultValue.SetMat4(reader->GetOptMat4("defaultValue", Math::mat4()));
                            break;
                        default:
                            n_error("Unknown material parameter type %s\n", ptype.AsCharPtr());
                        }

                        constant.system = system;
                        constant.name = name;
                        constant.offset = InvalidIndex;
                        constant.slot = InvalidIndex;
                        constant.group = InvalidIndex;
                        type->constants.Add(name, constant);
                    }
                } while (reader->SetToNextChild());
                reader->SetToParent();
            }
                        
            // setup type (this maps valid constants to their respective programs)
            type->Setup();
        } 
        while (reader->SetToNextChild());

        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
ShaderConfig* 
ShaderConfigServer::GetShaderConfig(const Resources::ResourceName& type)
{
    ShaderConfig* mat = this->shaderConfigsByName[type];
    return mat;
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<ShaderConfig*>*
ShaderConfigServer::GetShaderConfigsByBatch(CoreGraphics::BatchGroup::Code code)
{
    IndexT i = this->shaderConfigsByBatch.FindIndex(code);
    if (i == InvalidIndex)  return nullptr;
    else                    return &this->shaderConfigsByBatch.ValueAtIndex(code, i);
}

} // namespace Base
