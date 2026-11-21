#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::ConfirmModal

    This class is the yes or no question. It is for the moment where an action is about to throw
    work away. The core keeps one modal and shows it every frame. A caller gives the question, the
    button labels and the callbacks.

    The answer runs after the frame has drawn, through the core's action queue. When the question is
    put away without an answer, the dismissed callback runs instead.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/delegate.h"
#include "util/string.h"

namespace Editor
{

class Shell;

class ConfirmModal final : public Core::RefCounted
{
    __DeclareClass(ConfirmModal)
public:
    ConfirmModal();
    virtual ~ConfirmModal();

    /// Prepare the modal. The core calls this function at Setup.
    bool Setup(Shell* core);
    /// Release what the modal holds.
    void Discard();

    /// Open the question. The labels are what the buttons say. The dismissed callback runs when the
    /// popup closes without an answer.
    void Show(const Util::String& title, const Util::String& question, const Util::String& yesLabel,
        const Util::String& noLabel, Util::Delegate<void()> onYes, Util::Delegate<void()> onNo,
        Util::Delegate<void()> onDismissed = Util::Delegate<void()>());

    /// Show the popup. The core calls this function once per frame, inside the ImGui frame.
    void Draw();

private:
    /// Queue the pressed button's callback, so it runs after the frame that drew the press.
    void Run(Util::Delegate<void()> action);
    /// Queue the dismiss, unless a button already answered. Both popup close paths call this
    /// function.
    void CancelUnlessAnswered();
    /// Run the queued dismiss.
    void RunDismissed();

    Shell* core;

    /// the popup name. ImGui uses it to open and close the popup.
    Util::String title;
    /// the question
    Util::String question;
    /// what the yes button says
    Util::String yesLabel;
    /// what the no button says
    Util::String noLabel;
    /// what runs when yes is pressed
    Util::Delegate<void()> onYes;
    /// what runs when no is pressed
    Util::Delegate<void()> onNo;
    /// what runs when the question is put away without an answer
    Util::Delegate<void()> onDismissed;

    /// true when the modal is open
    bool visible;
    /// true when the modal must open the popup on the next draw
    bool openRequested;
    /// true when the modal must close the popup on the next draw
    bool closeRequested;
    /// true after a button queued its callback, so the close does not also run the dismiss
    bool answeredFired;
};

} // namespace Editor
