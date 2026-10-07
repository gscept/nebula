#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::N2Converter
    
    A utility class to convert N2 binary object files to Nebula3 object
    files.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/platform.h"
#include "io/uri.h"
#include "io/xmlwriter.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "util/stack.h"
#include "io/stream.h"
#include "io/binaryreader.h"
#include "toolkitutil/modelwriter.h"
#include "util/variant.h"
#include "util/simpletree.h"
#include "toolkitutil/logger.h"                  
#include "n2util/nvx2orderfreestreamreader.h"
#include "toolkitutil/n2util/n2reflectioninfo.h"
#include "toolkitutil/n2util/n2sceneloader.h"
#include "math/transform44.h"
#include "math/rectangle.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class N2Converter
{
public:
    /// constructor
    N2Converter();
    /// destructor
    ~N2Converter();
    
    /// setup the N2 converter
    void Setup();
    /// discard the N2 converter
    void Discard();
    /// return true if valid
    bool IsValid() const;

    /// set target platform
    void SetPlatform(Platform::Code platform);
    /// set source directory
    void SetSrcDir(const Util::String& srcDir);
    /// set destination directory
    void SetDstDir(const Util::String& dstDir);
    /// set the mesh directory (default is dst:meshes)
    void SetMeshDir(const Util::String& mshDir);
    /// set force conversion flag (otherwise check timestamps)
    void SetForceFlag(bool b);
    /// set binary mode
    void SetBinaryFlag(bool b);
    /// set verbosity on
    void SetVerbose(bool b);

    /// reset the used resources array
    void ResetUsedResources();
    /// get the resources of converted objects
    const Util::Array<Util::String>& GetUsedResources() const;
    /// get pointer to N2ReflectionInfo object
    const Ptr<N2ReflectionInfo>& GetN2ReflectionInfo() const;

    /// convert a single N2 file
    bool ConvertFile(Logger& logger, const Util::String& category, const Util::String& srcFile);
    /// convert all files in a category
    bool ConvertCategory(Logger& logger, const Util::String& category);
    /// convert all files
    bool ConvertAll(Logger& logger);

private:
    /// supported Nebula2 node types
    enum NodeType
    {
        TransformNode = 0,
        ShapeNode,
        Character3Node,
        Character3SkinAnimator,
        Character3SkinShapeNode,
        EmbeddedNodeType,
        AnimatorNode,
        ParticleSystemNode,        

        InvalidNodeType,
    };

    enum AnimatorNodeType
    {
        IntAnimator = 0,
        FloatAnimator,
        Float4Animator,
        TransformAnimator,
        TransformCurveAnimator,
        UvAnimator,

        InvalidAnimatorNodeType,
    };

    struct KeyValues
    {
        float time;
        Util::Variant value;
    };

    struct AnimatorInfo
    {
        Util::String loopType;
        AnimatorNodeType animatorNode;
        Util::String animation;
        Util::Array<KeyValues> key;
        Util::String paramName;
        Util::String vectorName;
        Util::Array<KeyValues> posKey;
        Util::Array<KeyValues> eulerKey;
        Util::Array<KeyValues> scaleKey;
        int animationGroup;
        Util::Array<int> layer;
    };    

    /// test if a conversion is needed (checks file time stamps and force flag)
    bool NeedsConversion(const Util::String& srcPath, const Util::String& dstPath);
    /// perform conversion for a single file
    bool PerformConversion(const Util::String& modelName, const Ptr<IO::BinaryReader>& reader, const Ptr<ModelWriter>& writer);
    /// actual node-parser
    bool ParseNodes(const Ptr<IO::BinaryReader>& reader, const Ptr<ModelWriter>& writer);
    /// called when a new node is encountered in the source file
    void BeginNode(const Util::String& objClass, const Util::String& objName, const Ptr<ModelWriter>& writer);
    /// called when a node is left in the source file
    void EndNode(const Ptr<ModelWriter>& writer);
    /// called for each data element encountered in the source file
    void ReadDataTag(Util::FourCC fourCC, const Ptr<IO::BinaryReader>& reader, const Ptr<ModelWriter>& writer);
    /// map a shader name to a ModelNodeType
    Util::String MapShaderNameToModelNodeType(const Util::String& shaderName);
    /// convert Nebula2 shader name to Nebula3 shader name
    Util::String ConvertShaderName(const Util::String& shaderName);
    /// convert skin lists of a character3 (skin lists are written to the n3 file)
    bool ConvertSkinLists(const Ptr<ModelWriter>& writer);
   /// write a particle system envelope curve tag
    void WriteParticleEnvelopeCurve(const Util::String& tag, Util::FourCC fourCC, const Ptr<IO::BinaryReader>& reader, const Ptr<ModelWriter>& writer);
    /// write a particle system RGB envelope curve tag
    void WriteParticleRGBEnvelopeCurve(const Util::String& redTag, Util::FourCC redFourCC, const Util::String& greenTag, Util::FourCC greenFourCC, const Util::String& blueTag, Util::FourCC blueFourCC, const Ptr<IO::BinaryReader>& reader, const Ptr<ModelWriter>& writer);
    /// write the animatornodes to nebula3-file
    void WriteAnimatorNode(const Ptr<ModelWriter>& writer);
    /// add an additional information to dictionary
    void FillAnimatorInformation();
    /// Set Animator Path
    void SetAnimatorPath(Util::String objName, Util::String objClass);
    /// convert from string to animatorNode
    AnimatorNodeType FromStringToAnimatorNode(Util::String animNode);
    /// Converts Relative AnimatorPath to Absolute AnimatorPath
    Util::String ConvertRelNodePathToAbsolute(Util::String relPath, bool pathToAnimatorNode=false);
    /// Paths are saved in Array of String-> Converts paths from Array to simpleString 
    Util::String ConvertNodePathFromArrayToString(Util::Array<Util::String> stringArray);
    /// convert shader semantic name
    Util::String ConvertShaderSemantic(const Util::String& rhs) const;
    /// convert animation resource id
    Util::String ConvertAnimationName(const Util::String& anim) const;
    /// add a used resource
    void AddUsedResource(const Util::String& resId);
    /// substitute Nebula2 resource assigns by N3 assigns
    Util::String SubstituteResourceAssigns(const Util::String& rhs);

    Platform::Code platform;
    Util::String srcDir;
    Util::String dstDir;
    Util::String mshDir;
    Util::String srcPath;
    Util::String dstPath;
    bool force;
    bool binary;
    bool verbose;
    bool isValid;
    NodeType nodeType;
    Util::Stack<NodeType> nodeTypeStack;

    Ptr<N2ReflectionInfo> n2ReflectionInfo;
    Ptr<N2SceneLoader> n2SceneLoader;

    Util::FixedArray<IndexT> skinFragmentPrimGroupIndices;
    Util::FixedArray<Util::FixedArray<IndexT>> skinFragmentJointPalettes;

    Util::Dictionary<Util::String, Util::Array<Util::String> > animatedNodesToAnimator;
    Util::Dictionary<Util::String, AnimatorInfo> animatorInformation;
    Util::Array<Util::String> path;
    Util::String lastAnimatorName;
    AnimatorInfo animInfo;
    AnimatorNodeType animNodeType;
    Util::Array<Util::String> usedResources;
       
    // gui parse elements
    class GuiElement
    {
    public:
        GuiElement();

        Util::String guiType;
        Util::String id;
        Util::String event;
        Util::String texture;
        Util::String pressedTexture;
        Util::String mouseOverTexture;
        Util::String disabledTexture;
        Util::String xAlignment;
        Util::String yAlignment;
        
        Math::bbox rawLocalBox;
        Math::transform44 rawTransform;

        Math::rectangle<float> rect;
        Math::float4 uv[4];
        bool ignoreInput;
    };
    Util::SimpleTree<GuiElement> windowTree;
    Util::Dictionary<Util::StringAtom, Util::StringAtom> alignmentTable;
    Ptr<Legacy::Nvx2OrderFreeStreamReader> nvx2Reader;

    /// check if n2 model is a gui scene
    bool IsGuiScene(const Ptr<IO::BinaryReader>& reader);
    /// convert gui scene
    void ConvertGuiScene(const Ptr<IO::BinaryReader>& reader, const Ptr<ModelWriter>& writer);
    /// read uv coordinates from mesh file
    Math::float4 ReadUvCoordinates(const IO::URI& uri, IndexT meshGroup);
    /// write xml gui node
    void WriteXmlGuiNode(const Ptr<IO::XmlWriter>& writer, Util::SimpleTree<GuiElement>::Node& node);
    /// map n2 gui type to n3
    Util::String MapGuiType(const Util::String& n2Type);
    /// compute dimensions of GUI node, take parent nodes into account
    void ComputeGUINodeDimensions(Util::SimpleTree<GuiElement>::Node& node);
};

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::SetVerbose(bool b)
{
    this->verbose = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::SetPlatform(Platform::Code p)
{
    this->platform = p;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::SetSrcDir(const Util::String& dir)
{
    this->srcDir = dir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::SetDstDir(const Util::String& dir)
{
    this->dstDir = dir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::SetMeshDir(const Util::String& dir)
{
    this->mshDir = dir;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::SetForceFlag(bool b)
{
    this->force = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::SetBinaryFlag(bool b)
{
    this->binary = b;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::ResetUsedResources()
{
    this->usedResources.Clear();
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Util::String>&
N2Converter::GetUsedResources() const
{
    return this->usedResources;
}

//------------------------------------------------------------------------------
/**
*/
inline void
N2Converter::AddUsedResource(const Util::String& resId)
{
    if (InvalidIndex == this->usedResources.BinarySearchIndex(resId))
    {
        this->usedResources.InsertSorted(resId);
    }
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<N2ReflectionInfo>&
N2Converter::GetN2ReflectionInfo() const
{
    return this->n2ReflectionInfo;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    
