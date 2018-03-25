//------------------------------------------------------------------------------
//  modelattributes.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelattributes.h"
#include "io/stream.h"
#include "io/ioserver.h"
#include "clip.h"
#include "io/xmlwriter.h"
#include "io/xmlreader.h"
#include "util/crc.h"

using namespace IO;
using namespace Util;
using namespace Particles;
namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::ModelAttributes, 'MOAT', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
ModelAttributes::ModelAttributes() :	
	exportFlags(ToolkitUtil::ExportFlags(ToolkitUtil::FlipUVs)),
	exportMode(ToolkitUtil::Static),
	scaleFactor(1.0f)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelAttributes::~ModelAttributes()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelAttributes::AddTake( const Ptr<Take>& take )
{
	this->takes.Append(take);
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Take>& 
ModelAttributes::GetTake( uint index )
{
	return this->takes[index];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Take>& 
ModelAttributes::GetTake( const Util::String& name )
{
	IndexT i;
	for (i = 0; i < this->takes.Size(); i++)
	{
		if (name == this->takes[i]->GetName())
		{
			return takes[i];
		}
	}
	n_error("Take '%s' was not found!\n", name.AsCharPtr());

	// fool compiler
	return takes[0];
}

//------------------------------------------------------------------------------
/**
*/
const Array<Ptr<Take> >& 
ModelAttributes::GetTakes() const
{
	return this->takes;
}

//------------------------------------------------------------------------------
/**
*/
const bool 
ModelAttributes::HasTake( const Ptr<Take>& take )
{
	IndexT i;
	for (i = 0; i < this->takes.Size(); i++)
	{
		if (take->GetName() == this->takes[i]->GetName())
		{
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
const bool
ModelAttributes::HasTake( const Util::String& name )
{
	IndexT i;
	for (i = 0; i < this->takes.Size(); i++)
	{
		if (name == this->takes[i]->GetName())
		{
			return true;
		}
	}
	return false;
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelAttributes::SetState( const Util::String& node, const ToolkitUtil::State& state )
{
	if (!this->nodeStateMap.Contains(node))
	{
		this->nodeStateMap.Add(node, state);
	}
	else
	{
		this->nodeStateMap[node] = state;
	}
}

//------------------------------------------------------------------------------
/**
*/
const ToolkitUtil::State& 
ModelAttributes::GetState( const Util::String& node )
{
	n_assert(this->nodeStateMap.Contains(node));
	return this->nodeStateMap[node];
}

//------------------------------------------------------------------------------
/**
*/
bool 
ModelAttributes::HasState( const Util::String& node )
{
	return this->nodeStateMap.Contains(node);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelAttributes::DeleteState(const Util::String& node)
{
	n_assert(this->nodeStateMap.Contains(node));
	this->nodeStateMap.Erase(node);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelAttributes::SetEmitterAttrs( const Util::String& node, const Particles::EmitterAttrs& attrs )
{
	if (!this->particleAttrMap.Contains(node))
	{
		this->particleAttrMap.Add(node, attrs);
	}
	else
	{
		this->particleAttrMap[node] = attrs;
	}
}

//------------------------------------------------------------------------------
/**
*/
const Particles::EmitterAttrs& 
ModelAttributes::GetEmitterAttrs( const Util::String& node )
{
	n_assert(this->particleAttrMap.Contains(node));
	return this->particleAttrMap[node];
}

//------------------------------------------------------------------------------
/**
*/
bool 
ModelAttributes::HasEmitterAttrs( const Util::String& node )
{
	return this->particleAttrMap.Contains(node);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelAttributes::DeleteEmitterAttrs(const Util::String& node)
{
	n_assert(this->particleAttrMap.Contains(node));
	this->particleAttrMap.Erase(node);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelAttributes::SetEmitterMesh( const Util::String& node, const Util::String& mesh )
{
	if (!this->particleMeshMap.Contains(node))
	{
		this->particleMeshMap.Add(node, mesh);
	}
	else
	{
		this->particleMeshMap[node] = mesh;
	}
}

//------------------------------------------------------------------------------
/**
*/
const Util::String& 
ModelAttributes::GetEmitterMesh( const Util::String& node )
{
	n_assert(this->particleMeshMap.Contains(node));
	return this->particleMeshMap[node];
}

//------------------------------------------------------------------------------
/**
*/
void
ModelAttributes::DeleteEmitterMesh(const Util::String& node)
{
	n_assert(this->particleMeshMap.Contains(node));
	this->particleMeshMap.Erase(node);
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelAttributes::Save(const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());

	// set correct access and open stream
	stream->SetAccessMode(Stream::WriteAccess);
	if (stream->Open())
	{
		// create xml writer
		Ptr<XmlWriter> writer = XmlWriter::Create();
		writer->SetStream(stream);
		writer->Open();

		// first write enclosing tag
		writer->BeginNode("Nebula3");

		// set version
		writer->SetInt("version", ModelAttributes::Version);

		// start off with writing flags
		writer->BeginNode("Options");

		// write options
		writer->SetInt("exportFlags", this->exportFlags);
		writer->SetInt("exportMode", this->exportMode);
		writer->SetFloat("scale", this->scaleFactor);

		// end options node
		writer->EndNode();

		if (this->nodeStateMap.Size() > 0)
		{
			// write states node
			writer->BeginNode("States");

			// go through all nodes and write their states
			IndexT i;
			for (i = 0; i < this->nodeStateMap.Size(); i++)
			{
				// get node name
				const String& node = this->nodeStateMap.KeyAtIndex(i);

				// get state 
				const ToolkitUtil::State& state = this->nodeStateMap.ValueAtIndex(i);

				// write model node
				writer->BeginNode("ModelNode");

				// set name
				writer->SetString("name", node);

				// set material
				writer->SetString("material", state.material);
				IndexT j;

				// begin textures node
				if (state.textures.Size() > 0)
				{
					writer->BeginNode("Textures");

					// go through all textures in state and put them in the document
					for (j = 0; j < state.textures.Size(); j++)
					{
						// start texture node
						writer->BeginNode("Texture");

						// set name
						writer->SetString("name", state.textures[j].textureName);

						// set value
						writer->SetString("value", state.textures[j].textureResource);

						// end node
						writer->EndNode();
					}

					// end textures node
					writer->EndNode();
				}

				if (state.variables.Size() > 0)
				{
					// begin variables node
					writer->BeginNode("Variables");

					// do the same for variables
					for (j = 0; j < state.variables.Size(); j++)
					{
						// get variable
						const Variant& var = state.variables[j].variableValue;
						const Variant& lower = state.variables[j].limits.Key();
						const Variant& upper = state.variables[j].limits.Value();

						// write variable node
						writer->BeginNode("Variable");

						// set name
						writer->SetString("name", state.variables[j].variableName);

						// write type and value
						writer->SetInt("type", var.GetType());

						switch (var.GetType())
						{
						case Variant::Float:
						{
							writer->SetFloat("value", var.GetFloat());
							writer->SetFloat("min", lower.GetFloat());
							writer->SetFloat("max", upper.GetFloat());
							break;
						}
						case Variant::Float2:
						{
							writer->SetFloat2("value", var.GetFloat2());
							break;
						}
						case Variant::Float4:
						{
							writer->SetFloat4("value", var.GetFloat4());
							break;
						}
						case Variant::Int:
						{
							writer->SetInt("value", var.GetInt());
							writer->SetInt("min", lower.GetInt());
							writer->SetInt("max", upper.GetInt());
							break;
						}
						case Variant::Bool:
						{
							writer->SetBool("value", var.GetBool());
							break;
						}
						}

						// end variable node
						writer->EndNode();
					}

					// end variables node
					writer->EndNode();
				}

				// end model node
				writer->EndNode();

			}

			// end states node
			writer->EndNode();
		}

		if (this->takes.Size() > 0)
		{
			// begin takes node
			writer->BeginNode("Takes");

			// add each take
			IndexT i;
			for (i = 0; i < this->takes.Size(); i++)
			{
				// get take from list
				const Ptr<Take>& take = this->takes[i];

				// write take node
				writer->BeginNode("Take");

				// set name of take
				writer->SetString("name", take->GetName());

				// go through and add each clip
				IndexT j;
				for (j = 0; j < take->GetClips().Size(); j++)
				{
					// get clip
					const Ptr<Clip>& clip = take->GetClips()[j];

					// write clip node
					writer->BeginNode("Clip");

					// set name of clip
					writer->SetString("name", clip->GetName());

					// write start value
					writer->SetInt("start", clip->GetStart());

					// write stop value
					writer->SetInt("stop", clip->GetEnd());

					// write post infinity tag
					writer->SetInt("post", clip->GetPostInfinity());

					// write pre infinity tag
					writer->SetInt("pre", clip->GetPreInfinity());

                    const Util::Array<Ptr<ClipEvent>>& events = clip->GetEvents();
                    IndexT k;
                    for (k = 0; k < events.Size(); k++)
                    {
                        // get event
                        const Ptr<ClipEvent>& event = events[k];

                        // write event node
                        writer->BeginNode("Event");

                        // set name and category of clip
                        writer->SetString("name", event->GetName());
                        writer->SetString("category", event->GetCategory());

                        // write marker type and marker
                        writer->SetInt("type", (int)event->GetMarkerType());
                        writer->SetInt("mark", event->GetMarker());

                        // end event
                        writer->EndNode();
                    }

					// end clip node
					writer->EndNode();
				}

				// end take node
				writer->EndNode();
			}

			// end takes node
			writer->EndNode();
		}

		if (this->jointMasks.Size() > 0)
		{
			writer->BeginNode("Masks");

			IndexT i;
			for (i = 0; i < this->jointMasks.Size(); i++)
			{
				// get mask from list
				const JointMask& mask = this->jointMasks[i];
				const Util::String& name = mask.name;

				writer->BeginNode("Mask");
				writer->SetString("name", name);
				writer->SetInt("weights", mask.weights.Size());
				IndexT j;
				for (j = 0; j < mask.weights.Size(); j++)
				{
					writer->BeginNode("Joint");
					writer->SetInt("index", j);
					writer->SetFloat("weight", mask.weights[j]);
					writer->EndNode();
				}
				writer->EndNode();
			}
		}

		if (this->particleAttrMap.Size() > 0)
		{
			// write particles
			writer->BeginNode("Particles");

			IndexT j;
			for (j = 0; j < this->particleAttrMap.Size(); j++)
			{
				// get node
				const String& name = this->particleAttrMap.KeyAtIndex(j);

				// get attrs
				const EmitterAttrs& attrs = this->particleAttrMap.ValueAtIndex(j);

				// get mesh
				const String& mesh = this->particleMeshMap.ValueAtIndex(j);

				// write particle
				writer->BeginNode("Particle");

				// set name
				writer->SetString("name", name);

				// set mesh
				writer->SetString("mesh", mesh);

				// get curves
				const EnvelopeCurve& emissionFrequency = attrs.GetEnvelope(EmitterAttrs::EmissionFrequency);
				const EnvelopeCurve& lifeTime = attrs.GetEnvelope(EmitterAttrs::LifeTime);
				const EnvelopeCurve& spreadMin = attrs.GetEnvelope(EmitterAttrs::SpreadMin);
				const EnvelopeCurve& spreadMax = attrs.GetEnvelope(EmitterAttrs::SpreadMax);
				const EnvelopeCurve& startVel = attrs.GetEnvelope(EmitterAttrs::StartVelocity);
				const EnvelopeCurve& rotationVel = attrs.GetEnvelope(EmitterAttrs::RotationVelocity);
				const EnvelopeCurve& size = attrs.GetEnvelope(EmitterAttrs::Size);
				const EnvelopeCurve& mass = attrs.GetEnvelope(EmitterAttrs::Mass);
				const EnvelopeCurve& timeMan = attrs.GetEnvelope(EmitterAttrs::TimeManipulator);
				const EnvelopeCurve& velFac = attrs.GetEnvelope(EmitterAttrs::VelocityFactor);
				const EnvelopeCurve& air = attrs.GetEnvelope(EmitterAttrs::AirResistance);
				const EnvelopeCurve& red = attrs.GetEnvelope(EmitterAttrs::Red);
				const EnvelopeCurve& green = attrs.GetEnvelope(EmitterAttrs::Green);
				const EnvelopeCurve& blue = attrs.GetEnvelope(EmitterAttrs::Blue);
				const EnvelopeCurve& alpha = attrs.GetEnvelope(EmitterAttrs::Alpha);

				// begin node
				writer->BeginNode("EmissionFrequency");

				// now set all attributes...
				writer->SetFloat4("Values", Math::float4(emissionFrequency.GetValues()[0],
					emissionFrequency.GetValues()[1],
					emissionFrequency.GetValues()[2],
					emissionFrequency.GetValues()[3]));
				writer->SetFloat("Pos0", emissionFrequency.GetKeyPos0());
				writer->SetFloat("Pos1", emissionFrequency.GetKeyPos1());
				writer->SetFloat("Frequency", emissionFrequency.GetFrequency());
				writer->SetFloat("Amplitude", emissionFrequency.GetAmplitude());
				writer->SetInt("Function", emissionFrequency.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(emissionFrequency.GetLimits()[0], emissionFrequency.GetLimits()[1]));

				// end node
				writer->EndNode();

				// begin node
				writer->BeginNode("LifeTime");

				writer->SetFloat4("Values", Math::float4(lifeTime.GetValues()[0],
					lifeTime.GetValues()[1],
					lifeTime.GetValues()[2],
					lifeTime.GetValues()[3]));
				writer->SetFloat("Pos0", lifeTime.GetKeyPos0());
				writer->SetFloat("Pos1", lifeTime.GetKeyPos1());
				writer->SetFloat("Frequency", lifeTime.GetFrequency());
				writer->SetFloat("Amplitude", lifeTime.GetAmplitude());
				writer->SetInt("Function", lifeTime.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(lifeTime.GetLimits()[0], lifeTime.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("SpreadMin");

				writer->SetFloat4("Values", Math::float4(spreadMin.GetValues()[0],
					spreadMin.GetValues()[1],
					spreadMin.GetValues()[2],
					spreadMin.GetValues()[3]));
				writer->SetFloat("Pos0", spreadMin.GetKeyPos0());
				writer->SetFloat("Pos1", spreadMin.GetKeyPos1());
				writer->SetFloat("Frequency", spreadMin.GetFrequency());
				writer->SetFloat("Amplitude", spreadMin.GetAmplitude());
				writer->SetInt("Function", spreadMin.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(spreadMin.GetLimits()[0], spreadMin.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("SpreadMax");

				writer->SetFloat4("Values", Math::float4(spreadMax.GetValues()[0],
					spreadMax.GetValues()[1],
					spreadMax.GetValues()[2],
					spreadMax.GetValues()[3]));
				writer->SetFloat("Pos0", spreadMax.GetKeyPos0());
				writer->SetFloat("Pos1", spreadMax.GetKeyPos1());
				writer->SetFloat("Frequency", spreadMax.GetFrequency());
				writer->SetFloat("Amplitude", spreadMax.GetAmplitude());
				writer->SetInt("Function", spreadMax.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(spreadMax.GetLimits()[0], spreadMax.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("StartVelocity");

				writer->SetFloat4("Values", Math::float4(startVel.GetValues()[0],
					startVel.GetValues()[1],
					startVel.GetValues()[2],
					startVel.GetValues()[3]));
				writer->SetFloat("Pos0", startVel.GetKeyPos0());
				writer->SetFloat("Pos1", startVel.GetKeyPos1());
				writer->SetFloat("Frequency", startVel.GetFrequency());
				writer->SetFloat("Amplitude", startVel.GetAmplitude());
				writer->SetInt("Function", startVel.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(startVel.GetLimits()[0], startVel.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("RotationVelocity");

				writer->SetFloat4("Values", Math::float4(rotationVel.GetValues()[0],
					rotationVel.GetValues()[1],
					rotationVel.GetValues()[2],
					rotationVel.GetValues()[3]));
				writer->SetFloat("Pos0", rotationVel.GetKeyPos0());
				writer->SetFloat("Pos1", rotationVel.GetKeyPos1());
				writer->SetFloat("Frequency", rotationVel.GetFrequency());
				writer->SetFloat("Amplitude", rotationVel.GetAmplitude());
				writer->SetInt("Function", rotationVel.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(rotationVel.GetLimits()[0], rotationVel.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("Size");

				writer->SetFloat4("Values", Math::float4(size.GetValues()[0],
					size.GetValues()[1],
					size.GetValues()[2],
					size.GetValues()[3]));
				writer->SetFloat("Pos0", size.GetKeyPos0());
				writer->SetFloat("Pos1", size.GetKeyPos1());
				writer->SetFloat("Frequency", size.GetFrequency());
				writer->SetFloat("Amplitude", size.GetAmplitude());
				writer->SetInt("Function", size.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(size.GetLimits()[0], size.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("Mass");

				writer->SetFloat4("Values", Math::float4(mass.GetValues()[0],
					mass.GetValues()[1],
					mass.GetValues()[2],
					mass.GetValues()[3]));
				writer->SetFloat("Pos0", mass.GetKeyPos0());
				writer->SetFloat("Pos1", mass.GetKeyPos1());
				writer->SetFloat("Frequency", mass.GetFrequency());
				writer->SetFloat("Amplitude", mass.GetAmplitude());
				writer->SetInt("Function", mass.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(mass.GetLimits()[0], mass.GetLimits()[1]));
				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("TimeManipulator");

				writer->SetFloat4("Values", Math::float4(timeMan.GetValues()[0],
					timeMan.GetValues()[1],
					timeMan.GetValues()[2],
					timeMan.GetValues()[3]));
				writer->SetFloat("Pos0", timeMan.GetKeyPos0());
				writer->SetFloat("Pos1", timeMan.GetKeyPos1());
				writer->SetFloat("Frequency", timeMan.GetFrequency());
				writer->SetFloat("Amplitude", timeMan.GetAmplitude());
				writer->SetInt("Function", timeMan.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(timeMan.GetLimits()[0], timeMan.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("VelocityFactor");

				writer->SetFloat4("Values", Math::float4(velFac.GetValues()[0],
					velFac.GetValues()[1],
					velFac.GetValues()[2],
					velFac.GetValues()[3]));
				writer->SetFloat("Pos0", velFac.GetKeyPos0());
				writer->SetFloat("Pos1", velFac.GetKeyPos1());
				writer->SetFloat("Frequency", velFac.GetFrequency());
				writer->SetFloat("Amplitude", velFac.GetAmplitude());
				writer->SetInt("Function", velFac.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(velFac.GetLimits()[0], velFac.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("AirResistance");

				writer->SetFloat4("Values", Math::float4(air.GetValues()[0],
					air.GetValues()[1],
					air.GetValues()[2],
					air.GetValues()[3]));
				writer->SetFloat("Pos0", air.GetKeyPos0());
				writer->SetFloat("Pos1", air.GetKeyPos1());
				writer->SetFloat("Frequency", air.GetFrequency());
				writer->SetFloat("Amplitude", air.GetAmplitude());
				writer->SetInt("Function", air.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(air.GetLimits()[0], air.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("Red");

				writer->SetFloat4("Values", Math::float4(red.GetValues()[0],
					red.GetValues()[1],
					red.GetValues()[2],
					red.GetValues()[3]));
				writer->SetFloat("Pos0", red.GetKeyPos0());
				writer->SetFloat("Pos1", red.GetKeyPos1());
				writer->SetFloat("Frequency", red.GetFrequency());
				writer->SetFloat("Amplitude", red.GetAmplitude());
				writer->SetInt("Function", red.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(red.GetLimits()[0], red.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("Green");

				writer->SetFloat4("Values", Math::float4(green.GetValues()[0],
					green.GetValues()[1],
					green.GetValues()[2],
					green.GetValues()[3]));
				writer->SetFloat("Pos0", green.GetKeyPos0());
				writer->SetFloat("Pos1", green.GetKeyPos1());
				writer->SetFloat("Frequency", green.GetFrequency());
				writer->SetFloat("Amplitude", green.GetAmplitude());
				writer->SetInt("Function", green.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(green.GetLimits()[0], green.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("Blue");

				writer->SetFloat4("Values", Math::float4(blue.GetValues()[0],
					blue.GetValues()[1],
					blue.GetValues()[2],
					blue.GetValues()[3]));
				writer->SetFloat("Pos0", blue.GetKeyPos0());
				writer->SetFloat("Pos1", blue.GetKeyPos1());
				writer->SetFloat("Frequency", blue.GetFrequency());
				writer->SetFloat("Amplitude", blue.GetAmplitude());
				writer->SetInt("Function", blue.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(blue.GetLimits()[0], blue.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin node
				writer->BeginNode("Alpha");

				writer->SetFloat4("Values", Math::float4(alpha.GetValues()[0],
					alpha.GetValues()[1],
					alpha.GetValues()[2],
					alpha.GetValues()[3]));
				writer->SetFloat("Pos0", alpha.GetKeyPos0());
				writer->SetFloat("Pos1", alpha.GetKeyPos1());
				writer->SetFloat("Frequency", alpha.GetFrequency());
				writer->SetFloat("Amplitude", alpha.GetAmplitude());
				writer->SetInt("Function", alpha.GetModFunc());
				writer->SetFloat2("Limits", Math::float2(alpha.GetLimits()[0], alpha.GetLimits()[1]));

				// end values node
				writer->EndNode();

				// begin values node
				writer->BeginNode("Values");

				// now write single-value attributes
				writer->SetFloat("EmissionDuration", attrs.GetFloat(EmitterAttrs::EmissionDuration));
				writer->SetBool("Looping", attrs.GetBool(EmitterAttrs::Looping));
				writer->SetFloat("ActivityDistance", attrs.GetFloat(EmitterAttrs::ActivityDistance));
				writer->SetBool("RenderOldestFirst", attrs.GetBool(EmitterAttrs::RenderOldestFirst));
				writer->SetBool("Billboard", attrs.GetBool(EmitterAttrs::Billboard));
				writer->SetFloat("StartRotationMin", attrs.GetFloat(EmitterAttrs::StartRotationMin));
				writer->SetFloat("StartRotationMax", attrs.GetFloat(EmitterAttrs::StartRotationMax));
				writer->SetFloat("Gravity", attrs.GetFloat(EmitterAttrs::Gravity));
				writer->SetFloat("ParticleStretch", attrs.GetFloat(EmitterAttrs::ParticleStretch));
				writer->SetFloat("TextureTile", attrs.GetFloat(EmitterAttrs::TextureTile));
				writer->SetBool("StretchToStart", attrs.GetBool(EmitterAttrs::StretchToStart));
				writer->SetFloat("VelocityRandomize", attrs.GetFloat(EmitterAttrs::VelocityRandomize));
				writer->SetFloat("RotationRandomize", attrs.GetFloat(EmitterAttrs::RotationRandomize));
				writer->SetFloat("SizeRandomize", attrs.GetFloat(EmitterAttrs::SizeRandomize));
				writer->SetFloat("PrecalcTime", attrs.GetFloat(EmitterAttrs::PrecalcTime));
				writer->SetBool("RandomizeRotation", attrs.GetBool(EmitterAttrs::RandomizeRotation));
				writer->SetInt("StretchDetail", attrs.GetInt(EmitterAttrs::StretchDetail));
				writer->SetBool("ViewAngleFade", attrs.GetBool(EmitterAttrs::ViewAngleFade));
				writer->SetFloat("StartDelay", attrs.GetFloat(EmitterAttrs::StartDelay));
				writer->SetFloat("PhasesPerSecond", attrs.GetFloat(EmitterAttrs::PhasesPerSecond));
				writer->SetInt("AnimPhases", attrs.GetInt(EmitterAttrs::AnimPhases));
				writer->SetFloat4("WindDirection", attrs.GetFloat4(EmitterAttrs::WindDirection));

				// end values node
				writer->EndNode();

				// end particle
				writer->EndNode();
			}

			// end particles
			writer->EndNode();
		}

		if (this->appendixNodes.Size() > 0)
		{
			// begin appendix part
			writer->BeginNode("AppendixNodes");

			IndexT j;
			for (j = 0; j < this->appendixNodes.Size(); j++)
			{
				const AppendixNode& node = this->appendixNodes.ValueAtIndex(j);
				writer->BeginNode("AppendixNode");
				writer->SetString("name", node.name);
				writer->SetString("path", node.path);
				writer->SetInt("type", node.type);

				// set transform
				writer->SetFloat4("position", node.transform.position);
				writer->SetFloat4("rotation", Math::float4(
					node.transform.rotation.x(),
					node.transform.rotation.y(),
					node.transform.rotation.z(),
					node.transform.rotation.w()));
				writer->SetFloat4("scale", node.transform.scale);

				switch (node.type)
				{
				case ModelAttributes::ParticleNode:
					writer->SetInt("primgroup", node.data.particle.primGroup);
				}

				writer->EndNode();
			}

			// end appendix part
			writer->EndNode();
		}

		// end Nebula 3 node
		writer->EndNode();

		// finish writing
		writer->Close();
		stream->Close();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelAttributes::Load(const Ptr<IO::Stream>& stream)
{
	n_assert(stream.isvalid());

	// set correct access mode and open stream
	stream->SetAccessMode(Stream::ReadAccess);
	if (stream->Open())
	{
		// create xml reader
		Ptr<XmlReader> reader = XmlReader::Create();
		reader->SetStream(stream);
		reader->Open();

		// get version
		int version = reader->GetInt("version");

		// stop loading if version is wrong
		if (version != ModelAttributes::Version)
		{
			n_warning("Invalid version in model attributes: %s\n", stream->GetURI().LocalPath().AsCharPtr());
			stream->Close();
			return;
		}

		// then make sure we have the options tag
		n_assert2(reader->SetToFirstChild("Options"), "CORRUPT .attributes FILE!: First tag must be Options!");

		// now load options
		this->exportFlags = (ToolkitUtil::ExportFlags)reader->GetInt("exportFlags");
		this->exportMode = (ToolkitUtil::ExportMode)reader->GetInt("exportMode");
		this->scaleFactor = reader->GetFloat("scale");

		// now jump back one step and start collecting nodes
		reader->SetToParent();

		// go to states node if it exists
		if (reader->SetToFirstChild("States"))
		{
			// set to first model node
			if (reader->SetToFirstChild("ModelNode")) do
			{
				// get name of node	
				String nodeName = reader->GetString("name");

				// get state
				ToolkitUtil::State state;

				// set material of state
				state.material = reader->GetString("material");

				// go to texture area
				if (reader->SetToFirstChild("Textures"))
				{
					// now iterate over each texture
					if (reader->SetToFirstChild("Texture")) do 
					{
						// create texture object
						ToolkitUtil::Texture texture;

						// set name and value of texture
						texture.textureName = reader->GetString("name");
						texture.textureResource = reader->GetString("value");

						// add it to the state
						state.textures.Append(texture);
					} 
					while (reader->SetToNextChild("Texture"));

					// go back to parent again
					reader->SetToParent();
				}

				// go to variables area
				if (reader->SetToFirstChild("Variables"))
				{
					// now iterate over each variable
					if (reader->SetToFirstChild("Variable")) do 
					{
						// create variable object
						ToolkitUtil::Variable var;

						// set name of variable
						var.variableName = reader->GetString("name");

						// get type of variable
						int type = reader->GetInt("type");

						// depending on which type we have, we need to extract the data differently
						switch (type)
						{
						case Variant::Float:
							{
								var.variableValue = reader->GetFloat("value");
                                var.limits = Util::KeyValuePair<Util::Variant, Util::Variant>(reader->GetOptFloat("min", 0.0f), reader->GetOptFloat("max", 1.0f));
								break;
							}
						case Variant::Float2:
							{
								var.variableValue = reader->GetFloat2("value");
								break;
							}
						case Variant::Float4:
							{
								var.variableValue = reader->GetFloat4("value");
								break;
							}
						case Variant::Int:
							{
								var.variableValue = reader->GetInt("value");
                                var.limits = Util::KeyValuePair<Util::Variant, Util::Variant>(reader->GetOptInt("min", 0), reader->GetOptInt("max", 1));
								break;
							}
						case Variant::Bool:
							{
								var.variableValue = reader->GetBool("value");
								break;
							}
						}

						// now add variable to state
						state.variables.Append(var);
					} 
					while (reader->SetToNextChild("Variable"));

					// go back to parent
					reader->SetToParent();
				}

				// now add model node to attributes
				this->nodeStateMap.Add(nodeName, state);
			}
			while (reader->SetToNextChild("ModelNode"));

			// go back to parent
			reader->SetToParent();
		}

		// now walk to takes tag
		if (reader->SetToFirstChild("Takes"))
		{
			// now iterate over each take
			if (reader->SetToFirstChild("Take")) do 
			{
				// create new take
				Ptr<Take> take = Take::Create();

				// set name of take
				take->SetName(reader->GetString("name"));

				// add take to list of takes
				this->takes.Append(take);

				// go through each clip
				if (reader->SetToFirstChild("Clip")) do 
				{
					// create new clip
					Ptr<Clip> clip = Clip::Create();

					// get data
                    clip->SetTake(take);
					clip->SetName(reader->GetString("name"));
					clip->SetStart(reader->GetInt("start"));
					clip->SetEnd(reader->GetInt("stop"));
					clip->SetPostInfinity((Clip::InfinityType)reader->GetInt("post"));
					clip->SetPreInfinity((Clip::InfinityType)reader->GetInt("pre"));

                    // get events
                    if (reader->SetToFirstChild("Event")) do 
                    {
                        // create new clip event
                        Ptr<ClipEvent> event = ClipEvent::Create();

                        // get data
                        event->SetName(reader->GetString("name"));
                        event->SetCategory(reader->GetString("category"));

                        // get marker type and load accordingly
                        ClipEvent::MarkerType type = (ClipEvent::MarkerType)reader->GetInt("type");
                        switch (type)
                        {
                        case ClipEvent::Ticks:
                            event->SetMarkerAsMilliseconds(reader->GetInt("mark"));
                            break;
                        case ClipEvent::Frames:
                            event->SetMarkerAsFrames(reader->GetInt("mark"));
                            break;
                        }

                        // add event
                        clip->AddEvent(event);
                    } 
                    while (reader->SetToNextChild("Event"));

					// add clip to take
					take->AddClip(clip);
				} 
				while (reader->SetToNextChild("Clip"));

			}
			while (reader->SetToNextChild("Take"));

			// go back to parent
			reader->SetToParent();
		}

		if (reader->SetToFirstChild("Masks"))
		{
			if (reader->SetToFirstChild("Mask")) do 
			{
				SizeT numWeights = reader->GetInt("weights");
				JointMask mask;
				mask.weights.Resize(numWeights);

				mask.name = reader->GetString("name");
				if (reader->SetToFirstChild("Joint")) do
				{
					int index = reader->GetInt("index");
					mask.weights[index] = reader->GetFloat("weight");
				} 
				while (reader->SetToNextChild("Joint"));

				// add to dictionary
				this->jointMasks.Append(mask);
			} 
			while (reader->SetToNextChild("Mask"));
			reader->SetToParent();
		}

		// go through particles
		if (reader->SetToFirstChild("Particles"))
		{
			// go through particle nodes
			if (reader->SetToFirstChild("Particle")) do 
			{
				// get node name
				String nodeName = reader->GetString("name");

				// get mesh name
				String mesh = reader->GetString("mesh");

				// create new node
				EmitterAttrs attrs;

				// get curves
				EnvelopeCurve emissionFrequency = attrs.GetEnvelope(EmitterAttrs::EmissionFrequency);
				EnvelopeCurve lifeTime = attrs.GetEnvelope(EmitterAttrs::LifeTime);
				EnvelopeCurve spreadMin = attrs.GetEnvelope(EmitterAttrs::SpreadMin);
				EnvelopeCurve spreadMax = attrs.GetEnvelope(EmitterAttrs::SpreadMax);
				EnvelopeCurve startVel = attrs.GetEnvelope(EmitterAttrs::StartVelocity);
				EnvelopeCurve rotationVel = attrs.GetEnvelope(EmitterAttrs::RotationVelocity);
				EnvelopeCurve size = attrs.GetEnvelope(EmitterAttrs::Size);
				EnvelopeCurve mass = attrs.GetEnvelope(EmitterAttrs::Mass);
				EnvelopeCurve timeMan = attrs.GetEnvelope(EmitterAttrs::TimeManipulator);
				EnvelopeCurve velFac = attrs.GetEnvelope(EmitterAttrs::VelocityFactor);
				EnvelopeCurve air = attrs.GetEnvelope(EmitterAttrs::AirResistance);
				EnvelopeCurve red = attrs.GetEnvelope(EmitterAttrs::Red);
				EnvelopeCurve green = attrs.GetEnvelope(EmitterAttrs::Green);
				EnvelopeCurve blue = attrs.GetEnvelope(EmitterAttrs::Blue);
				EnvelopeCurve alpha = attrs.GetEnvelope(EmitterAttrs::Alpha);

				Math::float4 values;
				Math::float2 limits;
				float keypos0, keypos1, freq, amp;
				int func;

				// go to emission frequency
				reader->SetToFirstChild("EmissionFrequency");

				// now get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				emissionFrequency.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				emissionFrequency.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::EmissionFrequency, emissionFrequency);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("LifeTime");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				lifeTime.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				lifeTime.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::LifeTime, lifeTime);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("SpreadMin");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				spreadMin.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				spreadMin.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::SpreadMin, spreadMin);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("SpreadMax");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				spreadMax.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				spreadMax.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::SpreadMax, spreadMax);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("StartVelocity");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				startVel.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				startVel.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::StartVelocity, startVel);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("RotationVelocity");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				rotationVel.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				rotationVel.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::RotationVelocity, rotationVel);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("Size");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				size.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				size.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::Size, size);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("Mass");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				mass.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				mass.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::Mass, mass);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("TimeManipulator");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				timeMan.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				timeMan.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::Mass, mass);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("VelocityFactor");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				velFac.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				velFac.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::VelocityFactor, velFac);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("AirResistance");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				air.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				air.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::AirResistance, air);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("Red");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				red.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				red.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::Red, red);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("Green");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				green.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				green.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::Green, green);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("Blue");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				blue.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				blue.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::Blue, blue);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("Alpha");

				// now Get all attributes...
				values = reader->GetFloat4("Values");
				keypos0 = reader->GetFloat("Pos0");
				keypos1 = reader->GetFloat("Pos1");
				freq = reader->GetFloat("Frequency");
				amp = reader->GetFloat("Amplitude");
				func = reader->GetInt("Function");
				limits = reader->GetFloat2("Limits");

				// setup curve
				alpha.Setup(values.x(), values.y(), values.z(), values.w(), keypos0, keypos1, freq, amp, (EnvelopeCurve::ModFunc)func);
				alpha.SetLimits(limits.x(), limits.y());
				attrs.SetEnvelope(EmitterAttrs::Alpha, alpha);

				// jump back to parent
				reader->SetToParent();

				// go to emission frequency
				reader->SetToFirstChild("Values");

				// now write single-value attributes
				float duration = reader->GetFloat("EmissionDuration");
				bool looping = reader->GetBool("Looping");
				float activityDistance = reader->GetFloat("ActivityDistance");
				bool oldestFirst = reader->GetBool("RenderOldestFirst");
				bool billboard = reader->GetBool("Billboard");
				float startRotationMin = reader->GetFloat("StartRotationMin");
				float startRotationMax = reader->GetFloat("StartRotationMax");
				float gravity = reader->GetFloat("Gravity");
				float particleStretch = reader->GetFloat("ParticleStretch");
				float textureTile = reader->GetFloat("TextureTile");
				bool stretchToStart = reader->GetBool("StretchToStart");
				float velocityRandomize = reader->GetFloat("VelocityRandomize");
				float rotationRandomize = reader->GetFloat("RotationRandomize");
				float sizeRandomize = reader->GetFloat("SizeRandomize");
				float precalcTime = reader->GetFloat("PrecalcTime");
				bool randomizeRotation = reader->GetBool("RandomizeRotation");
				int stretchDetail = reader->GetInt("StretchDetail");
				bool viewAngleFade = reader->GetBool("ViewAngleFade");
				float startDelay = reader->GetFloat("StartDelay");
				float phasesPerSecond = reader->GetFloat("PhasesPerSecond");
				int animPhases = reader->GetInt("AnimPhases");
				Math::float4 windDirection = reader->GetFloat4("WindDirection");

				// now set attributes...
				attrs.SetFloat(EmitterAttrs::EmissionDuration, duration);
				attrs.SetBool(EmitterAttrs::Looping, looping);
				attrs.SetFloat(EmitterAttrs::ActivityDistance, activityDistance);
				attrs.SetBool(EmitterAttrs::RenderOldestFirst, oldestFirst);
				attrs.SetBool(EmitterAttrs::Billboard, billboard);
				attrs.SetFloat(EmitterAttrs::StartRotationMin, startRotationMin);
				attrs.SetFloat(EmitterAttrs::StartRotationMax, startRotationMax);
				attrs.SetFloat(EmitterAttrs::Gravity, gravity);
				attrs.SetFloat(EmitterAttrs::ParticleStretch, particleStretch);
				attrs.SetFloat(EmitterAttrs::TextureTile, textureTile);
				attrs.SetBool(EmitterAttrs::StretchToStart, stretchToStart);
				attrs.SetFloat(EmitterAttrs::VelocityRandomize, velocityRandomize);
				attrs.SetFloat(EmitterAttrs::RotationRandomize, rotationRandomize);
				attrs.SetFloat(EmitterAttrs::SizeRandomize, sizeRandomize);
				attrs.SetFloat(EmitterAttrs::PrecalcTime, precalcTime);
				attrs.SetBool(EmitterAttrs::RandomizeRotation, randomizeRotation);
				attrs.SetInt(EmitterAttrs::StretchDetail, stretchDetail);
				attrs.SetBool(EmitterAttrs::ViewAngleFade, viewAngleFade);
				attrs.SetFloat(EmitterAttrs::StartDelay, startDelay);
				attrs.SetFloat(EmitterAttrs::PhasesPerSecond, phasesPerSecond);
				attrs.SetInt(EmitterAttrs::AnimPhases, animPhases);
				attrs.SetFloat4(EmitterAttrs::WindDirection, windDirection);

				// add node to nodes
				this->particleAttrMap.Add(nodeName, attrs);

				// add mesh name to nodes
				this->particleMeshMap.Add(nodeName, mesh);

				// jump back to parent
				reader->SetToParent();
			} 
			while (reader->SetToNextChild("Particle"));

			// go to parent
			reader->SetToParent();
		}

		// begin appendix nodes
		if (reader->SetToFirstChild("AppendixNodes"))
		{
			if (reader->SetToFirstChild("AppendixNode")) do
			{
				AppendixNode node;
				node.name = reader->GetString("name");
				node.path = reader->GetString("path");
				node.type = (AppendixNodeType)reader->GetInt("type");

				// get transform
				node.transform.position = reader->GetFloat4("position");
				node.transform.rotation = reader->GetFloat4("rotation");
				node.transform.scale = reader->GetFloat4("scale");

				switch (node.type)
				{
				case ModelAttributes::ParticleNode:
					node.data.particle.primGroup = reader->GetInt("primgroup");
				}
				

				this->appendixNodes.Add(node.name, node);
			}
			while (reader->SetToNextChild("AppendixNode"));

			reader->SetToParent();
		}

		// finish writing
		reader->Close();
		stream->Close();
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelAttributes::Clear()
{
	this->appendixNodes.Clear();
	this->particleAttrMap.Clear();
    this->particleMeshMap.Clear();
	this->nodeStateMap.Clear();
	this->jointMasks.Clear();

    IndexT i;
    for (i = 0; i < this->takes.Size(); i++)
    {
        this->takes[i]->Cleanup();
    }

    // cleanup takes
	this->ClearTakes();
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelAttributes::ClearTakes()
{
    // cleanup takes
    this->takes.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
ModelAttributes::AddAppendixNode(const Util::String& name, const AppendixNode& node)
{
	n_assert(!this->appendixNodes.Contains(name));
	this->appendixNodes.Add(name, node);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelAttributes::DeleteAppendixNode(const Util::String& name)
{
	n_assert(this->appendixNodes.Contains(name));
	this->appendixNodes.Erase(name);
}


} // namespace Importer