//------------------------------------------------------------------------------
//  modelattributes.cc
//  (C) 2012-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
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
    importFlags(ToolkitUtil::ImportFlags(ToolkitUtil::FlipUVs)),
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
ModelAttributes::AddTake(const Ptr<Take>& take)
{
    this->takes.Append(take);
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Take>&
ModelAttributes::GetTake(uint index)
{
    return this->takes[index];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Take>&
ModelAttributes::GetTake(const Util::String& name)
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
ModelAttributes::HasTake(const Ptr<Take>& take)
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
ModelAttributes::HasTake(const Util::String& name)
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
ModelAttributes::SetState(const Util::String& node, const ToolkitUtil::State& state)
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
ModelAttributes::GetState(const Util::String& node)
{
    n_assert(this->nodeStateMap.Contains(node));
    return this->nodeStateMap[node];
}

//------------------------------------------------------------------------------
/**
*/
bool
ModelAttributes::HasState(const Util::String& node)
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
        writer->BeginNode("Nebula");

        // set version
        writer->SetInt("version", ModelAttributes::Version);

        // start off with writing flags
        writer->BeginNode("Options");

        // write options
        writer->SetInt("exportFlags", this->importFlags);
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
        this->importFlags = (ToolkitUtil::ImportFlags)reader->GetInt("exportFlags");
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

} // namespace Importer