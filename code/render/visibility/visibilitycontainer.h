#pragma once
//------------------------------------------------------------------------------
/**
    The visibility container stores the visibility results for entities registered with the VisibilityContext.

    The idea is to map the list of graphics entities registered with the GraphicsServer against a list of 
    visibility results.

    TODO: But how does this map do any kind of sorting? If models are grouped by <shader, material, mesh> for example,
    how does an ordinary array account for that?

    SOLUTION: The array contains results for ALL entities, independent of observer mask, and the visibility status
    is in the same order as the <shader, material, mesh> grouping would be if it was deflated. The <shader, material, model> list 
    is always static during the frame, since it contains ALL shader, material and mesh groupings. The difference with the old system
    is that now, the only thing that changes is the flipping of a boolean value, instead of having to reconstruct the triple tuple list
    every frame. It means however, that everytime a graphics entity is attached, the VisibilityContainer MUST be notified so that
    it may change the size of the visibility array.
    
    (C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
namespace Visibility
{
class VisibilityContainer : public Core::RefCounted
{
    __DeclareClass(VisibilityContainer);
public:
    /// constructor
    VisibilityContainer();
    /// destructor
    virtual ~VisibilityContainer();
private:

    Util::Array<bool> visible;
};
} // namespace Visibility