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
    BaseWindow();
    BaseWindow(Util::String name);
    virtual ~BaseWindow();
        
    const Util::String& GetName() const;
    void SetName(const char* name);

    const Util::String& GetCategory() const;
    void SetCategory(const char* category);

    //Get and set so that ImGui can access it
    bool& Open();

    //Runs and renders the interface once.
    virtual void Run();

    // Runs every frame, no matter if the window is open or not.
    virtual void Update();

    const ImGuiWindowFlags_& GetAdditionalFlags() const;

    /// Retrieve position from window
    Math::vec2 GetPosition() const;

    /// Retrieve size of window
    Math::vec2 GetSize() const;

    /// Set window position
    void SetPosition(const Math::vec2& pos);
    
    /// Set window size
    void SetSize(const Math::vec2& size);


protected:
    ImGuiWindowFlags_ additionalFlags;
    Util::String name;
    /// category that window is a part of.
    Util::String category;
    bool open;

};

} // namespace Presentation
