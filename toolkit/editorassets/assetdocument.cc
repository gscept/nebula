//------------------------------------------------------------------------------
//  @file assetdocument.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "editorassets/assetdocument.h"
#include "editorassets/livebatcher.h"
#include "neweditor/api/shell.h"
#include "neweditor/model/editcommand.h"
#include "neweditor/model/editcommandmanager.h"
#include "io/fswrapper.h"
#include "io/ioserver.h"

namespace Editor
{

__ImplementClass(Editor::AssetDocument, 'ASDC', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
AssetDocument::AssetDocument() :
    shell(nullptr),
    changedOutsideCommands(false),
    ready(false),
    failed(false),
    closed(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AssetDocument::~AssetDocument()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The export is what gets loaded, so an asset without one, or with one older than its work file,
    is batched first. That waits for the batcher, but only when the export is not current.
*/
bool
AssetDocument::Open(const IO::Path& asset, Shell* shell)
{
    this->asset = asset;
    this->shell = shell;
    this->commands = EditCommandManager::Create();
    this->commands->Setup(shell);

    const IO::URI exportUri = asset.ExportURI();
    const IO::URI workUri = asset.WorkURI("assets");
    bool batch = !IO::FileExists(exportUri);
    if (!batch && IO::FileExists(workUri))
    {
        const IO::FileTime exportTime = IO::FSWrapper::GetFileWriteTime(exportUri.LocalPath());
        const IO::FileTime workTime = IO::FSWrapper::GetFileWriteTime(workUri.LocalPath());
        batch = exportTime < workTime;
    }
    if (batch)
    {
        LiveBatcher::BatchFile(asset);
        LiveBatcher::Wait();
    }

    return this->OnOpen();
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::Close()
{
    if (this->closed)
        return;
    this->closed = true;
    this->OnClose();
    if (this->commands.isvalid())
    {
        this->commands->Discard();
        this->commands = nullptr;
    }
    this->savedTop = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
const IO::Path&
AssetDocument::GetAsset() const
{
    return this->asset;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<EditCommandManager>
AssetDocument::GetCommandManager() const
{
    return this->commands;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::Draw()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::OnShown(bool shown)
{
    (void)shown;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDocument::OnEditAction(EditAction action)
{
    (void)action;
    return false;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDocument::CanSave() const
{
    return false;
}

//------------------------------------------------------------------------------
/**
    An unchanged asset is not written. The document writes the whole asset as it was loaded, so
    saving it unchanged would undo what changed the work file since, a reimport for instance.
*/
bool
AssetDocument::Save()
{
    if (!this->CanSave() || !this->IsDirty())
        return true;

    Util::String error;
    if (!this->OnSave(error))
    {
        if (this->shell != nullptr)
            this->shell->Log(LogMessageType::N_WARNING, Util::Format("Could not save '%s': %s", this->asset.GetFolderAndFile().AsCharPtr(), error.AsCharPtr()));
        return false;
    }

    this->savedTop = this->TopCommand();
    this->changedOutsideCommands = false;
    LiveBatcher::BatchFile(this->asset);
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::Revert()
{
    // the commands undo themselves back to the last save, which is what the work file has. A
    // change made outside them is the type's to take back
    if (this->commands.isvalid())
    {
        while (this->commands->CanUndo() && this->TopCommand() != this->savedTop)
            this->commands->Undo();
        this->commands->Clear();
    }
    this->OnRevert();
    this->savedTop = nullptr;
    this->changedOutsideCommands = false;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDocument::IsDirty() const
{
    return this->changedOutsideCommands || this->TopCommand() != this->savedTop;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDocument::IsReady() const
{
    return this->ready;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDocument::HasFailed() const
{
    return this->failed;
}

//------------------------------------------------------------------------------
/**
*/
const Util::String&
AssetDocument::GetFailReason() const
{
    return this->failReason;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDocument::OnOpen()
{
    this->SetReady();
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::OnClose()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDocument::OnSave(Util::String& outError)
{
    outError = "this asset type can't be saved";
    return false;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::OnRevert()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::SetReady()
{
    this->ready = true;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::SetFailed(const Util::String& reason)
{
    this->failed = true;
    this->failReason = reason;
}

//------------------------------------------------------------------------------
/**
*/
void
AssetDocument::MarkChanged()
{
    this->changedOutsideCommands = true;
}

//------------------------------------------------------------------------------
/**
*/
bool
AssetDocument::IsClosed() const
{
    return this->closed;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<EditCommand>
AssetDocument::TopCommand() const
{
    if (!this->commands.isvalid())
        return nullptr;
    const EditCommandManager::CommandList& undo = this->commands->GetUndoList();
    if (undo.IsEmpty() || undo.Back().commands.IsEmpty())
        return nullptr;
    return undo.Back().commands.Back();
}

} // namespace Editor
