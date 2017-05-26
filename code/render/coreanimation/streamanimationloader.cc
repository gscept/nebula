//------------------------------------------------------------------------------
//  streamanimationloader.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coreanimation/streamanimationloader.h"
#include "coreanimation/animresource.h"
#include "system/byteorder.h"
#include "coreanimation/naxfileformatstructs.h"

namespace CoreAnimation
{
__ImplementClass(CoreAnimation::StreamAnimationLoader, 'SANL', Resources::StreamResourceLoader);

using namespace IO;
using namespace Util;
using namespace System;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
bool
StreamAnimationLoader::SetupResourceFromStream(const Ptr<Stream>& stream)
{
    n_assert(stream.isvalid());
    n_assert(this->resource.isvalid());
    return this->SetupFromNax3(stream);
}

//------------------------------------------------------------------------------
/**
*/
bool
StreamAnimationLoader::SetupFromNax3(const Ptr<Stream>& stream)
{
    const Ptr<AnimResource>& anim = this->resource.downcast<AnimResource>();
    n_assert(!anim->IsLoaded());
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        uchar* ptr = (uchar*) stream->Map();

        // read header
        Nax3Header* naxHeader = (Nax3Header*) ptr;
        ptr += sizeof(Nax3Header);

        // check magic value
        if (FourCC(naxHeader->magic) != NEBULA3_NAX3_MAGICNUMBER)
        {
            n_error("StreamAnimationLoader::SetupFromNax3(): '%s' has invalid file format (magic number doesn't match)!", stream->GetURI().AsString().AsCharPtr());
            return false;
        }

		// load animation if it has clips in it
		if (naxHeader->numClips > 0)
		{
			// setup animation clips
			anim->BeginSetupClips(naxHeader->numClips);
			IndexT clipIndex;
			SizeT numClips = (SizeT)naxHeader->numClips;
			for (clipIndex = 0; clipIndex < numClips; clipIndex++)
			{
				Nax3Clip* naxClip = (Nax3Clip*)ptr;
				ptr += sizeof(Nax3Clip);

				// setup anim clip object
				AnimClip& clip = anim->Clip(clipIndex);
				clip.SetNumCurves(naxClip->numCurves);
				clip.SetStartKeyIndex(naxClip->startKeyIndex);
				clip.SetNumKeys(naxClip->numKeys);
				clip.SetKeyStride(naxClip->keyStride);
				clip.SetKeyDuration(naxClip->keyDuration);
				clip.SetPreInfinityType((InfinityType::Code)naxClip->preInfinityType);
				clip.SetPostInfinityType((InfinityType::Code)naxClip->postInfinityType);
				clip.SetName(naxClip->name);

				// add anim events
				clip.BeginEvents(naxClip->numEvents);
				IndexT eventIndex;
				for (eventIndex = 0; eventIndex < naxClip->numEvents; eventIndex++)
				{
					Nax3AnimEvent* naxEvent = (Nax3AnimEvent*)ptr;
					ptr += sizeof(Nax3AnimEvent);
					AnimEvent animEvent(naxEvent->name, naxEvent->category, naxEvent->keyIndex * clip.GetKeyDuration());
					clip.AddEvent(animEvent);
				}
				clip.EndEvents();

				// setup anim curves
				IndexT curveIndex;
				for (curveIndex = 0; curveIndex < naxClip->numCurves; curveIndex++)
				{
					Nax3Curve* naxCurve = (Nax3Curve*)ptr;
					ptr += sizeof(Nax3Curve);

					AnimCurve& animCurve = clip.CurveByIndex(curveIndex);
					animCurve.SetFirstKeyIndex(naxCurve->firstKeyIndex);
					animCurve.SetActive(naxCurve->isActive != 0);
					animCurve.SetStatic(naxCurve->isStatic != 0);
					animCurve.SetCurveType((CurveType::Code)naxCurve->curveType);
					animCurve.SetStaticKey(float4(naxCurve->staticKeyX, naxCurve->staticKeyY, naxCurve->staticKeyZ, naxCurve->staticKeyW));
				}
			}
			anim->EndSetupClips();

			// load keys
			const Ptr<AnimKeyBuffer>& animKeyBuffer = anim->SetupKeyBuffer(naxHeader->numKeys);
			void* keyPtr = animKeyBuffer->Map();
			Memory::Copy(ptr, keyPtr, animKeyBuffer->GetByteSize());
			animKeyBuffer->Unmap();
		}
        
        // shutdown stream
        stream->Unmap();
        stream->Close();
        return true;
    }
    return false;
}

} // namespace CoreAnimation
