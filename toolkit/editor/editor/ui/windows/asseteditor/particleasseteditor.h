#pragma once
//------------------------------------------------------------------------------
/**
    Editor for particle assets

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "asseteditor.h"

namespace Presentation
{

void ParticleEditor(AssetEditor* assetEditor, AssetEditorItem* item);
void ParticleSetup(AssetEditorItem* item);
void ParticleDiscard(AssetEditor* assetEditor, AssetEditorItem* item);
void ParticleShow(AssetEditor* assetEditor, AssetEditorItem* item, bool show);

void ParticleNew(const Ptr<IO::Stream>& stream);
} // namespace Presentation
