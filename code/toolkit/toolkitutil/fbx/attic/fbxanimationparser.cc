//------------------------------------------------------------------------------
//  fbxanimationparser.cc
//  (C) 2011 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbxanimationparser.h"
#include "toolkitutil/animutil/animbuildercurve.h"
#include "math/float4.h"
#include "coreanimation/curvetype.h"
#include "coreanimation/infinitytype.h"
#include "base/exporterbase.h"

#define KEYSPERMS 40

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::FBXAnimationParser, 'FBXA', ToolkitUtil::FBXParserBase);

using namespace ToolkitUtil;
using namespace Util;
using namespace Math;
using namespace CoreAnimation;
//------------------------------------------------------------------------------
/**
*/
FBXAnimationParser::FBXAnimationParser() : 
	skeleton(0),
	splitter(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
FBXAnimationParser::~FBXAnimationParser()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXAnimationParser::Parse( KFbxScene* scene, AnimBuilder* animBuilder /* = 0 */ )
{
	n_assert(this->skeleton || this->mode == Static);


	int animStackCount = scene->GetSrcObjectCount(FBX_TYPE(KFbxAnimStack));
	for (int animStackIndex = 0; animStackIndex < animStackCount; animStackIndex++)
	{
		KFbxAnimStack* animStack = scene->GetSrcObject(FBX_TYPE(KFbxAnimStack), animStackIndex);
		String animStackname = animStack->GetName();

		this->exporter->Progress(0, "Exporting: " + animStackname);

		// for some reason, Nebula looses track of animations if their names contain space...
		int animLayerCount = animStack->GetSrcObjectCount(FBX_TYPE(KFbxAnimLayer));

		// we only need the base (which contains the sum of all layers)
		KFbxAnimLayer* animLayer = animStack->GetSrcObject(FBX_TYPE(KFbxAnimLayer), 0);
		String animLayerName = animLayer->GetName();
		n_printf("Exporting animation clip: %s \n", animLayerName.AsCharPtr());
		int animCurveNodeCount = animLayer->GetSrcObjectCount(FBX_TYPE(KFbxAnimCurveNode));

		// we set keyspan to 1 because we have static curves, which means we have a minimum of 1 key
		int keySpan = 1;
		int preInfinityType = 0;
		int postInfinityType = 0;

		// we need to find the frame with the most keys (to be able to decide how long the clip should be)

		Util::Array<AnimBuilderCurve> curves;
		// recursively traverse skeleton to find animated nodes

		if (this->mode == Joints)
		{
			this->GetMaximumKeySpanFromSkeleton(this->skeleton->root->fbxNode, animLayer, keySpan);
			this->ConstructAnimationCurvesFromSkeleton(this->skeleton->root->fbxNode, animLayer, curves, preInfinityType, postInfinityType, keySpan);
		}
		else
		{
			this->ConstructAnimationCurvesFromScene(scene, animLayer, curves, preInfinityType, postInfinityType, keySpan);
		}

		// the splitter doesn't need to have any splits. if it doesn't, it will treat the clips as normal takes
		if (this->splitter.isvalid() && this->splitter->HasTake(animStackname))
		{
			Util::Array<AnimSplitterHelper::Split> splits = this->splitter->GetTake(animStackname);
			for (int splitIndex = 0; splitIndex < splits.Size(); splitIndex++)
			{
				const AnimSplitterHelper::Split& split = splits[splitIndex];
				AnimBuilderClip clip;
				clip.SetName(split.name);
				clip.SetKeyDuration(KEYSPERMS);
				int clipKeyCount = split.endOffset - split.startOffset;
				int jointCount = this->skeleton->joints.Size();
				clip.SetNumKeys(clipKeyCount);
				clip.SetPreInfinityType(split.preInfinity);
				clip.SetPostInfinityType(split.postInfinity);
				clip.SetStartKeyIndex(0);
				String s;
				s.Format("Splitting: %s into clip: %s", animStackname.AsCharPtr(), clip.GetName().AsString().AsCharPtr());
				this->exporter->Progress(0, s);

				for (int curveIndex = 0; curveIndex < curves.Size(); curveIndex+=3)
				{
					const AnimBuilderCurve& curveX = curves[curveIndex];
					const AnimBuilderCurve& curveY = curves[curveIndex+1];
					const AnimBuilderCurve& curveZ = curves[curveIndex+2];

					AnimBuilderCurve splitX;
					AnimBuilderCurve splitY;
					AnimBuilderCurve splitZ;

					splitX.SetCurveType(curveX.GetCurveType());
					splitY.SetCurveType(curveY.GetCurveType());
					splitZ.SetCurveType(curveZ.GetCurveType());

					if (curveX.IsStatic())
					{
						splitX.SetStaticKey(curveX.GetStaticKey());
						splitX.SetStatic(true);
						splitX.SetActive(true);
					}
					else
					{
						splitX.ResizeKeyArray(clipKeyCount);
						splitX.SetActive(true);
						splitX.SetStatic(false);
					}
					if (curveY.IsStatic())
					{
						splitY.SetStaticKey(curveY.GetStaticKey());
						splitY.SetStatic(true);
						splitX.SetActive(true);
					}
					else
					{
						splitY.ResizeKeyArray(clipKeyCount);
						splitY.SetActive(true);
						splitY.SetStatic(false);
					}
					if (curveZ.IsStatic())
					{
						splitZ.SetStaticKey(curveZ.GetStaticKey());
						splitZ.SetStatic(true);
						splitX.SetActive(true);
					}
					else
					{
						splitZ.ResizeKeyArray(clipKeyCount);
						splitZ.SetActive(true);
						splitZ.SetStatic(false);
					}

					int splitIndex = 0;
					for (int keyIndex = split.startOffset; (keyIndex < split.endOffset) && (keyIndex < keySpan); keyIndex++)
					{
						if (!splitX.IsStatic())
						{
							splitX.SetKey(splitIndex, curveX.GetKey(keyIndex));
						}
						if (!splitY.IsStatic())
						{
							splitY.SetKey(splitIndex, curveY.GetKey(keyIndex));
						}
						if (!splitZ.IsStatic())
						{
							splitZ.SetKey(splitIndex, curveZ.GetKey(keyIndex));
						}
						splitIndex++;
					}
					
					clip.AddCurve(splitX);
					clip.AddCurve(splitY);
					clip.AddCurve(splitZ);
				}

				animBuilder->AddClip(clip);
			}
		}
		else
		{
			AnimBuilderClip clip;
			int countKeys = 0;
			for (int keyIndex = 0; keyIndex < curves.Size(); keyIndex++)
			{
				clip.AddCurve(curves[keyIndex]);
				countKeys = max(countKeys, curves[keyIndex].GetNumKeys());
			}
			animStackname.ReplaceChars(" ", '_');
			clip.SetName(animStackname);
			clip.SetNumKeys(countKeys);
			clip.SetKeyDuration(KEYSPERMS);
			clip.SetStartKeyIndex(0);
			clip.SetPreInfinityType(preInfinityType == KFbxAnimCurveBase::eCONSTANT ? InfinityType::Constant : InfinityType::Cycle);
			clip.SetPostInfinityType(postInfinityType == KFbxAnimCurveBase::eCONSTANT ? InfinityType::Constant : InfinityType::Cycle);			
			animBuilder->AddClip(clip);
		}

		

		n_printf("Animation clip done!\n");


	}

	//animBuilder->FixInvalidKeyValues();
	//animBuilder->FixAnimCurveFirstKeyIndices();
	//animBuilder->FixInactiveCurveStaticKeyValues();
	animBuilder->BuildVelocityCurves();
	//animBuilder->TrimEnd(1);

}

//------------------------------------------------------------------------------
/**
*/
void 
FBXAnimationParser::ConstructAnimationCurvesFromSkeleton( KFbxNode* fbxNode, KFbxAnimLayer* fbxAnimLayer, Util::Array<AnimBuilderCurve>& curves, int& preInfType, int& postInfType, int keySpan)
{

	// we only want to traverse further if we actually have a skeleton connected to the node
	if (fbxNode->GetSkeleton())
	{
		this->ReadClip(fbxNode, fbxAnimLayer, curves, preInfType, postInfType, keySpan);

		int childCount = fbxNode->GetChildCount();
		for (int childIndex = 0; childIndex < childCount; childIndex++)
		{
			KFbxNode* child = fbxNode->GetChild(childIndex);
			ConstructAnimationCurvesFromSkeleton(child, fbxAnimLayer, curves, preInfType, postInfType, keySpan);
		}
	}	

}


//------------------------------------------------------------------------------
/**
*/
void 
FBXAnimationParser::GetMaximumKeySpanFromSkeleton( KFbxNode* fbxNode, KFbxAnimLayer* fbxAnimLayer, int& keySpan )
{
	
	// we only want to traverse further if we actually have a skeleton connected to the node, or if the reading mode is static
	if (fbxNode->GetSkeleton() || this->mode == Static)
	{

		KFbxAnimCurve* translationCurveX = fbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_T_X);
		KFbxAnimCurve* translationCurveY = fbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_T_Y);
		KFbxAnimCurve* translationCurveZ = fbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_T_Z);

		KFbxAnimCurve* rotationCurveX = fbxNode->LclRotation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_R_X);
		KFbxAnimCurve* rotationCurveY = fbxNode->LclRotation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_R_Y);
		KFbxAnimCurve* rotationCurveZ = fbxNode->LclRotation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_R_Z);

		KFbxAnimCurve* scaleCurveX = fbxNode->LclScaling.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_S_X);
		KFbxAnimCurve* scaleCurveY = fbxNode->LclScaling.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_S_Y);
		KFbxAnimCurve* scaleCurveZ = fbxNode->LclScaling.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_S_Z);

		if (translationCurveX && translationCurveY && translationCurveZ)
		{
			int numKeys = max(max(translationCurveX->KeyGetCount(), translationCurveY->KeyGetCount()), translationCurveZ->KeyGetCount());
			keySpan = max(keySpan, numKeys);
		}
		if (rotationCurveX && rotationCurveY && rotationCurveZ)
		{
			int numKeys = max(max(rotationCurveX->KeyGetCount(), rotationCurveY->KeyGetCount()), rotationCurveZ->KeyGetCount());
			keySpan = max(keySpan, numKeys);
		}
		if (scaleCurveX && scaleCurveY && scaleCurveZ)
		{
			int numKeys = max(max(scaleCurveX->KeyGetCount(), scaleCurveY->KeyGetCount()), scaleCurveZ->KeyGetCount());
			keySpan = max(keySpan, numKeys);
		}

		int childCount = fbxNode->GetChildCount();
		for (int childIndex = 0; childIndex < childCount; childIndex++)
		{
			KFbxNode* child = fbxNode->GetChild(childIndex);
			GetMaximumKeySpanFromSkeleton(child, fbxAnimLayer, keySpan);
		}
	}
	
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXAnimationParser::ReadClip( KFbxNode* fbxNode, KFbxAnimLayer* fbxAnimLayer, Util::Array<AnimBuilderCurve>& curves, int& preInfType, int& postInfType, int keySpan )
{

	KFbxAnimCurve* translationCurveX = fbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_T_X);
	KFbxAnimCurve* translationCurveY = fbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_T_Y);
	KFbxAnimCurve* translationCurveZ = fbxNode->LclTranslation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_T_Z);

	KFbxAnimCurve* rotationCurveX = fbxNode->LclRotation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_R_X);
	KFbxAnimCurve* rotationCurveY = fbxNode->LclRotation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_R_Y);
	KFbxAnimCurve* rotationCurveZ = fbxNode->LclRotation.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_R_Z);

	KFbxAnimCurve* scaleCurveX = fbxNode->LclScaling.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_S_X);
	KFbxAnimCurve* scaleCurveY = fbxNode->LclScaling.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_S_Y);
	KFbxAnimCurve* scaleCurveZ = fbxNode->LclScaling.GetCurve<KFbxAnimCurve>(fbxAnimLayer, KFCURVENODE_S_Z);

	AnimBuilderCurve translationCurve;
	AnimBuilderCurve rotationCurve;
	AnimBuilderCurve scaleCurve;

	translationCurve.SetCurveType(CurveType::Translation);
	rotationCurve.SetCurveType(CurveType::Rotation);
	scaleCurve.SetCurveType(CurveType::Scale);

	// translation
	if (translationCurveX && translationCurveY && translationCurveZ)
	{
		preInfType |= translationCurveX->GetPreExtrapolation() | translationCurveY->GetPreExtrapolation() | translationCurveZ->GetPreExtrapolation();
		postInfType |= translationCurveX->GetPostExtrapolation() | translationCurveY->GetPostExtrapolation() | translationCurveZ->GetPostExtrapolation();
		int xKeys = translationCurveX->KeyGetCount();
		int yKeys = translationCurveY->KeyGetCount();
		int zKeys = translationCurveZ->KeyGetCount();
		translationCurve.ResizeKeyArray(keySpan);
		
		int keyIndex;

		int xIndex = 0;
		int yIndex = 0;
		int zIndex = 0;
		for (keyIndex = 0; keyIndex < keySpan; keyIndex++)
		{

			float4 key = float4(translationCurveX->EvaluateIndex(xIndex) / this->scaleFactor, translationCurveY->EvaluateIndex(yIndex) / this->scaleFactor, translationCurveZ->EvaluateIndex(zIndex) / this->scaleFactor, 0.0f);
			translationCurve.SetKey(keyIndex, key);
			if (keyIndex+1 < xKeys)
			{
				xIndex++;
			}
			if (keyIndex+1 < yKeys)
			{
				yIndex++;
			}
			if (keyIndex+1 < zKeys)
			{
				zIndex++;
			}
		}
		translationCurve.SetStatic(false);
		translationCurve.SetActive(true);
	}
	else
	{
		translationCurve.SetFirstKeyIndex(0);
		translationCurve.SetStatic(true);
		translationCurve.SetActive(true);
		float4 key = float4((float)fbxNode->LclTranslation.Get()[0] / this->scaleFactor, (float)fbxNode->LclTranslation.Get()[1] / this->scaleFactor, (float)fbxNode->LclTranslation.Get()[2] / this->scaleFactor, 0.0f);
		translationCurve.SetStaticKey(key);
	}

	// rotation
	if (rotationCurveX && rotationCurveY && rotationCurveZ)
	{
		preInfType |= rotationCurveX->GetPreExtrapolation() | rotationCurveY->GetPreExtrapolation() | rotationCurveZ->GetPreExtrapolation();
		postInfType |= rotationCurveX->GetPostExtrapolation() | rotationCurveY->GetPostExtrapolation() | rotationCurveZ->GetPostExtrapolation();
		int xKeys = rotationCurveX->KeyGetCount();
		int yKeys = rotationCurveY->KeyGetCount();
		int zKeys = rotationCurveZ->KeyGetCount();
		rotationCurve.ResizeKeyArray(keySpan);
		int keyIndex;

		int xIndex = 0;
		int yIndex = 0;
		int zIndex = 0;
		for (keyIndex = 0; keyIndex < keySpan; keyIndex++)
		{
			KFbxXMatrix matrix;
			matrix.SetR(KFbxVector4(rotationCurveX->EvaluateIndex(xIndex), rotationCurveY->EvaluateIndex(yIndex), rotationCurveZ->EvaluateIndex(zIndex), 0));
			KFbxQuaternion quat = matrix.GetQ();
			float4 key = float4((float)quat[0], (float)quat[1], (float)quat[2], (float)quat[3]);
			rotationCurve.SetKey(keyIndex, key);

			if (keyIndex+1 < xKeys)
			{
				xIndex++;
			}
			if (keyIndex+1 < yKeys)
			{
				yIndex++;
			}
			if (keyIndex+1 < zKeys)
			{
				zIndex++;
			}
		}

		rotationCurve.SetStatic(false);
		rotationCurve.SetActive(true);
	}
	else
	{
		rotationCurve.SetFirstKeyIndex(0);
		rotationCurve.SetStatic(true);
		rotationCurve.SetActive(true);
		KFbxXMatrix matrix;
		matrix.SetR(KFbxVector4(fbxNode->LclRotation.Get()[0], fbxNode->LclRotation.Get()[1], fbxNode->LclRotation.Get()[2], 0));
		KFbxQuaternion quat = matrix.GetQ();
		float4 key = float4((float)quat[0], (float)quat[1], (float)quat[2], (float)quat[3]);
		rotationCurve.SetStaticKey(key);
	}

	//scaling
	if (scaleCurveX && scaleCurveY && scaleCurveZ)
	{
		preInfType |= scaleCurveX->GetPreExtrapolation() | scaleCurveY->GetPreExtrapolation() | scaleCurveZ->GetPreExtrapolation();
		postInfType |= scaleCurveX->GetPostExtrapolation() | scaleCurveY->GetPostExtrapolation() | scaleCurveZ->GetPostExtrapolation();
		int xKeys = scaleCurveX->KeyGetCount();
		int yKeys = scaleCurveY->KeyGetCount();
		int zKeys = scaleCurveZ->KeyGetCount();
		scaleCurve.ResizeKeyArray(keySpan);
		int keyIndex;

		int xIndex = 0;
		int yIndex = 0;
		int zIndex = 0;
		for (keyIndex = 0; keyIndex < keySpan; keyIndex++)
		{
			float4 key = float4(scaleCurveX->EvaluateIndex(xIndex), scaleCurveY->EvaluateIndex(yIndex), scaleCurveZ->EvaluateIndex(zIndex), 0.0f);
			scaleCurve.SetKey(keyIndex, key);

			if (keyIndex+1 < xKeys)
			{
				xIndex++;
			}
			if (keyIndex+1 < yKeys)
			{
				yIndex++;
			}
			if (keyIndex+1 < zKeys)
			{
				zIndex++;
			}
		}
		scaleCurve.SetStatic(false);
		scaleCurve.SetActive(true);
	}
	else
	{
		scaleCurve.SetFirstKeyIndex(0);
		scaleCurve.SetStatic(true);
		scaleCurve.SetActive(true);
		float4 key = float4((float)fbxNode->LclScaling.Get()[0], (float)fbxNode->LclScaling.Get()[1], (float)fbxNode->LclScaling.Get()[2], 0.0f);
		scaleCurve.SetStaticKey(key);
	}

	curves.Append(translationCurve);
	curves.Append(rotationCurve);
	curves.Append(scaleCurve);
}

//------------------------------------------------------------------------------
/**
*/
void 
FBXAnimationParser::ConstructAnimationCurvesFromScene( KFbxScene* scene, KFbxAnimLayer* fbxAnimLayer, Util::Array<ToolkitUtil::AnimBuilderCurve>& curves, int& preInfType, int& postInfType, int keySpan)
{
	int meshes = scene->GetSrcObjectCount(FBX_TYPE(KFbxMesh));
	for (int i = 0; i < meshes; i++)
	{
		KFbxMesh* mesh = scene->GetSrcObject(FBX_TYPE(KFbxMesh), i);
		this->ReadClip(mesh->GetNode(), fbxAnimLayer, curves, preInfType, postInfType, keySpan);
	}
}

//------------------------------------------------------------------------------
/**
*/
float 
FBXAnimationParser::TimeModeToFPS( const KTime::ETimeMode& timeMode )
{
	switch (timeMode)
	{
	case KTime::eFRAMES100: return 100;
	case KTime::eFRAMES120: return 120;
	case KTime::eFRAMES1000: return 1000;
	case KTime::eFRAMES30: return 30;
	case KTime::eFRAMES30_DROP: return 30;
	case KTime::eFRAMES48: return 48;
	case KTime::eFRAMES50: return 50;
	case KTime::eFRAMES60: return 60;
	case KTime::eNTSC_DROP_FRAME: return 29.97002617f;
	case KTime::eNTSC_FULL_FRAME: return 29.97002617f;
	case KTime::ePAL: return 25;
	case KTime::eCINEMA: return 24;
	case KTime::eCINEMA_ND: return 23.976f;
	case KTime::eCUSTOM: return -1; // invalid mode, has to be handled separately
	default: return 24;

	}
}
} // namespace ToolkitUtil