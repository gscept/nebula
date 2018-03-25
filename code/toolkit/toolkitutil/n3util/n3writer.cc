//------------------------------------------------------------------------------
//  n3writer.cc
//  (C) 2011-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "n3writer.h"
#include "util/fourcc.h"
#include "physics/model/templates.h"

using namespace Util;
using namespace Math;

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::N3Writer, 'N3WR', Core::RefCounted);
//------------------------------------------------------------------------------
/**
	Constructor
*/
N3Writer::N3Writer() :
	isOpen(false),
	isBeginModel(false),
	isBeginRoot(false),
	modelCounter(0)
{
	// empty
}

//------------------------------------------------------------------------------
/**
	Destructor
*/
N3Writer::~N3Writer()
{
	n_assert(!this->isOpen);
	n_assert(!this->isBeginModel);
}

//------------------------------------------------------------------------------
/**
	Open the writer
*/
void 
N3Writer::Open(const Ptr<IO::Stream>& stream)
{
	n_assert(!this->isOpen);
	this->modelWriter->SetPlatform(Platform::Win32);
	this->modelWriter->SetVersion(1);
	this->modelWriter->SetStream(stream);
	this->modelWriter->Open();
	this->modelCounter = 0;
	this->isOpen = true;	
	this->isBeginModel = false;
	this->isBeginCharacter = false;
}


//------------------------------------------------------------------------------
/**
*/
bool 
N3Writer::IsOpen()
{
	return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
void N3Writer::SetModelWriter( Ptr<ModelWriter> writer )
{
	n_assert(!this->isOpen);
	n_assert(!writer->IsOpen());
	this->modelWriter = writer;
}

//------------------------------------------------------------------------------
/**
	Close the writer
*/
void
N3Writer::Close()
{
	n_assert(this->isOpen);
	n_assert(!this->isBeginModel);
	this->modelWriter->Close();
	this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
	Writes the necessary data to open a model/scene
*/
void 
N3Writer::BeginModel(const Util::String& modelName)
{
	n_assert(this->isOpen);
	n_assert(!this->isBeginModel);
	
	this->modelCounter = 0;
	this->modelWriter->BeginModel("Model", FourCC('MODL'), modelName);

	this->isBeginModel = true;
}

//------------------------------------------------------------------------------
/**
	Writes the necessary data to close the model/scene
*/
void 
N3Writer::EndModel()
{
	n_assert(this->isBeginModel);

	this->modelWriter->EndModel();
	this->modelCounter = 0;
	this->isBeginModel = false;
	this->isBeginCharacter = false;
}

//------------------------------------------------------------------------------
/**
	Begins a character and writes skins, joints and animation resources
*/
void N3Writer::BeginCharacter(const Util::String& modelName,
							  const Util::Array<Skinlist>& skins, 
							  const Util::Array<Joint>& jointArray, 
							  const Util::String& animationResource, 
							  const Util::Array<JointMask>& jointMasks)
{
	n_assert(this->isOpen);
	n_assert(!this->isBeginCharacter);

	// begins a character node
	this->modelWriter->BeginModelNode("CharacterNode", FourCC('CHRN'), modelName);

	// write the amount of skin lists
	this->modelWriter->BeginTag("Number of skin lists", FourCC('NSKL'));
	this->modelWriter->WriteInt(skins.Size());
	this->modelWriter->EndTag();

	// write the actual skin lists
	for (int i = 0; i < skins.Size(); i++)
	{
		this->modelWriter->BeginTag("Skin List", FourCC('SKNL'));
		this->modelWriter->WriteString(skins[i].name);
		this->modelWriter->WriteInt(skins[i].skins.Size());
		for (int j = 0; j < skins[i].skins.Size(); j++)
		{
			this->modelWriter->WriteString(skins[i].skins[j]);
		}
		this->modelWriter->EndTag();
	}

	// write the associated animation resource
	this->modelWriter->BeginTag("Animation Resource", FourCC('ANIM'));
	this->modelWriter->WriteString(animationResource);
	this->modelWriter->EndTag();

	// write the amount of joints
	this->modelWriter->BeginTag("Number of joints", FourCC('NJNT'));
	this->modelWriter->WriteInt(jointArray.Size());
	this->modelWriter->EndTag();

	// write the actual joints
	for (int i = 0; i < jointArray.Size(); i++)
	{
		this->modelWriter->BeginTag("Joint", FourCC('JONT'));
		this->modelWriter->WriteInt(i);
		this->modelWriter->WriteInt(jointArray[i].parent);
		this->modelWriter->WriteFloat4(jointArray[i].translation);
		this->modelWriter->WriteFloat4(Math::float4(jointArray[i].rotation.x(), jointArray[i].rotation.y(), jointArray[i].rotation.z(), jointArray[i].rotation.w()));
		this->modelWriter->WriteFloat4(jointArray[i].scale);
		this->modelWriter->WriteString(jointArray[i].name);
		this->modelWriter->EndTag();
	}

	if (jointMasks.Size() > 0)
	{
		// write number of masks
		this->modelWriter->BeginTag("Number of masks", FourCC('NJMS'));
		this->modelWriter->WriteInt(jointMasks.Size());
		this->modelWriter->EndTag();

		// write joint mask
		for (int i = 0; i < jointMasks.Size(); i++)
		{
			this->modelWriter->BeginTag("Joint mask", FourCC('JOMS'));
			this->modelWriter->WriteString(jointMasks[i].name);
			this->modelWriter->WriteInt(jointMasks[i].weights.Size());
			for (int j = 0; j < jointMasks[i].weights.Size(); j++)
			{
				this->modelWriter->WriteFloat(jointMasks[i].weights[j]);
			}
			this->modelWriter->EndTag();		
		}
	}

	this->isBeginCharacter = true;
}

//------------------------------------------------------------------------------
/**
	Ends a character node
*/
void N3Writer::EndCharacter()
{
	n_assert(this->isBeginCharacter);
	this->isBeginCharacter = false;
	this->modelWriter->EndModelNode();
}

//------------------------------------------------------------------------------
/**
	Convenience function for writing transformation attributes
*/
void 
N3Writer::WriteTransform( const Transform& transform )
{
	n_assert(this->isOpen);
	n_assert(this->isBeginModel || this->isBeginCharacter);		
	if (transform.position != Math::point())
	{
		this->modelWriter->BeginTag("Transform Position", FourCC('POSI'));
		this->modelWriter->WriteFloat4(transform.position);
		this->modelWriter->EndTag();
	}		

	if (transform.rotation != Math::quaternion())
	{
		this->modelWriter->BeginTag("Transform Rotation", FourCC('ROTN'));
		this->modelWriter->WriteFloat4(Math::float4(transform.rotation.x(), transform.rotation.y(), transform.rotation.z(), transform.rotation.w()));
		this->modelWriter->EndTag();
	}		

	if (transform.scale != Math::vector())
	{
		this->modelWriter->BeginTag("Transform Scale", FourCC('SCAL'));
		this->modelWriter->WriteFloat4(transform.scale);
		this->modelWriter->EndTag();
	}		

	if (transform.rotatePivot != Math::point())
	{
		this->modelWriter->BeginTag("Transform Rotation Pivot", FourCC('RPIV'));
		this->modelWriter->WriteFloat4(transform.rotatePivot);
		this->modelWriter->EndTag();
	}		

	if (transform.scalePivot != Math::point())
	{
		this->modelWriter->BeginTag("Transform Scale Pivot", FourCC('SPIV'));
		this->modelWriter->WriteFloat4(transform.scalePivot);
		this->modelWriter->EndTag();
	}
	
}

//------------------------------------------------------------------------------
/**
	Writes core infromation into model node
*/
void N3Writer::WriteState(const Util::String& meshResource, const bbox& boundingBox, PrimitiveGroupIndex groupIndex, const State& state, const Util::String& material)
{
	n_assert(this->isOpen);
	n_assert(this->isBeginModel || this->isBeginCharacter);	
	// write mesh
	this->modelWriter->BeginTag("Mesh Resource", FourCC('MESH'));
	this->modelWriter->WriteString(meshResource);
	this->modelWriter->EndTag();

	// write primitive group index
	this->modelWriter->BeginTag("Primitive Group Index", FourCC('PGRI'));
	this->modelWriter->WriteInt(groupIndex);
	this->modelWriter->EndTag();

	// write material
    this->modelWriter->BeginTag("Material", FourCC('MATE'));
    this->modelWriter->WriteString(material);
	this->modelWriter->EndTag();

    // write bounding box
	this->modelWriter->BeginTag("Bounding Box", FourCC('LBOX'));
	this->modelWriter->WriteFloat4(boundingBox.center());
	this->modelWriter->WriteFloat4(boundingBox.extents());
	this->modelWriter->EndTag();	
}

//------------------------------------------------------------------------------
/**
	Writes a static model to a .n3 file
*/
void 
N3Writer::BeginStaticModel( const Util::String& name, const Transform& transform, PrimitiveGroupIndex groupIndex, const bbox& boundingBox, const Util::String& meshResource, const State& state, const Util::String& material )
{
	n_assert(this->isOpen);
	n_assert(this->isBeginModel);

	// then create actual model with shape node
	this->modelWriter->BeginModelNode("ShapeNode", FourCC('SPND'), name);
	
	this->WriteTransform(transform);	
	this->WriteState(meshResource, boundingBox, groupIndex, state, material);
}

//------------------------------------------------------------------------------
/**
	Writes a skinned model to a .n3 file
*/
void 
N3Writer::BeginSkinnedModel( const Util::String& name, 
									 const Transform& transform, 
									 const bbox& boundingBox, 
									 int fragmentIndex, 
									 int fragmentCount, 
									 const Util::Array<ModelConstants::SkinNode>& skinNodes, 
									 const Util::String& skinResource, 
									 const State& state, 
									 const Util::String& material)
{
	n_assert(this->isOpen);
	n_assert(this->isBeginCharacter);

	// then create actual model with shape node
	this->modelWriter->BeginModelNode("CharacterSkinNode", FourCC('CHSN'), name);

	this->WriteTransform(transform);	

	// write the amount of available skin fragments
	this->modelWriter->BeginTag("Number of skin fragments", FourCC('NSKF'));
	this->modelWriter->WriteInt(fragmentCount);
	this->modelWriter->EndTag();

	IndexT i;
	for (i = 0; i < fragmentCount; i++)
	{
		// get skin node
		const ModelConstants::SkinNode& node = skinNodes[i];

		// write the used skin fragments
		this->modelWriter->BeginTag("Used skin fragments", FourCC('SFRG'));
		this->modelWriter->WriteInt(node.primitiveGroupIndex);

		// write the used joints for the fragment
		this->modelWriter->WriteInt(node.fragmentJoints.Size());

		IndexT j;
		for (j = 0; j < node.fragmentJoints.Size(); j++)
		{
			this->modelWriter->WriteInt(node.fragmentJoints[j]);
		}
		this->modelWriter->EndTag();

		// write core information such as material, shader variables, textures and mesh. (PGRI is 0 because we have skins)
		this->WriteState(skinResource, boundingBox, node.primitiveGroupIndex, state, material);
	}
}

//------------------------------------------------------------------------------
/**
	Writes a particle model to a .n3 file
*/
void 
N3Writer::BeginParticleModel(const Util::String& name, const Transform& transform, const Util::String& meshResource, const IndexT primGroup, const State& state, const Util::String& material, const Particles::EmitterAttrs& emitterAttrs)
{
	n_assert(this->isOpen);
	n_assert(this->isBeginModel);

	Math::bbox box;
	this->modelWriter->BeginModelNode("ParticleSystemNode", FourCC('PSND'), name);
	this->WriteTransform(transform);
	this->WriteState(meshResource, box, primGroup, state, material);

	// write Emission Frequency Curve
	this->modelWriter->BeginTag("Emission Frequency", FourCC('EFRQ'));
	emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::EmissionFrequency).GetModFunc());
	this->modelWriter->EndTag();

	// write Life Time Curve
	this->modelWriter->BeginTag("Life Time", FourCC('PLFT'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::LifeTime).GetModFunc());
	this->modelWriter->EndTag();

	// write Spread Min Curve
	this->modelWriter->BeginTag("Spread Min", FourCC('PSMN'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMin).GetModFunc());
	this->modelWriter->EndTag();

	// write Spread Max Curve
	this->modelWriter->BeginTag("Spread Max", FourCC('PSMX'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::SpreadMax).GetModFunc());
	this->modelWriter->EndTag();

	// write Start Velocity Curve
	this->modelWriter->BeginTag("Start Velocity", FourCC('PSVL'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::StartVelocity).GetModFunc());
	this->modelWriter->EndTag();

	// write Rotation Velocity Curve
	this->modelWriter->BeginTag("Rotation Velocity", FourCC('PRVL'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::RotationVelocity).GetModFunc());
	this->modelWriter->EndTag();

	// write Size Curve
	this->modelWriter->BeginTag("Size", FourCC('PSZE'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Size).GetModFunc());
	this->modelWriter->EndTag();

	// write Mass Curve
	this->modelWriter->BeginTag("Mass", FourCC('PMSS'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Mass).GetModFunc());
	this->modelWriter->EndTag();

	// write Time Manipulator Curve
	this->modelWriter->BeginTag("Time Manipulator", FourCC('PTMN'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::TimeManipulator).GetModFunc());
	this->modelWriter->EndTag();

	// write Velocity Factor Curve
	this->modelWriter->BeginTag("Velocity Factor", FourCC('PVLF'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::VelocityFactor).GetModFunc());
	this->modelWriter->EndTag();

	// write Air Resistance Curve
	this->modelWriter->BeginTag("Air Resistance", FourCC('PAIR'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::AirResistance).GetModFunc());
	this->modelWriter->EndTag();

	// write Red Curve
	this->modelWriter->BeginTag("Red", FourCC('PRED'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Red).GetModFunc());
	this->modelWriter->EndTag();

	// write Green Curve
	this->modelWriter->BeginTag("Green", FourCC('PGRN'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Green).GetModFunc());
	this->modelWriter->EndTag();

	// write Blue Curve
	this->modelWriter->BeginTag("Blue", FourCC('PBLU'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Blue).GetModFunc());
	this->modelWriter->EndTag();

	// write Alpha Curve
	this->modelWriter->BeginTag("Alpha", FourCC('PALP'));
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetValues()[0]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetValues()[1]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetValues()[2]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetValues()[3]);
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetKeyPos0());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetKeyPos1());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetFrequency());
	this->modelWriter->WriteFloat(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetAmplitude());
	this->modelWriter->WriteInt(emitterAttrs.GetEnvelope(Particles::EmitterAttrs::Alpha).GetModFunc());
	this->modelWriter->EndTag();

	// write EmissionDuration
	this->modelWriter->BeginTag("EmissionDuration", FourCC('PEDU'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::EmissionDuration));
	this->modelWriter->EndTag();

	// write Looping
	this->modelWriter->BeginTag("Looping", FourCC('PLPE'));
	this->modelWriter->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::Looping));
	this->modelWriter->EndTag();

	// write Activity Distance
	this->modelWriter->BeginTag("Activity Distance", FourCC('PACD'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::ActivityDistance));
	this->modelWriter->EndTag();

	// write Render Oldest First
	this->modelWriter->BeginTag("Render Oldest First", FourCC('PROF'));
	this->modelWriter->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::RenderOldestFirst));
	this->modelWriter->EndTag();

	// write Billboard
	this->modelWriter->BeginTag("Billboard", FourCC('PBBO'));
	this->modelWriter->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::Billboard));
	this->modelWriter->EndTag();

	// write Start Rotation Min
	this->modelWriter->BeginTag("Start Rotation Min", FourCC('PRMN'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::StartRotationMin));
	this->modelWriter->EndTag();

	// write Start Rotation Max
	this->modelWriter->BeginTag("Start Rotation Max", FourCC('PRMX'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::StartRotationMax));
	this->modelWriter->EndTag();

	// write Gravity
	this->modelWriter->BeginTag("Gravity", FourCC('PGRV'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::Gravity));
	this->modelWriter->EndTag();

	// write Particle Stretch
	this->modelWriter->BeginTag("Particle Stretch", FourCC('PSTC'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::ParticleStretch));
	this->modelWriter->EndTag();

	// write Texture Tile
	this->modelWriter->BeginTag("Texture Tile", FourCC('PTTX'));
	this->modelWriter->WriteInt((int)(emitterAttrs.GetFloat(Particles::EmitterAttrs::TextureTile)));
	this->modelWriter->EndTag();

	// write Stretch To Start
	this->modelWriter->BeginTag("Stretch To Start", FourCC('PSTS'));
	this->modelWriter->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::StretchToStart));
	this->modelWriter->EndTag();

	// write Velocity Randomize
	this->modelWriter->BeginTag("Velocity Randomize", FourCC('PVRM'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::VelocityRandomize));
	this->modelWriter->EndTag();

	// write Rotation Randomize
	this->modelWriter->BeginTag("Rotation Randomize", FourCC('PRRM'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::RotationRandomize));
	this->modelWriter->EndTag();

	// write Size Randomize
	this->modelWriter->BeginTag("Size Randomize", FourCC('PSRM'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::SizeRandomize));
	this->modelWriter->EndTag();

	// write Precalc Time
	this->modelWriter->BeginTag("Precalc Time", FourCC('PPCT'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::PrecalcTime));
	this->modelWriter->EndTag();

	// write Randomize Rotation
	this->modelWriter->BeginTag("Randomize Rotation", FourCC('PRRD'));
	this->modelWriter->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::RandomizeRotation));
	this->modelWriter->EndTag();

	// write Stretch Detail
	this->modelWriter->BeginTag("Stretch Detail", FourCC('PSDL'));
	this->modelWriter->WriteInt(emitterAttrs.GetInt(Particles::EmitterAttrs::StretchDetail));
	this->modelWriter->EndTag();

	// write View Angle Fade
	this->modelWriter->BeginTag("View Angle Fade", FourCC('PVAF'));
	this->modelWriter->WriteInt(emitterAttrs.GetBool(Particles::EmitterAttrs::ViewAngleFade));
	this->modelWriter->EndTag();

	// write StartDelay
	this->modelWriter->BeginTag("StartDelay", FourCC('PDEL'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::StartDelay));
	this->modelWriter->EndTag();

	// write Phases per second
	this->modelWriter->BeginTag("Phases per second", FourCC('PDPS'));
	this->modelWriter->WriteFloat(emitterAttrs.GetFloat(Particles::EmitterAttrs::PhasesPerSecond));
	this->modelWriter->EndTag();

	// write wind direction
	this->modelWriter->BeginTag("Wind direction", FourCC('WIDR'));
	this->modelWriter->WriteFloat4(emitterAttrs.GetFloat4(Particles::EmitterAttrs::WindDirection));
	this->modelWriter->EndTag();

	// write View Angle Fade
	this->modelWriter->BeginTag("AnimPhases", FourCC('PVAP'));
	this->modelWriter->WriteInt(emitterAttrs.GetInt(Particles::EmitterAttrs::AnimPhases));
	this->modelWriter->EndTag();

}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::BeginPhysicsNode(const Util::String& name)
{
	this->modelWriter->BeginPhysicsNode(name);	
}

//------------------------------------------------------------------------------
/**
*/
void
N3Writer::BeginColliders()
{
	this->modelWriter->BeginColliderNode();
}
//------------------------------------------------------------------------------
/**
*/
void
N3Writer::EndColliders()
{
	this->modelWriter->EndColliderNode();
}
//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::WritePhysicsColliders(const Util::String& name, const Util::Array<Physics::ColliderDescription> & colliders)
{
	if (colliders.IsEmpty())
	{
		return;
	}
	this->modelWriter->BeginModelNode("ColliderGroup",FourCC('CLGR'),name);	
	for(Util::Array<Physics::ColliderDescription>::Iterator iter = colliders.Begin();iter != colliders.End();iter++)
	{
		this->modelWriter->BeginTag("Collider",FourCC('CLDR'));		

		this->modelWriter->WriteFloat4(iter->transform.getrow0());
		this->modelWriter->WriteFloat4(iter->transform.getrow1());
		this->modelWriter->WriteFloat4(iter->transform.getrow2());
		this->modelWriter->WriteFloat4(iter->transform.getrow3());
		
		this->modelWriter->WriteInt(iter->type);
		switch(iter->type)
		{
			case Physics::ColliderSphere:
				{
					this->modelWriter->WriteFloat(iter->sphere.radius);
				}
				break;
			case Physics::ColliderCube:
				{
					this->modelWriter->WriteFloat4(iter->box.halfWidth);
				}
				break;
			case Physics::ColliderCapsule:
				{
					this->modelWriter->WriteFloat(iter->capsule.radius);
					this->modelWriter->WriteFloat(iter->capsule.height);
				}
				break;
			case Physics::ColliderPlane:
				{
					this->modelWriter->WriteFloat(iter->plane.plane.a());
					this->modelWriter->WriteFloat(iter->plane.plane.b());
					this->modelWriter->WriteFloat(iter->plane.plane.c());
					this->modelWriter->WriteFloat(iter->plane.plane.d());
				}
				break;
			case Physics::ColliderMesh:
				{
					this->modelWriter->WriteInt(iter->mesh.meshType);
					this->modelWriter->WriteString(iter->mesh.meshResource);
					this->modelWriter->WriteInt(iter->mesh.primGroup);
				}
				break;
		}
		this->modelWriter->EndTag();
	}
	this->modelWriter->EndModelNode();
}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::WritePhysicsObjects(const Util::Array<Physics::PhysicsObjectDescription> & objects)
{
	for(Util::Array<Physics::PhysicsObjectDescription>::Iterator iter = objects.Begin();iter != objects.End();iter++)
	{
		this->modelWriter->BeginTag("PhysicsObject",FourCC('POBJ'));		
		this->modelWriter->WriteString(iter->type.AsString());	
		this->modelWriter->WriteString(iter->name);
		this->modelWriter->WriteInt(iter->category);
		this->modelWriter->WriteInt(iter->collideFilterMask);
		this->modelWriter->WriteInt(iter->material);
		this->modelWriter->WriteString(iter->colliderGroup);
		this->modelWriter->WriteFloat(iter->mass);
		this->modelWriter->WriteInt(iter->bodyFlags);
		this->modelWriter->EndTag();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::WriteLODDistances( float maxDistance, float minDistance )
{
	n_assert(this->isOpen);
	n_assert(this->isBeginModel);
	this->modelWriter->BeginTag("LODMinDistance", FourCC('SMID'));
	this->modelWriter->WriteFloat(minDistance);
	this->modelWriter->EndTag();

	this->modelWriter->BeginTag("LODMaxDistance", FourCC('SMAD'));
	this->modelWriter->WriteFloat(maxDistance);
	this->modelWriter->EndTag();
}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::EndPhysicsNode()
{
	this->modelWriter->EndPhysicsNode();

}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::BeginRoot( const Math::bbox& sceneBox )
{
	n_assert(!this->isBeginRoot);
	this->modelWriter->BeginModelNode("TransformNode", FourCC('TRFN'), "root");

	this->modelWriter->BeginTag("Scene Bounding Box", 'LBOX');
	this->modelWriter->WriteFloat4(sceneBox.center());
	this->modelWriter->WriteFloat4(sceneBox.extents());
	this->modelWriter->EndTag();	

	this->isBeginRoot = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::EndRoot()
{
	n_assert(this->isBeginRoot);
	this->modelWriter->EndModelNode();

	this->isBeginRoot = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::BeginSkin( const Util::String& skinName, const Math::bbox& boundingBox )
{
	n_assert(this->isBeginCharacter);
	//this->modelWriter->BeginModelNode("TransformNode", FourCC('TRFN'), skinName);
}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::EndSkin()
{
	n_assert(this->isBeginCharacter);
	//this->modelWriter->EndModelNode();
}

//------------------------------------------------------------------------------
/**
*/
void 
N3Writer::EndModelNode()
{
	// ends a model node node
	this->modelWriter->EndModelNode();
}
} // namespace Toolkit
