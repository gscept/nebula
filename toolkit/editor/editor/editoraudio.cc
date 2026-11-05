//------------------------------------------------------------------------------
//  editoraudio.cc
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editoraudio.h"
#include "audio/audiodevice.h"

namespace Editor
{

Audio::ClipId Fanfare;

struct EditorClip
{
    const char* name;
    Audio::ClipId id;
};

static EditorClip EditorClips[] =
{
    { "tool:syswork/data/audio/fanfare.wav", Fanfare },
};

//------------------------------------------------------------------------------
/**
*/
void
LoadEditorAudioClips()
{
    Audio::AudioDevice* device = Audio::AudioDevice::Instance();
    for (EditorClip& clip : EditorClips)
    {
        clip.id = device->LoadClip(clip.name);
        n_assert(clip.id != Audio::ClipId::Invalid());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
UnloadEditorAudioClips()
{
    Audio::AudioDevice* device = Audio::AudioDevice::Instance();
    for (EditorClip& clip : EditorClips)
    {
        if (clip.id != Audio::ClipId::Invalid())
        {
            device->UnloadClip(clip.id);
            clip.id = Audio::ClipId::Invalid();
        }
    }
}

} // namespace Editor
