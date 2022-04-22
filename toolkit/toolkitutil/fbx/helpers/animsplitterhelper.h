#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimSplitterHelper
    
    Sets up animation splits from command line arguments. 
    It works by accepting signatures with the syntax <clipname>;<start>-<stop>.
    For example flying;0-39 would create a clip flying with the offset 0-39
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coreanimation/infinitytype.h"
#include "io/xmlreader.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AnimSplitterHelper : public Core::RefCounted
{
    __DeclareClass(AnimSplitterHelper);
public:
    
    struct Split
    {
        Util::String name;
        int startOffset;
        int endOffset;
        CoreAnimation::InfinityType::Code postInfinity;
        CoreAnimation::InfinityType::Code preInfinity;

        Split()
        {
            postInfinity = CoreAnimation::InfinityType::Constant;
            preInfinity = CoreAnimation::InfinityType::Constant;
        }

    };

    /// constructor
    AnimSplitterHelper();
    /// destructor
    virtual ~AnimSplitterHelper();

    /// sets up the splitter helper
    void Setup(Ptr<IO::XmlReader> reader);

    /// gets take by name
    const Util::Array<Split>& GetTake(const Util::String& take);
    /// returns true if take exists
    bool HasTake(const Util::String& take);

private:
    
    Util::String batch;
    Util::Dictionary<Util::String, Util::Array<Split> > takes;
}; 

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<AnimSplitterHelper::Split>& 
AnimSplitterHelper::GetTake( const Util::String& take )
{
    return this->takes[take];
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
AnimSplitterHelper::HasTake( const Util::String& take )
{
    return this->takes.FindIndex(take) >= 0;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------