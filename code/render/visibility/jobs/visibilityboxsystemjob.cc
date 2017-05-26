//------------------------------------------------------------------------------
//  charevalskeletonjob.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "jobs/stdjob.h"
#include "math/float4.h"
#include "math/matrix44.h"
#include "math/bbox.h"
#include "math/frustum.h"
#include "visibility/observercontext.h"
#include "visibility/visibilitysystems/visibilityboxsystem.h"

namespace Visibility
{
using namespace Math;
using namespace Util;
using namespace Graphics;

//------------------------------------------------------------------------------
/**
*/
void 
UpdateBoundingBoxes(const matrix44* observerFrustum,
                    const Math::point& observerPosition,
                    VisibilityBox::VisibilityBoxJobData* visBoxes,
                    SizeT numBoxes,
                    IndexT* neighbors)
{   
    SizeT numCameraBoxes = 0;
    // first clear the visibility of all visibility boxes
    IndexT i;
    for (i = 0; i < numBoxes; i++)
    {           
        visBoxes[i].isVisible = false;
        visBoxes[i].isCameraBox = false;
        visBoxes[i].isProcessed = false;
    }

    // first pass: find all first order boxes (which contain the camera position)
    for (i = 0; i < numBoxes; i++)
    {
        if (visBoxes[i].boxFrustum.inside(observerPosition))
        {
            visBoxes[i].isCameraBox = true;
            numCameraBoxes++;
        }
    }

    // if camera is not inside any box, need to make all boxes visible
    if (0 == numCameraBoxes)
    {
        for (i = 0; i < numBoxes; i++)
        {
            visBoxes[i].isVisible = true;
        }
    }
    else
    {
        // second pass: set visibility flag on all camera boxes, and their
        // direct neighbours
        for (i = 0; i < numBoxes; i++)
        {
            SizeT numNeighbors = *neighbors;
            neighbors++;
            if (visBoxes[i].isCameraBox)
            {
                // all camera boxes are definitely visible
                visBoxes[i].isVisible = true;
                visBoxes[i].isProcessed = true;

                // scan direct neighbours, set them to visible if they are
                // in the view frustum
                IndexT neighbourIndex;
                for (neighbourIndex = 0; neighbourIndex < numNeighbors; neighbourIndex++)
                {
                    IndexT nIndex = neighbors[neighbourIndex];
                    VisibilityBox::VisibilityBoxJobData* neigborBox = &visBoxes[nIndex];

                    // skip already processed boxes
                    if (!neigborBox->isProcessed)
                    {
                        neigborBox->isProcessed = true;                         
                        frustum viewFrustum = frustum(*observerFrustum);                            
                        const static bbox unitCubeBox(point(0.0f, 0.0f, 0.0f), vector(0.5f, 0.5f, 0.5f));
                        ClipStatus::Type clipResult = viewFrustum.clipstatus(unitCubeBox, neigborBox->transform);                            
                        if (ClipStatus::Outside != clipResult)
                        {
                            neigborBox->isVisible = true;                            
                        }
                    }
                }
            }
            neighbors += numNeighbors;
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityBoxSystemJobFunc(const JobFuncContext& ctx)
{
    VisibilityBox::VisibilityBoxJobData* visBoxes = (VisibilityBox::VisibilityBoxJobData*)ctx.uniforms[0];    
    SizeT numBoxes = ctx.uniformSizes[0];
    IndexT* neighbors = (IndexT*)ctx.uniforms[1];
    SizeT numNeighbors = ctx.uniformSizes[1];
    Ptr<VisibilityContext>* output = (Ptr<VisibilityContext>*)ctx.outputs[0];
    SizeT numOutputs = ctx.outputSizes[0];
    const Math::point& observerPosition = *(Math::point*)ctx.inputs[0];
    uint entityTypeMask = *(uint*)ctx.inputs[1];
    uint curCellIndex = 0;
    uint curEntitiesIndex = 0;

    // update visibility boxes for this observer     
    matrix44* observerFrustum = (matrix44*)ctx.inputs[2]; 
    UpdateBoundingBoxes(observerFrustum, observerPosition, visBoxes, numBoxes, neighbors);

    IndexT* boxMapping = (IndexT*)ctx.inputs[3];
    SizeT numBoxMappings = ctx.inputSizes[3];
    // now go thru visiblecontexts, checked before
    // cause this vis system comes after other systems, we work on the outEntitiyArray
    // and throw out any non visible contexts
    IndexT contextIdx;
    for (contextIdx = 0; contextIdx < numOutputs; ++contextIdx)
    {           
        if (output[contextIdx].isvalid())
        { 
            // FIXME: optimize search for box mapping 
            IndexT* mPtr = boxMapping; 
            IndexT boxMapIdx;
            for (boxMapIdx = 0; boxMapIdx < numBoxMappings; ++boxMapIdx)
            {                   
                uint boxMappingPtr = *mPtr;
                mPtr++;
                SizeT numBoxesForContext = *mPtr;
                mPtr++;
                if (numBoxesForContext > 0)
                {   
                    uint contextPtr = (uint)output[contextIdx].get();
                    if (boxMappingPtr == contextPtr)
                    {   
                        IndexT boxMappingIdx;
                        for (boxMappingIdx = 0; boxMappingIdx < numBoxesForContext; ++boxMappingIdx)
                        {  
                            const VisibilityBox::VisibilityBoxJobData& box = visBoxes[mPtr[boxMappingIdx]];                
                            if (!box.isVisible)
                            {
                                output[contextIdx] = 0;
                                break;
                            }  
                        }  
                    }
                    mPtr += numBoxesForContext;
                }            	
            }
             
        }
    }
}

} // namespace Visibility
__ImplementSpursJob(Visibility::VisibilityBoxSystemJobFunc);

