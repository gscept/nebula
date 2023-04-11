//------------------------------------------------------------------------------
//  animbuildercurve.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "model/animutil/animbuildercurve.h"

namespace ToolkitUtil
{

using namespace CoreAnimation;

//------------------------------------------------------------------------------
/**
*/
AnimBuilderCurve::AnimBuilderCurve() 
    : firstKeyOffset(0)
    , numKeys(0)
    , preInfinityType(InfinityType::Code::InvalidInfinityType)
    , postInfinityType(InfinityType::Code::InvalidInfinityType)
    , curveType(CurveType::Code::InvalidCurveType)
{
    // empty
}
 

} // namespace ToolkitUtil