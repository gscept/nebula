//------------------------------------------------------------------------------
//  @file confirmmodal.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "neweditor/widgets/confirmmodal.h"
#include "neweditor/api/shell.h"
#include "neweditor/base/palette.h"
#include "imgui.h"

namespace Editor
{

__ImplementClass(Editor::ConfirmModal, 'CNMO', Core::RefCounted);

// The question wraps at this width, so the popup stays a dialog rather than a long line
static const float QuestionWidth = 380.0f;

//------------------------------------------------------------------------------
/**
*/
ConfirmModal::ConfirmModal() :
    core(nullptr),
    visible(false),
    openRequested(false),
    closeRequested(false),
    answeredFired(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ConfirmModal::~ConfirmModal()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    The modal is built with the core, which is what draws it every frame. It is never visible until
    asked.
*/
bool
ConfirmModal::Setup(Shell* core)
{
    this->core = core;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
ConfirmModal::Discard()
{
    this->onYes = Util::Delegate<void()>();
    this->onNo = Util::Delegate<void()>();
    this->onDismissed = Util::Delegate<void()>();
    this->core = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
ConfirmModal::Show(const Util::String& title, const Util::String& question, const Util::String& yesLabel,
    const Util::String& noLabel, Util::Delegate<void()> onYes, Util::Delegate<void()> onNo,
    Util::Delegate<void()> onDismissed)
{
    this->title = title;
    this->question = question;
    this->yesLabel = yesLabel;
    this->noLabel = noLabel;
    this->onYes = onYes;
    this->onNo = onNo;
    this->onDismissed = onDismissed;

    this->visible = true;
    this->openRequested = true;
    this->closeRequested = false;
    this->answeredFired = false;
}

//------------------------------------------------------------------------------
/**
    The popup centers on the viewport and sizes to the question. The yes button carries the accent
    color and the no button is muted, so the eye lands on keeping the work.
*/
void
ConfirmModal::Draw()
{
    if (!this->visible)
        return;

    if (this->openRequested)
    {
        // The popup is named by the title, so the open and the begin have to say the same thing
        ImGui::OpenPopup(this->title.AsCharPtr());
        this->openRequested = false;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
        viewport->WorkPos.y + viewport->WorkSize.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    bool open = true;
    if (ImGui::BeginPopupModal(this->title.AsCharPtr(), &open))
    {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + QuestionWidth);
        ImGui::TextUnformatted(this->question.AsCharPtr());
        ImGui::PopTextWrapPos();

        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteAccent));
        const bool yes = ImGui::Button(this->yesLabel.AsCharPtr());
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHex(PaletteMuted));
        const bool no = ImGui::Button(this->noLabel.AsCharPtr());
        ImGui::PopStyleColor();

        if (yes)
            this->Run(this->onYes);
        else if (no)
            this->Run(this->onNo);

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
            this->closeRequested = true;

        if (this->closeRequested)
        {
            this->CancelUnlessAnswered();
            ImGui::CloseCurrentPopup();
            this->closeRequested = false;
            this->visible = false;
        }

        ImGui::EndPopup();
    }

    // ImGui itself closing the popup, Escape through the nav, lands here too. The question went
    // unanswered: the dismiss tells the asker who was waiting on an answer
    if (!open)
    {
        this->CancelUnlessAnswered();
        this->visible = false;
    }
}

//------------------------------------------------------------------------------
/**
    The callback does not run here, it is queued: what a question answers changes the model, and the
    core runs model changes after the frame that drew them.
*/
void
ConfirmModal::Run(Util::Delegate<void()> action)
{
    this->answeredFired = true;
    this->closeRequested = true;
    if (action.IsValid() && this->core != nullptr)
        this->core->QueueAction(action);
}

//------------------------------------------------------------------------------
/**
    A popup that closed without a button tells the asker who was waiting on the answer: a question
    put away is not a yes, and the asker's request goes back to waiting.
*/
void
ConfirmModal::CancelUnlessAnswered()
{
    if (this->answeredFired || !this->onDismissed.IsValid() || this->core == nullptr)
        return;
    this->core->QueueAction(Util::Delegate<void()>::FromMethod<ConfirmModal, &ConfirmModal::RunDismissed>(this));
}

//------------------------------------------------------------------------------
/**
    Both popup close paths can queue in the same frame, so the callback is taken out and cleared
    before it runs: the question is answered, or dismissed, exactly once.
*/
void
ConfirmModal::RunDismissed()
{
    Util::Delegate<void()> action = this->onDismissed;
    this->onDismissed = Util::Delegate<void()>();
    if (action.IsValid())
        action();
}

} // namespace Editor
