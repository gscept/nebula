#pragma once
//------------------------------------------------------------------------------
/**
    Presentation::BaseWindow

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "util/string.h"
#include "core/refcounted.h"
#include "imgui.h"

//------------------------------------------------------------------------------
namespace Presentation
{
class BaseWindow : public Core::RefCounted
{
    __DeclareClass(BaseWindow)
public:

    enum class SaveMode
    {
        None,
        SaveActive,
        SaveAll
    };

    BaseWindow();
    BaseWindow(Util::String name);
    virtual ~BaseWindow();
        
    const Util::String GetName() const;
    void SetName(const char* name);

    const Util::String& GetCategory() const;
    void SetCategory(const char* category);

    //Get and set so that ImGui can access it
    bool& Open();

    //Runs and renders the interface once.
    virtual void Run(SaveMode save);

    // Runs every frame, no matter if the window is open or not.
    virtual void Update();

    const ImGuiWindowFlags_& GetAdditionalFlags() const;

    /// Retrieve window padding from window
    Math::vec2 GetWindowPadding() const;

    /// Set window padding
    void SetWindowPadding(const Math::vec2& padding);

    /// Retrieve position from window
    Math::vec2 GetPosition() const;

    /// Retrieve size of window
    Math::vec2 GetSize() const;

    /// Set window position
    void SetPosition(const Math::vec2& pos);
    
    /// Set window size
    void SetSize(const Math::vec2& size);

    /// Increment the edit counter to mark the window title with *
    void Edit();
    /// Decrement edit counter
    void Unedit(int count = 1);
    /// Saves to reset the edit counter
    void Save();

    /// Format title based on edit counter, adds an asterisk if the edit counter is non-zero to indicate an edited state
    static Util::String FormatName(const Util::String& name, int editCounter);

protected:
    friend class WindowServer;

    ImGuiWindowFlags_ additionalFlags;
    Util::String name;
    /// category that window is a part of.
    Util::String category;
    bool open;
    int editCounter;

private:
    Math::vec2 windowPadding;
    bool usesCustomWindowPadding = false;
};

} // namespace Presentation
