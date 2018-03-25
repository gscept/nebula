//------------------------------------------------------------------------------
//  n2converter.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "n2converter.h"
#include "toolkitutil/binarymodelwriter.h"
#include "toolkitutil/xmlmodelwriter.h"
#include "io/ioserver.h"
#include "math/point.h"
#include "math/vector.h"
#include "io/xmlreader.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
N2Converter::N2Converter() :
    srcDir("dst:gfxlib"),
    dstDir("dst:models"),
    mshDir("dst:meshes"),
    platform(Platform::Win32),
    force(false),
    binary(true),
    nodeType(InvalidNodeType),
    verbose(false),
    isValid(false)
{
    // empty
    this->animInfo.animation.Clear();
    this->animInfo.key.Clear();
    this->animInfo.loopType.Clear();
    this->animInfo.paramName.Clear();
    this->animInfo.animatorNode = InvalidAnimatorNodeType;
    this->animInfo.animationGroup = -1;
    this->animInfo.vectorName.Clear();
    this->animInfo.posKey.Clear();
    this->animInfo.eulerKey.Clear();
    this->animInfo.scaleKey.Clear();
    this->animInfo.layer.Clear();

    //this->alignmentTable.Add("", "UnalignedX");
    this->alignmentTable.Add("Left", "LeftSide");
    this->alignmentTable.Add("Center", "CenterX");
    this->alignmentTable.Add("Right", "RightSide");
    //this->alignmentTable.Add("", "UnalignedY");
    this->alignmentTable.Add("Top", "UpSide");
    this->alignmentTable.Add("VCenter", "CenterY");
    this->alignmentTable.Add("Bottom", "DownSide");    
}

//------------------------------------------------------------------------------
/**
*/
N2Converter::~N2Converter()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
N2Converter::Setup()
{
    n_assert(!this->IsValid());
    this->isValid = true;
    this->ResetUsedResources();
        
    // setup N2ReflectionInfo and N2SceneLoader object
    this->n2ReflectionInfo = N2ReflectionInfo::Create();
    this->n2ReflectionInfo->Setup();
    this->n2SceneLoader = N2SceneLoader::Create();
    this->n2SceneLoader->Setup(this->n2ReflectionInfo);
}

//------------------------------------------------------------------------------
/**
*/
void
N2Converter::Discard()
{
    n_assert(this->IsValid());

    this->n2SceneLoader->Discard();
    this->n2ReflectionInfo->Discard();
    this->n2SceneLoader = 0;
    this->n2ReflectionInfo = 0;
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2Converter::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2Converter::ConvertAll(Logger& logger)
{
    bool success = true;
    Array<String> categories = IoServer::Instance()->ListDirectories(this->srcDir, "*");
    IndexT i;
    for (i = 0; i < categories.Size(); i++)
    {
        // ignore revision control system dirs
        if ((categories[i] != "CVS") && (categories[i] != ".svn"))
        {
            success &= this->ConvertCategory(logger, categories[i]);
        }
    }
    return success;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2Converter::ConvertCategory(Logger& logger, const String& category)
{
    n_assert(category.IsValid());
    bool success = true;

    // build absolute category path and get list of textures in category
    String catDir;
    catDir.Format("%s/%s", this->srcDir.AsCharPtr(), category.AsCharPtr());
    Array<String> files = IoServer::Instance()->ListFiles(catDir, "*.n2");

    // for each file...
    IndexT i;
    for (i = 0; i < files.Size(); i++)
    {
        success &= this->ConvertFile(logger, category, files[i]);
    }
    return success;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2Converter::ConvertFile(Logger& logger, const String& category, const String& fileName)
{
    this->srcPath.Format("%s/%s/%s", this->srcDir.AsCharPtr(), category.AsCharPtr(), fileName.AsCharPtr());
   
    // FIXME!
    // Debug dump of parsed scene node tree...
    // This scene node tree will later be the base of conversion from N2 to N3 file format.
    if (this->verbose)
    {
        Ptr<SceneNodeTree> sceneNodeTree = this->n2SceneLoader->Load(this->srcPath, logger);
        logger.Print("\n*** DUMP OF: %s:\n", this->srcPath.AsCharPtr());
        sceneNodeTree->Dump(logger);
    }
    
    String dstFilename = fileName;
    dstFilename.StripFileExtension();
    if (this->binary)
    {
        dstFilename.Append(".n3");
    }
    else
    {
        dstFilename.Append(".xml");
    }
    this->dstPath.Format("%s/%s/%s", this->dstDir.AsCharPtr(), category.AsCharPtr(), dstFilename.AsCharPtr());
    
    // check if conversion is necessary
    if (!this->NeedsConversion(srcPath, dstPath))
    {
        return true;
    }

    // display status
    if (this->verbose)
    {
        logger.Print("%s -> %s\n", this->srcPath.AsCharPtr(), this->dstPath.AsCharPtr());
    }

    // make sure the dst directory exists
    IoServer* ioServer = IoServer::Instance();
    String dstCatDir;
    dstCatDir.Format("%s/%s", this->dstDir.AsCharPtr(), category.AsCharPtr());
    ioServer->CreateDirectory(dstCatDir);

    // setup IO streams and stream readers/writers
    Ptr<Stream> srcStream = ioServer->CreateStream(srcPath);
    Ptr<BinaryReader> reader = BinaryReader::Create();
    reader->SetStream(srcStream);
    if (!reader->Open())
    {
        n_printf("Could not open '%s' for reading!\n", srcPath.AsCharPtr());
        return false;
    }
    Ptr<Stream> dstStream = ioServer->CreateStream(dstPath);
    Ptr<ModelWriter> modelWriter;
    if (this->binary)
    {
        modelWriter = BinaryModelWriter::Create();
    }
    else
    {
        modelWriter = XmlModelWriter::Create();
    }
    modelWriter->SetStream(dstStream);
    modelWriter->SetPlatform(this->platform);
    if (!modelWriter->Open())
    {
        n_printf("Could not open '%s' for writing!\n", dstPath.AsCharPtr());
        return false;
    }

    this->lastAnimatorName.Clear();
    this->animatedNodesToAnimator.Clear();
    this->animatorInformation.Clear();

    // perform actual conversion
    String modelName = category;
    modelName.Append("/");
    modelName.Append(fileName);
    modelName.StripFileExtension();
    if (!this->PerformConversion(modelName, reader, modelWriter))
    {
        n_printf("Conversion failed: '%s'!\n", srcPath.AsCharPtr());
        return false;
    }

    // done
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2Converter::PerformConversion(const String& curModelName, const Ptr<BinaryReader>& reader, const Ptr<ModelWriter>& writer)
{
    // first make sure the source file is actually a N2 file, and skip the header
    FourCC magic(reader->ReadUInt());
    if (magic != FourCC('NOB0'))
    {
        n_printf("Source file '%s' is not a Nebula2 binary file!\n", this->srcPath.AsCharPtr());
        return false;
    }
    
    // skip source header data
    String headerData = reader->ReadString();

    // check if file is a gui scene
    if (this->IsGuiScene(reader))
    {
        // convert to ui xml file
        this->ConvertGuiScene(reader, writer);
    }
    else
    {  
        // recursively read nodes
        writer->BeginModel("Models::Model", FourCC('MODL'), curModelName);
        if (!this->ParseNodes(reader, writer))
        {
            n_printf("Conversion error in RecurseReadNodes() ('%s')!\n", this->srcPath.AsCharPtr());
            return false;
        }
        writer->EndModel();
        n_assert(this->nodeTypeStack.IsEmpty());      
    }
    
    // all done
    return true;
}

//------------------------------------------------------------------------------
/**
*/
bool
N2Converter::ParseNodes(const Ptr<BinaryReader>& reader, const Ptr<ModelWriter>& writer)
{
    const FourCC newFourCC('_new');
    const FourCC selFourCC('_sel');
    FourCC curFourCC;
    while (!reader->Eof())
    {
        curFourCC = reader->ReadUInt();   
        if (curFourCC == newFourCC)
        {
            // save nodeinformation from last node
            this->FillAnimatorInformation();

            // new node encountered
            String objClass = reader->ReadString();
            String objName = reader->ReadString();
            this->SetAnimatorPath(objName, objClass);
            this->BeginNode(objClass, objName, writer);
        }
        else if (curFourCC == selFourCC)
        {
            // skip relative path '..'
            String selPath = reader->ReadString();
            // erase last path segment
            if (path.Size() > 0)
            {
                this->path.EraseIndex(path.Size()-1);     
            }            
            this->EndNode(writer);
        }
        else
        {
            // read current data tag
            this->ReadDataTag(curFourCC, reader, writer);
        }
    }
    if (/*!this->animatorInformation.IsEmpty()
        && */!this->animatedNodesToAnimator.IsEmpty())
    {
        // set the last animator node
        this->FillAnimatorInformation();

        // write optional animator node
        this->WriteAnimatorNode(writer);

        writer->EndModelNode();
    }

    // done
    return true;
}

//------------------------------------------------------------------------------
/**
*/  
void 
N2Converter::WriteAnimatorNode(const Ptr<ModelWriter>& writer)
{
    IndexT i;    
    writer->BeginModelNode("Model::AnimatorNode", FourCC('MANI'), "animator");
    for (i = 0; i < this->animatorInformation.Size(); i++)
    {
        AnimatorInfo& animInfo = this->animatorInformation.ValueAtIndex(i);

        // catch errors
        if (((animInfo.animatorNode == IntAnimator) ||
            (animInfo.animatorNode == FloatAnimator) ||
            (animInfo.animatorNode == Float4Animator)) &&
            (animInfo.paramName.IsEmpty()))
        {
            continue;
        }

        if (this->animatedNodesToAnimator.Contains(this->animatorInformation.KeyAtIndex(i)))
        {
            writer->BeginTag("BeginAnimSection", FourCC('BASE'));
            writer->WriteInt((int)animInfo.animatorNode);
            
            const Array<String>& pathToNodes = this->animatedNodesToAnimator[this->animatorInformation.KeyAtIndex(i)];
            IndexT pathIdx;
            for (pathIdx = 0; pathIdx < pathToNodes.Size(); pathIdx++)
            {
                writer->BeginTag("AnimatedNodes", FourCC('ANNO'));
                writer->WriteString(pathToNodes[pathIdx]);
                writer->EndTag();
            }

            if (!animInfo.animation.IsEmpty())
            {
                writer->BeginTag("AnimationName", FourCC('SANI'));
                writer->WriteString(animInfo.animation);
                writer->EndTag();
            }

            if (!animInfo.loopType.IsEmpty())
            {
                writer->BeginTag("LoopType", FourCC('SLPT'));
                writer->WriteString(animInfo.loopType);
                writer->EndTag();
            }

            if (!animInfo.paramName.IsEmpty())
            {
                writer->BeginTag("ParamName", FourCC('SPNM'));
                writer->WriteString(animInfo.paramName);
                writer->EndTag();
            }

            if (!animInfo.key.IsEmpty())
            {
                writer->BeginTag("Key", FourCC('ADDK'));
                //format
                switch(animInfo.key[0].value.GetType())
                {
                case Variant::Float:
                    writer->WriteString("Float");
                    break;
                case Variant::Float4:
                    writer->WriteString("Float4");
                    break;
                case Variant::Int:
                    writer->WriteString("Int");
                    break;
                }
                //size
                writer->WriteInt((int)animInfo.key.Size());
                for(IndexT j = 0; j < animInfo.key.Size(); j++)
                {
                    writer->WriteFloat(animInfo.key[j].time);
                    switch (animInfo.key[j].value.GetType())
                    {
                    case Variant::Float:
                        writer->WriteFloat(animInfo.key[j].value.GetFloat());
                        break;
                    case Variant::Float4:
                        writer->WriteFloat4(animInfo.key[j].value.GetFloat4());
                        break;
                    case Variant::Int:
                        writer->WriteInt(animInfo.key[j].value.GetInt());
                        break;
                    }
                }

                writer->EndTag();
            }

            if (!animInfo.vectorName.IsEmpty())
            {
                writer->BeginTag("VectorName", FourCC('SVCN'));
                writer->WriteString(animInfo.vectorName);
                writer->EndTag();
            }

            if (animInfo.animationGroup > -1)
            {
                writer->BeginTag("AnimationGroup", FourCC('SAGR'));
                writer->WriteInt(animInfo.animationGroup);
                writer->EndTag();
            }

            if (!animInfo.posKey.IsEmpty())
            {
                writer->BeginTag("PositionKey", FourCC('ADPK'));
                if (animInfo.animatorNode == 5)  //uvAnimator
                {
                    writer->WriteInt((int)animInfo.posKey.Size());
                    for (IndexT j = 0; j < animInfo.posKey.Size(); j++)
                    {
                        writer->WriteInt(animInfo.layer[j]);
                        writer->WriteFloat(animInfo.posKey[j].time);
                        writer->WriteFloat4(animInfo.posKey[j].value.GetFloat4());
                    }
                }
                else    //transformanimator
                {
                    writer->WriteInt((int)animInfo.posKey.Size());
                    for(IndexT j = 0; j < animInfo.posKey.Size(); j++)
                    {
                        writer->WriteFloat(animInfo.posKey[j].time);
                        writer->WriteFloat4(animInfo.posKey[j].value.GetFloat4());
                    }
                }
                writer->EndTag();
            }

            if (!animInfo.eulerKey.IsEmpty())
            {
                writer->BeginTag("EulerKey", FourCC('ADEK'));
                if (animInfo.animatorNode == 5)
                {
                    writer->WriteInt((int)animInfo.eulerKey.Size());
                    for (IndexT j = 0; j < animInfo.eulerKey.Size(); j++)
                    {
                        writer->WriteInt(animInfo.layer[j]);
                        writer->WriteFloat(animInfo.eulerKey[j].time);
                        writer->WriteFloat4(animInfo.eulerKey[j].value.GetFloat4());
                    }
                }
                else 
                {
                    writer->WriteInt((int)animInfo.eulerKey.Size());
                    for(IndexT j = 0; j < animInfo.eulerKey.Size(); j++)
                    {
                        writer->WriteFloat(animInfo.eulerKey[j].time);
                        writer->WriteFloat4(animInfo.eulerKey[j].value.GetFloat4());
                    }
                }
                writer->EndTag();
            }

            if (!animInfo.scaleKey.IsEmpty())
            {
                writer->BeginTag("ScaleKey", FourCC('ADSK'));
                if (animInfo.animatorNode == 5)
                {
                    writer->WriteInt((int)animInfo.scaleKey.Size());
                    for (IndexT j = 0; j < animInfo.scaleKey.Size(); j++)
                    {
                        writer->WriteInt(animInfo.layer[j]);
                        writer->WriteFloat(animInfo.scaleKey[j].time);
                        writer->WriteFloat4(animInfo.scaleKey[j].value.GetFloat4());
                    }
                }
                else 
                {
                    writer->WriteInt((int)animInfo.scaleKey.Size());
                    for(IndexT j = 0; j < animInfo.scaleKey.Size(); j++)
                    {
                        writer->WriteFloat(animInfo.scaleKey[j].time);
                        writer->WriteFloat4(animInfo.scaleKey[j].value.GetFloat4());
                    }
                }
                writer->EndTag();
            }              
            writer->EndTag();   
        }
    }
    writer->EndModelNode();
}

//------------------------------------------------------------------------------
/**
*/
void
N2Converter::BeginNode(const String& objClass, const String& objName, const Ptr<ModelWriter>& writer)
{
    // check for known classes
    if (objClass == "ntransformnode")
    {
        this->nodeType = TransformNode;
        this->animNodeType = InvalidAnimatorNodeType;
        writer->BeginModelNode("Models::TransformNode", FourCC('TRFN'), objName);
    }
    else if (objClass == "nshapenode")
    {
        this->nodeType = ShapeNode;
        this->animNodeType = InvalidAnimatorNodeType;
        writer->BeginModelNode("Models::ShapeNode", FourCC('SPND'), objName);
    }    
    else if (objClass == "nmultilayerednode")
    {
        this->nodeType = ShapeNode;
        this->animNodeType = InvalidAnimatorNodeType;
        writer->BeginModelNode("Models::ShapeNode", FourCC('SPND'), objName);
    }
    else if (objClass == "ncharacter3node")
    {
        this->nodeType = Character3Node;
        this->animNodeType = InvalidAnimatorNodeType;
        writer->BeginModelNode("Characters::CharacterNode", FourCC('CHRN'), objName);
        this->ConvertSkinLists(writer);
    }
    else if (objClass == "ncharacter3skinshapenode")
    {
        this->nodeType = Character3SkinShapeNode;
        this->animNodeType = InvalidAnimatorNodeType;
        writer->BeginModelNode("Characters::CharacterSkinNode", FourCC('CHSN'), objName);
    }
    else if (objClass == "nintanimator")
    {
        this->nodeType = EmbeddedNodeType;
        this->animNodeType = IntAnimator;
        this->lastAnimatorName = this->ConvertRelNodePathToAbsolute(objName, true);
    }
    else if (objClass == "nfloatanimator")
    {
        this->nodeType = EmbeddedNodeType;
        this->animNodeType = FloatAnimator;
        this->lastAnimatorName = this->ConvertRelNodePathToAbsolute(objName, true);
    }
    else if (objClass == "nvectoranimator")
    {
        this->nodeType = EmbeddedNodeType;
        this->animNodeType = Float4Animator;
        this->lastAnimatorName = this->ConvertRelNodePathToAbsolute(objName, true);
    }
    else if (objClass == "ntransformanimator")
    {
        this->nodeType = EmbeddedNodeType;
        this->animNodeType = TransformAnimator;
        this->lastAnimatorName = this->ConvertRelNodePathToAbsolute(objName, true);
    }
    else if (objClass == "ntransformcurveanimator")
    {
        this->nodeType = EmbeddedNodeType;
        this->animNodeType = TransformCurveAnimator;
        this->lastAnimatorName = this->ConvertRelNodePathToAbsolute(objName, true);
    }
    else if (objClass == "nuvanimator")
    {
        this->nodeType = EmbeddedNodeType;
        this->animNodeType = UvAnimator;
        this->lastAnimatorName = this->ConvertRelNodePathToAbsolute(objName, true);
    }
    else if (objClass == "ncharacter3skinanimator")
    {
        this->nodeType = EmbeddedNodeType;
        this->animNodeType = InvalidAnimatorNodeType;
    }
    else if (objClass == "nparticleshapenode2")
    {
        this->nodeType = ParticleSystemNode;
        writer->BeginModelNode("Particles::ParticleSystemNode", FourCC('PSND'), objName);
    }
    else if (objClass == "nlodnode")
    {
        // use normal transformnode for nlodnode
        this->nodeType = TransformNode;
        this->animNodeType = InvalidAnimatorNodeType;
        writer->BeginModelNode("Models::TransformNode", FourCC('TRFN'), objName);
    }
    else
    {
        this->nodeType = InvalidNodeType;
        this->animNodeType = InvalidAnimatorNodeType;
    }
    this->nodeTypeStack.Push(this->nodeType);
}

//------------------------------------------------------------------------------
/**
*/
void
N2Converter::EndNode(const Ptr<ModelWriter>& writer)
{
    NodeType nodeType = this->nodeTypeStack.Pop();
    if ((nodeType != InvalidNodeType) && (nodeType != EmbeddedNodeType))
    {
        if (!this->nodeTypeStack.IsEmpty() || this->animatedNodesToAnimator.IsEmpty())
        {
            writer->EndModelNode();
        }
    }
    this->nodeType = InvalidNodeType;
}

//------------------------------------------------------------------------------
/**
*/
void
N2Converter::ReadDataTag(FourCC fourCC, const Ptr<BinaryReader>& reader, const Ptr<ModelWriter>& writer)
{
    ushort length = reader->ReadUShort();
    if (InvalidNodeType == this->nodeType)
    {
        // if current node type is unknown, skip the tag's data block 
        reader->GetStream()->Seek(length, Stream::Current);
    }
    else
    {
        if ((FourCC('SLCB') == fourCC) && (this->nodeType != EmbeddedNodeType))
        {
            // setlocalbox
            float x, y, z;
            writer->BeginTag("LocalBox", FourCC('LBOX'));
            x = reader->ReadFloat();
            y = reader->ReadFloat();
            z = reader->ReadFloat();
            writer->WriteFloat4(point(x, y, z));
            x = reader->ReadFloat();
            y = reader->ReadFloat();
            z = reader->ReadFloat();
            writer->WriteFloat4(vector(x, y, z));
            writer->EndTag();
        }
        else if (FourCC('SPOS') == fourCC)
        {
            // setposition
            writer->BeginTag("Position", FourCC('POSI'));
            float x = reader->ReadFloat();
            float y = reader->ReadFloat();
            float z = reader->ReadFloat();
            writer->WriteFloat4(point(x, y, z));
            writer->EndTag();
        }
        else if (FourCC('SQUT') == fourCC)
        {
            // setquat
            writer->BeginTag("Rotation", FourCC('ROTN'));
            writer->WriteFloat4(reader->ReadFloat4());
            writer->EndTag();
        }
        else if (FourCC('SSCL') == fourCC)
        {
            // setscale
            writer->BeginTag("Scale", FourCC('SCAL'));
            float x = reader->ReadFloat();
            float y = reader->ReadFloat();
            float z = reader->ReadFloat();
            writer->WriteFloat4(vector(x, y, z));
            writer->EndTag();
        }
        else if (FourCC('SRTP') == fourCC)
        {
            // setrotatepivot
            writer->BeginTag("RotatePivot", FourCC('RPIV'));
            float x = reader->ReadFloat();
            float y = reader->ReadFloat();
            float z = reader->ReadFloat();
            writer->WriteFloat4(point(x, y, z));
            writer->EndTag();
        }
        else if (FourCC('SSCP') == fourCC)
        {
            // setscalepivot
            writer->BeginTag("ScalePivot", FourCC('SPIV'));
            float x = reader->ReadFloat();
            float y = reader->ReadFloat();
            float z = reader->ReadFloat();
            writer->WriteFloat4(point(x, y, z));
            writer->EndTag();
        }
        else if (FourCC('SVSP') == fourCC)
        {
            // setviewspace flag
            writer->BeginTag("RelativeToViewSpace", FourCC('SVSP')); 
            writer->WriteBool(reader->ReadBool());
            writer->EndTag();
        }
        else if (FourCC('SLKV') == fourCC)
        {
            // setlockviewer flag
            writer->BeginTag("LockViewer", FourCC('SLKV')); 
            writer->WriteBool(reader->ReadBool());
            writer->EndTag();
        }
        else if (FourCC('SMID') == fourCC)
        {
            // setmindistance flag
            writer->BeginTag("MinDistance", FourCC('SMID')); 
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SMAD') == fourCC)
        {
            // setmaxdistance flag
            writer->BeginTag("MaxDistance", FourCC('SMAD')); 
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('STXT') == fourCC)
        {
            // settexture
            writer->BeginTag("ShaderTexture", FourCC('STXT'));
            writer->WriteString(this->ConvertShaderSemantic(reader->ReadString()));

            String resId = this->SubstituteResourceAssigns(reader->ReadString());
            this->AddUsedResource(resId);
            resId.StripFileExtension();
            writer->WriteString(resId);
            writer->EndTag();
        }
        else if (FourCC('SINT') == fourCC)
        {
            // setint
            writer->BeginTag("ShaderInt", FourCC('SINT'));
            writer->WriteString(this->ConvertShaderSemantic(reader->ReadString()));
            writer->WriteInt(reader->ReadInt());
            writer->EndTag();
        }
        else if (FourCC('SFLT') == fourCC)
        {
            // setfloat
            writer->BeginTag("ShaderFloat", FourCC('SFLT'));
            writer->WriteString(this->ConvertShaderSemantic(reader->ReadString()));
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SVEC') == fourCC)
        {
            // setvector
            writer->BeginTag("ShaderVector", FourCC('SVEC'));
            writer->WriteString(this->ConvertShaderSemantic(reader->ReadString()));
            writer->WriteFloat4(reader->ReadFloat4());
            writer->EndTag();
        }
        else if (FourCC('SSHD') == fourCC)
        {
            // setshader
            String shaderName = reader->ReadString();
            writer->BeginTag("Shader", FourCC('SHDR'));
            writer->WriteString(this->ConvertShaderName(shaderName));
            writer->EndTag();

            // map shader name to ModelNodeType
            writer->BeginTag("ModelNodeType", FourCC('MNTP'));
            writer->WriteString(this->MapShaderNameToModelNodeType(shaderName));
            writer->EndTag();
        }
        else if (FourCC('SMSH') == fourCC)
        {
            // setmesh
            writer->BeginTag("Mesh", FourCC('MESH'));
            String resId = this->SubstituteResourceAssigns(reader->ReadString());
            this->AddUsedResource(resId);
            writer->WriteString(resId);
            writer->EndTag();
        }
        else if (FourCC('SGRI') == fourCC)
        {
            // setgroupindex
            writer->BeginTag("PrimitiveGroupIndex", FourCC('PGRI'));
            writer->WriteInt(reader->ReadInt());
            writer->EndTag();
        }
        else if (FourCC('SANM') == fourCC)
        {
            // setanim
            writer->BeginTag("Animation", FourCC('ANIM'));
            String resId = this->SubstituteResourceAssigns(reader->ReadString());
            this->AddUsedResource(resId);
            writer->WriteString(this->ConvertAnimationName(resId));
            writer->EndTag();
        }
        else if (FourCC('BJNT') == fourCC)
        {
            // beginjoints
            writer->BeginTag("NumJoints", 'NJNT');
            writer->WriteInt(reader->ReadInt());
            writer->EndTag();
        }
        else if (FourCC('SJNT') == fourCC)
        {
            // setjoint
            writer->BeginTag("Joint", 'JONT');
            int jointIndex = reader->ReadInt();
            int parentJointIndex = reader->ReadInt();
            point poseTranslation;
            poseTranslation.x() = reader->ReadFloat();
            poseTranslation.y() = reader->ReadFloat();
            poseTranslation.z() = reader->ReadFloat();
            float4 poseRotation = reader->ReadFloat4();
            vector poseScale;
            poseScale.x() = reader->ReadFloat();
            poseScale.y() = reader->ReadFloat();
            poseScale.z() = reader->ReadFloat();
            String name = reader->ReadString();
            writer->WriteInt(jointIndex);
            writer->WriteInt(parentJointIndex);
            writer->WriteFloat4(poseTranslation);
            writer->WriteFloat4(poseRotation);
            writer->WriteFloat4(poseScale);
            writer->WriteString(name);
            writer->EndTag();
        }
        else if (FourCC('SVRT') == fourCC)
        {
            // setvariations
            writer->BeginTag("Variations", 'VART');
            String resId = this->SubstituteResourceAssigns(reader->ReadString());
            this->AddUsedResource(resId);
            writer->WriteString(this->ConvertAnimationName(resId));
            writer->EndTag();
        }
        else if (FourCC('BGFR') == fourCC)
        {
            // setnumfragments
            int numFragments = reader->ReadInt();
            writer->BeginTag("NumSkinFragments", 'NSKF');
            writer->WriteInt(numFragments);
            writer->EndTag();
            this->skinFragmentPrimGroupIndices.SetSize(numFragments);
            this->skinFragmentJointPalettes.SetSize(numFragments);
        }
        else if (FourCC('SFGI') == fourCC)
        {
            IndexT fragIndex = reader->ReadInt();
            this->skinFragmentPrimGroupIndices[fragIndex] = reader->ReadInt();
        }
        else if (FourCC('BGJP') == fourCC)
        {
            // beginjointpalette            
            int fragIndex = reader->ReadInt();
            int skinFragmentNumJoints = reader->ReadInt();            
            this->skinFragmentJointPalettes[fragIndex].SetSize(skinFragmentNumJoints);
        }
        else if (FourCC('SJID') == fourCC)
        {
            // setjointindices
            // skip unnecessary data
            int fragIndex = reader->ReadInt();
            int paletteStartIndex = reader->ReadInt();

            // add up to 8 joint indices
            IndexT i;
            IndexT curIndex = paletteStartIndex;
            for (i = 0; i < 8; i++)
            {
                int jointIndex = reader->ReadInt();
                if (curIndex < this->skinFragmentJointPalettes[fragIndex].Size())
                {
                    this->skinFragmentJointPalettes[fragIndex][curIndex++] = jointIndex;
                }
            }
        }
        else if (FourCC('EDFR') == fourCC)
        {
            // endfragment
            IndexT i;
            for (i = 0; i < this->skinFragmentPrimGroupIndices.Size(); i++)
            {
                writer->BeginTag("SkinFragment", 'SFRG');
                writer->WriteInt(this->skinFragmentPrimGroupIndices[i]);
                writer->WriteIntArray(this->skinFragmentJointPalettes[i].AsArray());
                writer->EndTag();
            }
        }        
        else if (FourCC('SCVA') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleEmissionFrequency", 'EFRQ', reader, writer);
        }
        else if (FourCC('SCVB') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleLifeTime", 'PLFT', reader, writer);
        }
        else if (FourCC('SCVD') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleSpreadMin", 'PSMN', reader, writer);
        }
        else if (FourCC('SCVE') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleSpreadMax", 'PSMX', reader, writer);
        }
        else if (FourCC('SCVF') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleStartVelocity", 'PSVL', reader, writer);
        }
        else if (FourCC('SCVH') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleRotationVelocity", 'PRVL', reader, writer);
        }
        else if (FourCC('SCVJ') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleSize", 'PSZE', reader, writer);
        }
        else if (FourCC('SCVL') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleMass", 'PMSS', reader, writer);
        }
        else if (FourCC('STMM') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleTimeManipulator", 'PTMN', reader, writer);
        }
        else if (FourCC('SCVN') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleVelocityFactor", 'PVLF', reader, writer);
        }
        else if (FourCC('SCVQ') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleAirResistance", 'PAIR', reader, writer);
        }
        else if (FourCC('SCVC') == fourCC)
        {
            this->WriteParticleRGBEnvelopeCurve("ParticleRed", 'PRED', "ParticleGreen", 'PGRN', "ParticleBlue", 'PBLU', reader, writer);
        }
        else if (FourCC('SCVM') == fourCC)
        {
            this->WriteParticleEnvelopeCurve("ParticleAlpha", 'PALP', reader, writer);
        }
        else if (FourCC('SEMD') == fourCC)
        {
            writer->BeginTag("ParticleEmissionDuration", 'PEDU');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SLOP') == fourCC)
        {
            writer->BeginTag("ParticleLoopEmission", 'PLPE');
            writer->WriteInt(reader->ReadUChar());
            writer->EndTag();
        }
        else if (FourCC('SACD') == fourCC)
        {
            writer->BeginTag("ParticleActivityDistance", 'PACD');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SROF') == fourCC)
        {
            writer->BeginTag("ParticleRenderOldestFirst", 'PROF');
            writer->WriteInt(reader->ReadUChar());
            writer->EndTag();
        }
        else if (FourCC('SBBO') == fourCC)
        {
            writer->BeginTag("ParticleBillboardOrientation", 'PBBO');
            writer->WriteInt(reader->ReadUChar());
            writer->EndTag();
        }
        else if (FourCC('SRMN') == fourCC)
        {
            writer->BeginTag("ParticleRotationMin", 'PRMN');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SRMX') == fourCC)
        {
            writer->BeginTag("ParticleRotationMax", 'PRMX');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SGRV') == fourCC)
        {
            writer->BeginTag("ParticleGravity", 'PGRV');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SPST') == fourCC)
        {
            writer->BeginTag("ParticleStretch", 'PSTC');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('STTX') == fourCC)
        {
            writer->BeginTag("ParticleTextureTile", 'PTTX');
            writer->WriteFloat((float)reader->ReadInt());
            writer->EndTag();
        }
        else if (FourCC('SSTS') == fourCC)
        {
            writer->BeginTag("ParticleStretchToStart", 'PSTS');
            writer->WriteInt(reader->ReadUChar());
            writer->EndTag();
        }
        else if (FourCC('SCVR') == fourCC)
        {
            writer->BeginTag("ParticleVelocityRandomize", 'PVRM');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SCVS') == fourCC)
        {
            writer->BeginTag("ParticleRotationRandomize", 'PRRM');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SCVT') == fourCC)
        {
            writer->BeginTag("ParticleSizeRandomize", 'PSRM');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SPCT') == fourCC)
        {
            writer->BeginTag("ParticlePrecalcTime", 'PPCT');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('SCVU') == fourCC)
        {
            writer->BeginTag("ParticleRandomRotDir", 'PRRD');
            writer->WriteInt(reader->ReadUChar());
            writer->EndTag();
        }
        else if (FourCC('SSDT') == fourCC)
        {
            writer->BeginTag("ParticleStretchDetail", 'PSDL');
            writer->WriteInt(reader->ReadInt());
            writer->EndTag();
        }
        else if (FourCC('SVAF') == fourCC)
        {
            writer->BeginTag("ParticleViewAngleFade", 'PVAF');
            writer->WriteInt(reader->ReadUChar());
            writer->EndTag();
        }
        else if (FourCC('STDL') == fourCC)
        {
            writer->BeginTag("ParticleStartDelay", 'PDEL');
            writer->WriteFloat(reader->ReadFloat());
            writer->EndTag();
        }
        else if (FourCC('STUS') == fourCC)
        {
            Array<Math::float4> multilayerStretch;
            Array<Math::float4> multilayerSpec;
            // FIXME: layer size is fixed to 8 layers
            IndexT i;
            const SizeT numLayer = 8;
            float curStretch[numLayer];
            float curSpec[numLayer];
            for (i = 0; i < numLayer; i++)
            {
                n_assert(reader->ReadInt() == i);
                curStretch[i] = reader->ReadFloat();

                FourCC curFourCC = reader->ReadUInt(); 
                n_assert2(FourCC('SSPI') == curFourCC, "Reading multilayer parameter: Wrong fourCC!");
                ushort length = reader->ReadUShort();

                n_assert(reader->ReadInt() == i);
                curSpec[i] = reader->ReadFloat();
                
                if (i < numLayer - 1)
                {
                    curFourCC = reader->ReadUInt(); 
                    n_assert(FourCC('STUS') == curFourCC);
                    length = reader->ReadUShort();
                }
            }  

            // group single stretch and spec values to float4 for better use as shadervariables
            const int float4Size = 4;
            SizeT numVars = numLayer / float4Size;
            for (i = 0; i < numVars; i++)
            {
                // stretch
                writer->BeginTag("MultilayerUVStretch", 'STUS');
                writer->WriteInt(i);
                writer->WriteFloat4(Math::float4(curStretch[i * float4Size],
                                    curStretch[i * float4Size + 1],
                                    curStretch[i * float4Size + 2],
                                    curStretch[i * float4Size + 3]));
                writer->EndTag();

                // specular
                writer->BeginTag("MultilayerSpecIntensity", 'SSPI');
                writer->WriteInt(i);
                writer->WriteFloat4(Math::float4(curSpec[i * float4Size],
                                    curSpec[i * float4Size + 1],
                                    curSpec[i * float4Size + 2],
                                    curSpec[i * float4Size + 3]));
                writer->EndTag();  
            }
        }

        else if (FourCC('ADDA') == fourCC)
        {
            String absPath = this->ConvertRelNodePathToAbsolute(reader->ReadString());
            IndexT index = this->animatedNodesToAnimator.FindIndex(absPath);

            if (index == -1)
            {
                // add new Key if Absolute Path not exists
                Array<String> nodePath;
                nodePath.Append(this->ConvertNodePathFromArrayToString(this->path));
                this->animatedNodesToAnimator.Add(absPath, nodePath);
            }
            else
            {
                // append nodepath to key if key already exists
                this->animatedNodesToAnimator[absPath].Append(this->ConvertNodePathFromArrayToString(this->path));
            }
        }
        else if (FourCC('SLPT') == fourCC)
        {
            // nebula3 uses no skinanimators, so this fourcc is only used by the other animators
            if (this->animNodeType >= IntAnimator && this->animNodeType <= UvAnimator)
            {
                this->animInfo.loopType = reader->ReadString();
            }
            else
            {                
                reader->GetStream()->Seek(length, Stream::Current);
            }
        }
        else if (FourCC('ADDK') == fourCC)
        {
            if (this->animNodeType == Float4Animator)
            {
                KeyValues val;
                val.time = reader->ReadFloat();
                val.value = reader->ReadFloat4();
                this->animInfo.key.Append(val);
            }
            else if (this->animNodeType == FloatAnimator)
            {
                KeyValues val;
                val.time = reader->ReadFloat();
                val.value = reader->ReadFloat();
                this->animInfo.key.Append(val);
            }
            else if (this->animNodeType == IntAnimator)
            {
                KeyValues val;
                val.time = reader->ReadFloat();
                val.value = reader->ReadInt();
                this->animInfo.key.Append(val);
            }
            else
            {                
                reader->GetStream()->Seek(length, Stream::Current);
            }
        }
        else if (FourCC('SPNM') == fourCC)
        {
            //ShaderParamName
            this->animInfo.paramName = reader->ReadString();
        }
        else if (FourCC('SVCN') == fourCC)
        {
            //VectorName
            this->animInfo.vectorName = reader->ReadString();
        }
        else if (FourCC('ADPK') == fourCC)
        {
            //PositionKey
            KeyValues val;
            if (this->animNodeType == UvAnimator)
            {
                this->animInfo.layer.Append(reader->ReadInt());
                val.time = (float)reader->ReadFloat();
                float u = reader->ReadFloat();
                float v = reader->ReadFloat();
                val.value.SetFloat4(Math::float4(u, v, 0, 1));
            }
            else //transformanimator
            {
                val.time = (float)reader->ReadFloat();
                float x = reader->ReadFloat();
                float y = reader->ReadFloat();
                float z = reader->ReadFloat();
                val.value.SetFloat4(Math::float4(x, y, z, 1));
            }
            
            this->animInfo.posKey.Append(val);
        }
        else if (FourCC('ADEK') == fourCC)
        {
            //EulerKey
            KeyValues val;
            if (this->animNodeType == UvAnimator)
            {
                this->animInfo.layer.Append(reader->ReadInt());
                val.time = (float)reader->ReadFloat();
                float u = n_deg2rad(reader->ReadFloat());
                float v = n_deg2rad(reader->ReadFloat());
                val.value.SetFloat4(Math::float4(u, v, 0, 1));
            }
            else //transformanimator
            {
                val.time = (float)reader->ReadFloat();
                float x = reader->ReadFloat();
                float y = reader->ReadFloat();
                float z = reader->ReadFloat();
                val.value.SetFloat4(Math::float4(x, y, z, 1));
            }
            this->animInfo.eulerKey.Append(val);
        }
        else if (FourCC('ADSK') == fourCC)
        {
            //ScaleKey
            KeyValues val;
            if (this->animNodeType == UvAnimator)
            {
                this->animInfo.layer.Append(reader->ReadInt());
                val.time = (float)reader->ReadFloat();
                float u = reader->ReadFloat();
                float v = reader->ReadFloat();
                val.value.SetFloat4(Math::float4(u, v, 1, 1));
            }
            else //transformanimator
            {
                val.time = (float)reader->ReadFloat();
                float x = reader->ReadFloat();
                float y = reader->ReadFloat();
                float z = reader->ReadFloat();
                val.value.SetFloat4(Math::float4(x, y, z, 1));
            }
            this->animInfo.scaleKey.Append(val);
        }
        else if (FourCC('SANI') == fourCC)
        {
            //animation
            String resId = this->SubstituteResourceAssigns(reader->ReadString());
            this->AddUsedResource(resId);
            this->animInfo.animation = this->ConvertAnimationName(resId);
        }
        else if (FourCC('SAGR') == fourCC)
        {
            //animationgroup
            this->animInfo.animationGroup = reader->ReadInt();
        }
        else if (FourCC('SSTA') == fourCC)
        {
            writer->BeginTag("StringAttr", 'SSTA');
            writer->WriteString(reader->ReadString());
            writer->WriteString(reader->ReadString());
            writer->EndTag();
        }
        else
        {
            // skip unknown data tags
            reader->GetStream()->Seek(length, Stream::Current);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Reads a particle system envelope curve data entry from the reader, 
    and writes it as a new data tag to the model writer.
*/
void
N2Converter::WriteParticleEnvelopeCurve(const String& tagName, FourCC fourCC, const Ptr<BinaryReader>& reader, const Ptr<ModelWriter>& writer)
{
    writer->BeginTag(tagName, fourCC);
    writer->WriteFloat(reader->ReadFloat());    // val0
    writer->WriteFloat(reader->ReadFloat());    // val1
    writer->WriteFloat(reader->ReadFloat());    // val2
    writer->WriteFloat(reader->ReadFloat());    // val3
    writer->WriteFloat(reader->ReadFloat());    // keyPos0
    writer->WriteFloat(reader->ReadFloat());    // keyPos1
    writer->WriteFloat(reader->ReadFloat());    // frequency
    writer->WriteFloat(reader->ReadFloat());    // amplitude
    writer->WriteInt(reader->ReadInt());        // modfunc
    writer->EndTag();
}

//------------------------------------------------------------------------------
/**
    Special version of WriteParticleEnvelopeCurve which reads an
    RGB envelope curve and writes it as 3 independent envelope curves.
*/
void
N2Converter::WriteParticleRGBEnvelopeCurve(const String& redTagName, FourCC redFourCC,
                                           const String& greenTagName, FourCC greenFourCC,
                                           const String& blueTagName, FourCC blueFourCC,
                                           const Ptr<BinaryReader>& reader,
                                           const Ptr<ModelWriter>& writer)
{
    // read original data
    float red[4], green[4], blue[4];
    float keyPos0, keyPos1;
    float frequency, amplitude;
    int modFunc;
    IndexT i;
    for (i = 0; i < 4; i++)
    {
        red[i]   = reader->ReadFloat();
        green[i] = reader->ReadFloat();
        blue[i]  = reader->ReadFloat();
    }
    keyPos0 = reader->ReadFloat();
    keyPos1 = reader->ReadFloat();

    // NOTE: Nebula2-RGB-EnvelopeCurves don't have the sine/cosine modulation!
    frequency = 1.0f;
    amplitude = 0.0f;
    modFunc = 0;

    // write red data tag
    writer->BeginTag(redTagName, redFourCC);
    writer->WriteFloat(red[0]);
    writer->WriteFloat(red[1]);
    writer->WriteFloat(red[2]);
    writer->WriteFloat(red[3]);
    writer->WriteFloat(keyPos0);
    writer->WriteFloat(keyPos1);
    writer->WriteFloat(frequency);
    writer->WriteFloat(amplitude);
    writer->WriteInt(modFunc);
    writer->EndTag();

    // write green data tag
    writer->BeginTag(greenTagName, greenFourCC);
    writer->WriteFloat(green[0]);
    writer->WriteFloat(green[1]);
    writer->WriteFloat(green[2]);
    writer->WriteFloat(green[3]);
    writer->WriteFloat(keyPos0);
    writer->WriteFloat(keyPos1);
    writer->WriteFloat(frequency);
    writer->WriteFloat(amplitude);
    writer->WriteInt(modFunc);
    writer->EndTag();

    // write blue data tag
    writer->BeginTag(blueTagName, blueFourCC);
    writer->WriteFloat(blue[0]);
    writer->WriteFloat(blue[1]);
    writer->WriteFloat(blue[2]);
    writer->WriteFloat(blue[3]);
    writer->WriteFloat(keyPos0);
    writer->WriteFloat(keyPos1);
    writer->WriteFloat(frequency);
    writer->WriteFloat(amplitude);
    writer->WriteInt(modFunc);
    writer->EndTag();
}


//------------------------------------------------------------------------------
/**
*/
String
N2Converter::MapShaderNameToModelNodeType(const String& shaderName)
{
    if ("simple_alpha" == shaderName
        || "particle2_unlit" == shaderName)
    {
        return "Alpha";
    } 
    else if ("particle2" == shaderName)
    {
        return "ParticleLit";
    }
    else if ("additive_alpha" == shaderName)
    {
        return "Additive";
    }
    else if (String::MatchPattern(shaderName, "*alpha*") ||
             "volumefog" == shaderName ||  
             "hair" == shaderName ||
             "skinned_alpha" == shaderName)
    {
        return "AlphaLit";
    }
    else if ("particle2_additive" == shaderName)
    {
        return "Additive";
    }
    else if ("static_atest" == shaderName)
    {
        return "AlphaTest";
    }
    else if ("skybox" == shaderName)
    {
        return "Background";
    }
    else if ("gui3d" == shaderName)
    {
        return "UnlitAlpha";
    }
	else if ("qwiiz_solid_unlit" == shaderName)
	{
	    return "UnlitSolid";
	}
	else if ("qwiiz_translucent" == shaderName)
	{
	    return "Translucent";
	}
    else
    {
        return "Solid";
    }
}

//------------------------------------------------------------------------------
/**
*/
String
N2Converter::ConvertShaderName(const String& shdName)
{
    if ("lightmapped" == shdName
        || "lightmapped2" == shdName
        || "lightmapped_alpha" == shdName)
    {
        return "shd:lightmapped";
    }
	else if ("lightmapped2_skinned" == shdName)
	{
		return "shd:lightmapped2_skinned";
	}
	else if ("lightmapped_environment" == shdName)
	{
		return "shd:lightmapped_environment";
	}
    else if ("skybox" == shdName)
    {
        return "shd:skybox";
    }
    else if ("multilayered" == shdName 
        ||"multilayeredbump" == shdName)
    {
        return "shd:multilayer";
    }
    else if ("volumefog" == shdName)
    {
        return "shd:volumefog";
    }
    else if ("alpha_uvanimation" == shdName)
    {
        return "shd:uvanimated";
    }
    else if ("particle2" == shdName)
    {
        return "shd:particle";
    }
    else if ("particle2_additive" == shdName)
    {
        return "shd:particle";
    }
	else if ("particle2_unlit" == shdName)
    {
        return "shd:particle";
    }
    else if ("simple_alpha" == shdName)
    {
        return "shd:unlit";
    }
    else if ("environment_skinned" == shdName
           || "environment" == shdName
           || "environment_alpha" == shdName)
    {
        return "shd:environment";
    }
    else if ("gui3d" == shdName)
    {
        return "shd:unlit";
    }
	else if ("qwiiz_lm_alpha" == shdName)
	{
		return "shd:qwiiz_lm_alpha";
	}
	else if ("qwiiz_lm_env_alpha" == shdName)
	{
		return "shd:qwiiz_lm_env_alpha";
	}
	else if ("qwiiz_solid_unlit" == shdName)
	{
		return "shd:qwiiz_solid_unlit";
	}
	else if ("qwiiz_toon_specularmap" == shdName)
	{
		return "shd:qwiiz_toon_specularmap";
	}
	else if ("qwiiz_uvanimation_lightmapped" == shdName)
	{
		return "shd:qwiiz_uvanimated_lightmapped";
	}
    else
    {
        // map everything else to static
        return "shd:static";
    }
}

//------------------------------------------------------------------------------
/**
    Convert shader semantic name if necessary (on the PS3, shader
    semantics are all upper case).
*/
String
N2Converter::ConvertShaderSemantic(const String& sem) const
{
   return sem;   
}

//------------------------------------------------------------------------------
/**
    Convert animation resource id (just changes the file extension).
*/
String
N2Converter::ConvertAnimationName(const String& anim) const
{
    String res = anim;
    res.StripFileExtension();
    res.Append(".nax3");
    return res;
}

//------------------------------------------------------------------------------
/**
    Perform a file time check to decide whether a texture must be
    converted.
*/
bool
N2Converter::NeedsConversion(const String& srcPath, const String& dstPath)
{
    // file time check overriden?
    if (this->force)
    {
        return true;
    }

    // if this is a character, we would need to check skin lists as well,
    // instead we just assume that a conversion is needed
    if (String::MatchPattern(srcPath, "*characters*"))
    {
        return true;
    }

    // otherwise check file times of src and dst file
    IoServer* ioServer = IoServer::Instance();
    if (ioServer->FileExists(dstPath))
    {
        FileTime srcFileTime = ioServer->GetFileWriteTime(srcPath);
        FileTime dstFileTime = ioServer->GetFileWriteTime(dstPath);
        if (dstFileTime > srcFileTime)
        {
            // dst file newer then src file, don't need to convert
            return false;
        }
    }

    // fallthrough: dst file doesn't exist, or it is older then the src file
    return true;
}

//------------------------------------------------------------------------------
/**
    Loads the skin lists for the current character and writes them
    into the n3 file. This method is called by EndNode() when
    a Character3Node is finished.
*/
bool
N2Converter::ConvertSkinLists(const Ptr<ModelWriter>& writer)
{
    IoServer* ioServer = IoServer::Instance();
    
    // build skin list directory from current file name
    String skinListDir = this->srcPath;
    skinListDir.StripFileExtension();
    skinListDir.Append("/skinlists");
    if (!ioServer->DirectoryExists(skinListDir))
    {
        return false;
    }

    // get the skin lists files for this character
    Array<String> skinLists = ioServer->ListFiles(skinListDir, "*.xml");
    if (skinLists.Size() == 0)
    {
        return false;
    }

    // write number of skin lists to the N3 file
    writer->BeginTag("NumSkinLists", 'NSKL');
    writer->WriteInt(skinLists.Size());
    writer->EndTag();

    // parse skin list xml files
    IndexT skinListIndex;
    for (skinListIndex = 0; skinListIndex < skinLists.Size(); skinListIndex++)
    {   
        String skinListName = skinLists[skinListIndex];
        skinListName.StripFileExtension();
        Array<String> skins;
        
        // read skin list into an xml reader
        String skinListPath = skinListDir + "/" + skinLists[skinListIndex];
        Ptr<Stream> stream = ioServer->CreateStream(skinListPath);
        Ptr<XmlReader> xmlReader = XmlReader::Create();
        xmlReader->SetStream(stream);
        if (xmlReader->Open())
        {
            xmlReader->SetToNode("/skins");
            if (xmlReader->SetToFirstChild()) do
            {
                xmlReader->SetToFirstChild();
                xmlReader->SetToFirstChild();
                skins.Append(xmlReader->GetCurrentNodeName());
                xmlReader->SetToParent();
                xmlReader->SetToParent();
            }
            while (xmlReader->SetToNextChild());
            xmlReader->Close();
        }

        // write skin list to n3 file
        writer->BeginTag("SkinList", 'SKNL');
        writer->WriteString(skinListName);
        writer->WriteInt(skins.Size());
        IndexT skinIndex;
        for (skinIndex = 0; skinIndex < skins.Size(); skinIndex++)
        {
            writer->WriteString(skins[skinIndex]);
        }
        writer->EndTag();
    }
    return true;
}

//------------------------------------------------------------------------------
/**
*/
N2Converter::AnimatorNodeType
N2Converter::FromStringToAnimatorNode(String animNode)
{
    if (animNode == "ntransformcurveanimator")      return TransformCurveAnimator;
    else if (animNode == "ntransformanimator")      return TransformAnimator;
    else if (animNode == "nfloatanimator")          return FloatAnimator;
    else if (animNode == "nintanimator")            return IntAnimator;
    else if (animNode == "nvectoranimator")         return Float4Animator;
    else if (animNode == "nuvanimator")             return UvAnimator;
    else return InvalidAnimatorNodeType;
}

//------------------------------------------------------------------------------
/**
*/
void
N2Converter::FillAnimatorInformation()
{
    if ((!this->lastAnimatorName.IsEmpty()) && (this->animInfo.animatorNode != InvalidAnimatorNodeType))
    {
        this->animatorInformation.Add(this->lastAnimatorName, this->animInfo);
        this->lastAnimatorName.Clear();

        //Cleanup BufferVariable for later usage
        this->animInfo.animation.Clear();
        this->animInfo.key.Clear();
        this->animInfo.loopType.Clear();
        this->animInfo.paramName.Clear();
        this->animInfo.animatorNode = InvalidAnimatorNodeType;
        this->animInfo.animationGroup = -1;
        this->animInfo.vectorName.Clear();
        this->animInfo.posKey.Clear();
        this->animInfo.eulerKey.Clear();
        this->animInfo.scaleKey.Clear();
        this->animInfo.layer.Clear();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
N2Converter::SetAnimatorPath(String objName, String objClass)
{
    // build the path
    if(path.Size() > 0)
        this->path.Append("/"+objName);
    else
        this->path.Append(objName);

    //save the objectClass for later use
    this->animInfo.animatorNode = this->FromStringToAnimatorNode(objClass);
}

//------------------------------------------------------------------------------
/**
*/
String
N2Converter::ConvertNodePathFromArrayToString(Array<String> stringArray)
{
    IndexT tokenIdx;
    String returnValue;

    for (tokenIdx = 0; tokenIdx < stringArray.Size(); tokenIdx++)
    {
        returnValue.Append(stringArray[tokenIdx]);
    }

    return returnValue;
}

//------------------------------------------------------------------------------
/**
    Substitute old-school Nebula2 assigns into Nebula3 assigns
    (e.g. textures: -> tex:)
*/
String
N2Converter::SubstituteResourceAssigns(const String& str)
{
    String result = str;
    result.SubstituteString("textures:", "tex:");
    result.SubstituteString("meshes:", "msh:");
    result.SubstituteString("anims:", "ani:");
    return result;
}

//------------------------------------------------------------------------------
/**
    Converts the relative AnimatorPath to an Absolute AnimatorPath
*/
String
N2Converter::ConvertRelNodePathToAbsolute(String relPath, bool pathToAnimatorNode)
{
    Array<String> tokenized = relPath.Tokenize("/");
    IndexT tokenIdx;
    Array<String> nodePath = this->path;

    for(tokenIdx = tokenized.Size()-1; tokenIdx > -1; tokenIdx--)
    {
        if (tokenized[tokenIdx] == "..")
        {
            //Erase last ".." and last nodePath-segment
            nodePath.EraseIndex(nodePath.Size()-1);
            tokenized.EraseIndex(tokenIdx);
        }
    }
    String fragmentRelativePath = this->ConvertNodePathFromArrayToString(tokenized);
    String fragmentNodePath = this->ConvertNodePathFromArrayToString(nodePath);

    if (!pathToAnimatorNode)
    {
        // return Absolute Path
        if (fragmentNodePath.IsEmpty())
            return fragmentRelativePath;
        else
        {
            return fragmentNodePath + "/" + fragmentRelativePath;
        }
    }
    else
    {
        return fragmentNodePath;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool 
N2Converter::IsGuiScene(const Ptr<BinaryReader>& reader)
{
    Stream::Position oldPos = reader->GetStream()->GetPosition();

    bool isGuiScene = false;
    const FourCC newFourCC('_new');
    const FourCC selFourCC('_sel');
    FourCC curFourCC;
    while (!reader->Eof() && !isGuiScene)
    {
        curFourCC = reader->ReadUInt();   
        if (curFourCC == newFourCC)
        {
            // new node encountered skip strings
            String objClass = reader->ReadString();
            String objName = reader->ReadString();
        }
        else if (curFourCC == selFourCC)
        {
            // skip relative path '..'
            String selPath = reader->ReadString();
        }
        else
        {
            ushort length = reader->ReadUShort();
            // check for setboolattr with rlGui == true
            if (FourCC('SBOA') == curFourCC)
            {
                String attrName = reader->ReadString();
                bool value = reader->ReadBool();
                if (attrName == "rlGui")
                {
                    isGuiScene = value;                       
                }
            }
            else
            {     
                // skip non bool attr
                reader->GetStream()->Seek(length, Stream::Current);
            }
        }
    }
    // reset stream position
    reader->GetStream()->Seek(oldPos, Stream::Begin);

    return isGuiScene;
}

//------------------------------------------------------------------------------
/**
*/
void 
N2Converter::ConvertGuiScene(const Ptr<IO::BinaryReader>& reader, const Ptr<ModelWriter>& writer)
{           
    //n_printf("Converting GuiScene %s: \n", this->srcPath.AsCharPtr());

    // create one nvx2reader for reading uv coordinated from mesh file
    this->nvx2Reader = Legacy::Nvx2OrderFreeStreamReader::Create();
    if (this->platform != Platform::Linux
        && this->platform != Platform::Win32)        
    {
        this->nvx2Reader->SetSourcePlatformEndianess(System::ByteOrder::BigEndian);
    }

    const FourCC newFourCC('_new');
    const FourCC selFourCC('_sel');
    FourCC fourCC;
    String objClass;
    String objName;

    Math::bbox curRawLocalBox;

    uint curLevel = 0;
    Util::String meshPath;
    Util::Stack<uint> guiLevel;
    Util::SimpleTree<GuiElement>::Node* node = &this->windowTree.Root();       
    while (!reader->Eof())
    {
        fourCC = reader->ReadUInt();   
        if (fourCC == newFourCC)
        {
            // new node encountered skip strings
            objClass = reader->ReadString();
            String curObjName = reader->ReadString();
            if (objClass == "ntransformnode")
            {
                objName = curObjName;                 
            }      
            //n_printf("ObjName: %s \n", curObjName.AsCharPtr());
            curLevel++;            
        }
        else if (fourCC == selFourCC)
        {                   
            String selPath = reader->ReadString();
            
            if (selPath == "..")
            { 
                if (guiLevel.Size() > 0 &&
                    curLevel == guiLevel.Peek())
                {
                    node = &node->Parent();                    
                    guiLevel.Pop();
                }           
                curLevel--;                
            }
        }
        else
        {
            ushort length = reader->ReadUShort();
            // check for setboolattr with rlGui == true
            if (FourCC('SBOA') == fourCC)
            {
                String attrName = reader->ReadString();
                bool value = reader->ReadBool(); 
                if (attrName == "rlGui" && value)
                {    
                    // new element
                    GuiElement newElement;
                    newElement.id = objName;
                    newElement.rawLocalBox = curRawLocalBox;
                    newElement.rect.set(0.0f, 0.0f, 1.0f, 1.0f);
                    node->Append(newElement);

                    // go down
                    node = &node->Child(node->Size()-1);
                    guiLevel.Push(curLevel);                    
                }
            }
            else if (FourCC('SLCB') == fourCC)
            {
                // setlocalbox
                float4 mid, extents;
                mid.x() = reader->ReadFloat();
                mid.y() = reader->ReadFloat();
                mid.z() = reader->ReadFloat();                 
                extents.x() = reader->ReadFloat();
                extents.y() = reader->ReadFloat();
                extents.z() = reader->ReadFloat();
                curRawLocalBox.set(mid, extents);
            }
            else if (FourCC('SPOS') == fourCC)
            {
                // read position
                point pos;
                pos.x() = reader->ReadFloat();
                pos.y() = reader->ReadFloat();
                pos.z() = reader->ReadFloat();
                node->Value().rawTransform.setposition(pos);
            }
            else if (FourCC('SSCL') == fourCC)
            {
                // read scale
                vector scale;
                scale.x() = reader->ReadFloat();
                scale.y() = reader->ReadFloat();
                scale.z() = reader->ReadFloat();
                node->Value().rawTransform.setscale(scale);
            }
            else if (FourCC('SSCP') == fourCC)
            {
                // read scale pivot
                point scalePivot;
                scalePivot.x() = reader->ReadFloat();
                scalePivot.y() = reader->ReadFloat();
                scalePivot.z() = reader->ReadFloat();
                node->Value().rawTransform.setscalepivot(scalePivot);
            }
            else if (FourCC('SSTA') == fourCC)
            {
                String attrName = reader->ReadString();
                String value = reader->ReadString();      
                // in N3 name of element is equal to event name 
                if (value != "Generic" && attrName == "rlGuiEvent")
                {                       
                    node->Value().event = value;
                }
                else if (attrName == "rlGuiType")
                {
                    node->Value().guiType = value;                    
                }
            }
            else if (FourCC('STXT') == fourCC)
            {
                String semantic = reader->ReadString();
                String texPath = reader->ReadString();
                if (semantic == "DiffMap0" && node->HasParent())
                {                               
                    texPath.SubstituteString("textures:", "");
                    texPath.StripFileExtension();
                    if (objName == "pressed")
                    {
                        node->Value().pressedTexture = texPath;
                    }
                    else if (objName == "disabled")
                    {
                        node->Value().disabledTexture = texPath;
                    }
                    else if (objName == "mouseover")
                    {
                        node->Value().mouseOverTexture = texPath;
                    }
                    else if (objName == "normal")
                    {
                        node->Value().texture = texPath;
                    }
                    else
                    {
                        // set global Window texture
                        node->Parent().Value().texture = texPath;
                    }
                }                   
            }
            else if (FourCC('SMSH') == fourCC)
            {
                meshPath = reader->ReadString();
                meshPath.SubstituteString("meshes:", String(this->mshDir + "/"));
            }
            else if (FourCC('SGRI') == fourCC)
            {
                uint groupIndex = reader->ReadUInt();
                if (objName.IsValid())
                {     
                    // get uv coordinates
                    Math::float4 uvs = this->ReadUvCoordinates(meshPath, groupIndex);
                    //n_printf("Write %s uvs: %s, of node %s\n\n", objName.AsCharPtr(), String::FromFloat4(uvs).AsCharPtr(), node->Value().id.AsCharPtr());
                    // save coordinates
                    if (objName == "normal")
                    {
                        node->Value().uv[0] = uvs;
                        objName = "";
                    }
                    else if (objName == "mouseover")
                    {
                        node->Value().uv[1] = uvs;
                        objName = "";
                    }
                    else if (objName == "pressed")
                    {
                        node->Value().uv[2] = uvs;
                        objName = "";
                    }
                    else if (objName == "disabled")
                    {
                        node->Value().uv[3] = uvs;
                        objName = "";
                    }         
                }
            }
            else
            {     
                // skip non bool attr
                reader->GetStream()->Seek(length, Stream::Current);
            }
        }         
    }   

    if (this->nvx2Reader->IsOpen())
    {
        this->nvx2Reader->Close();
    }
    this->nvx2Reader = 0;

    IoServer* ioServer = IoServer::Instance();
    ioServer->CreateDirectory("dst:ui");
    String xmlPath;
    xmlPath.Format("dst:%s/%s", "ui", this->dstPath.ExtractFileName().AsCharPtr());
    xmlPath.ChangeFileExtension("xml");
    Ptr<Stream> dstStream = ioServer->CreateStream(xmlPath);
    Ptr<XmlWriter> xmlWriter = XmlWriter::Create();
    xmlWriter->SetStream(dstStream);

    if (xmlWriter->Open())
    {
        xmlWriter->BeginNode("Nebula3");
        xmlWriter->SetString("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");

        SimpleTree<GuiElement>::Node& node = this->windowTree.Root();            
        SizeT numChildren = node.Size();
        if (numChildren > 0)
        {                   
            xmlWriter->BeginNode("Window");
            xmlWriter->SetString("title", "Window");
            xmlWriter->SetString("xAlign", "CenterX");
            xmlWriter->SetString("yAlign", "CenterY");
            xmlWriter->SetString("texture", node.Child(0).Value().texture);
            xmlWriter->SetFloat("fadeIn", 0.5f);
            xmlWriter->SetFloat("fadeOut", 0.5f);
            IndexT i;
            for (i = 0; i < numChildren; ++i)
            {
                this->WriteXmlGuiNode(xmlWriter, node.Child(i));
            }                    
            xmlWriter->EndNode();
        }      
        xmlWriter->EndNode();           
        xmlWriter->Close();
    }
    this->windowTree.Root().Clear();
}

//------------------------------------------------------------------------------
/**
*/
Math::float4 
N2Converter::ReadUvCoordinates(const IO::URI& uri, IndexT meshGroup)
{
    float4 uvs(0,0,1,1);
            
    if (!nvx2Reader->GetStream().isvalid()
        || nvx2Reader->GetStream()->GetURI() != uri)
    {
        if (nvx2Reader->IsOpen())
        {
            nvx2Reader->Close();
        }     
        IoServer* ioServer = IoServer::Instance();
        Ptr<Stream> stream = ioServer->CreateStream(uri);
        nvx2Reader->SetStream(stream);
        nvx2Reader->SetUsage(Base::ResourceBase::UsageCpu);
        nvx2Reader->SetAccess(Base::ResourceBase::AccessRead);
        nvx2Reader->SetRawMode(true);
        bool success = nvx2Reader->Open();     
        n_assert(success);
    }

    System::ByteOrder byteOrder(nvx2Reader->GetSourcePlatformEndianess(), System::ByteOrder::Host);
    
    // get uvs
    const Util::Array<CoreGraphics::PrimitiveGroup>& primitiveGroups = nvx2Reader->GetPrimitiveGroups();
    n_assert(meshGroup < primitiveGroups.Size());
    IndexT baseIndex = primitiveGroups[meshGroup].GetBaseIndex();
    n_assert(primitiveGroups[meshGroup].GetNumIndices() >= 6);
    ushort* indexPtr = nvx2Reader->GetIndexData();
    char* vertexPtr = (char*)nvx2Reader->GetVertexData();
    ushort uvIndex0 = byteOrder.Convert(indexPtr[baseIndex + 0]);
    ushort uvIndex5 = byteOrder.Convert(indexPtr[baseIndex + 5]);
    const Util::Array<CoreGraphics::VertexComponent>& components = nvx2Reader->GetVertexComponents();
    SizeT texCoordSize = 0;
    SizeT texCoordOffset = 0;  
    SizeT vertexWidth = 0;      
    IndexT i;
    for (i = 0; i < components.Size(); ++i)
    {
        if (texCoordSize == 0)
        {
            if (components[i].GetSemanticName() != CoreGraphics::VertexComponent::TexCoord1
                || components[i].GetSemanticIndex() != 0)        	
            {                                                              
                texCoordOffset += components[i].GetByteSize();
            }
            else
            {
                texCoordSize = components[i].GetByteSize() / 2;                
            }
        }
        vertexWidth += components[i].GetByteSize();
    }         
    char* vertex0 = vertexPtr + uvIndex0 * vertexWidth;
    char* vertex5 = vertexPtr + uvIndex5 * vertexWidth;
    n_assert(texCoordOffset > 0 && texCoordSize > 0);

    uvs.x() = (float)byteOrder.Convert(*(ushort*)(vertex0 + texCoordOffset)) / 8192.0f;
    uvs.y() = (float)byteOrder.Convert(*(ushort*)(vertex5 + texCoordOffset + texCoordSize))/ 8192.0f;

    uvs.z() = (float)byteOrder.Convert(*(ushort*)(vertex5 + texCoordOffset))/ 8192.0f;
    uvs.w() = (float)byteOrder.Convert(*(ushort*)(vertex0 + texCoordOffset + texCoordSize))/ 8192.0f;

    return uvs;
}

//------------------------------------------------------------------------------
/**
    Update the position and size members of a GUINode from
    the rawLocalBox, rawPosition and rawSize.
*/
void
N2Converter::ComputeGUINodeDimensions(SimpleTree<GuiElement>::Node& node)
{
    // need to compute flattened parent hierarchy transform
    matrix44 m = node.Value().rawTransform.getmatrix();
    SimpleTree<GuiElement>::Node* nodePtr = &node;
    while (nodePtr->HasParent())
    {
        m = matrix44::multiply(m, nodePtr->Parent().Value().rawTransform.getmatrix());
        nodePtr = &(nodePtr->Parent());
    }

    // transform local bounding box
    bbox box = node.Value().rawLocalBox;
    box.transform(m);

    // n2 gui has 4:3 range from x: -2 -> 2
    //                       and y: -1.5 -> 1.5
    // scale to left/top: 0 -> right/bottom: 1.0

    node.Value().rect.left   = (box.pmin.x() / 4.0f) + 0.5f;
    node.Value().rect.right  = (box.pmax.x() / 4.0f) + 0.5f;
    node.Value().rect.top    = 1.0f - ((box.pmax.y() / 3.0f) + 0.5f);
    node.Value().rect.bottom = 1.0f - ((box.pmin.y() / 3.0f) + 0.5f);
}

//------------------------------------------------------------------------------
/**
*/
void 
N2Converter::WriteXmlGuiNode(const Ptr<XmlWriter>& writer, Util::SimpleTree<GuiElement>::Node& node)
{
    // compute node dimensions
    this->ComputeGUINodeDimensions(node);

    // write to XML stream
    writer->BeginNode(this->MapGuiType(node.Value().guiType));
    writer->SetString("id", node.Value().id);     
    if (node.Value().event.IsValid())
    {
        writer->SetString("event", node.Value().event);
    }
    float4 r;
    r.x() = node.Value().rect.left;
    r.y() = node.Value().rect.top;
    r.z() = node.Value().rect.right;
    r.w() = node.Value().rect.bottom;
    writer->SetFloat4("rect", r);
    writer->SetFloat4("defaultUV", node.Value().uv[0]);
    if (node.Value().texture.IsValid())
    {
        writer->SetString("texture", node.Value().texture);
    }
    if (node.Value().guiType == "Canvas")
    {
        // supported, but no extra parameters
    }
    else if (node.Value().guiType == "Frame")
    {
        // supported, but no extra parameters
    }
    else if (node.Value().guiType == "Button")
    {      
        writer->SetFloat4("mouseOverUV", node.Value().uv[1]);
        writer->SetFloat4("pressedUV", node.Value().uv[2]);
        writer->SetFloat4("disabledUV", node.Value().uv[3]); 
        if (node.Value().pressedTexture.IsValid())
        {
            writer->SetString("pressedTexture", node.Value().pressedTexture);
        }
        if (node.Value().disabledTexture.IsValid())
        {
            writer->SetString("disabledTexture", node.Value().disabledTexture);
        }
        if (node.Value().mouseOverTexture.IsValid())
        {
            writer->SetString("mouseOverTexture", node.Value().disabledTexture);
        }
    }
    else
    {
        // node not supported
    }

    SizeT numChildren = node.Size();
    IndexT i;
    for (i = 0; i < numChildren; ++i)
    {
        this->WriteXmlGuiNode(writer, node.Child(i));
    }
    writer->EndNode();
}

//------------------------------------------------------------------------------
/**
*/
Util::String 
N2Converter::MapGuiType(const Util::String& n2Type)
{
    if (n2Type == "Canvas")
    {
        return "Canvas";
    }
    else if (n2Type == "Frame")
    {
        return "Frame";
    }
    else if ((n2Type == "Button") || (n2Type == "TextButton") || (n2Type == "CheckButton"))
    {
        return "Button";
    }
    else if ((n2Type == "Label") || (n2Type == "TextLabel"))
    {
        return "Label";
    }
    else
    {
        return "Frame";
    }
}

//------------------------------------------------------------------------------
/**
*/
N2Converter::GuiElement::GuiElement()
{
    this->rect.set(0.0f, 0.0f, 0.0f, 0.0f);
    uv[0] = Math::vector::nullvec();
    uv[1] = Math::vector::nullvec();
    uv[2] = Math::vector::nullvec();
    uv[3] = Math::vector::nullvec();
}
} // namespace ToolkitUtil