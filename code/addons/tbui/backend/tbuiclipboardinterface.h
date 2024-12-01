//------------------------------------------------------------------------------
//  backend/tbuiclipboard.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "platform/tb_clipboard_interface.h"

namespace TBUI
{

class TBUIClipboardInterface : public tb::TBClipboardInterface
{
public:
    void Empty() override;
    bool HasText() override;
    bool SetText(const char* text) override;
    bool GetText(tb::TBStr& text) override;

private:
    tb::TBStr clipboard;
};

} // namespace tb
