// ================================================================================
// ==      This file is a part of Turbo Badger. (C) 2011-2014, Emil Segerås      ==
// ==                     See tb_core.h for more information.                    ==
// ================================================================================

#include "tb_style_edit.h"
#include "tb_style_edit_content.h"
#include <assert.h>

namespace tb {

// == TBTextFragmentContentFactory ==========================================================================

int TBTextFragmentContentFactory::GetContent(const char *text)
{
	if (text[0] == '<')
	{
		int i = 0;
		while (text[i] != '>' && text[i] > 31)
			i++;
		if (text[i] == '>')
		{
			i++;
			return i;
		}
	}
	return 0;
}

TBTextFragmentContent *TBTextFragmentContentFactory::CreateFragmentContent(const char *text, int text_len)
{
	if (strncmp(text, "<hr>", text_len) == 0)
		return new TBTextFragmentContentHR(100, 2);
	else if (strncmp(text, "<u>", text_len) == 0)
		return new TBTextFragmentContentUnderline();
	else if (strncmp(text, "<color ", MIN(text_len, 7)) == 0)
	{
		TBColor color;
		color.SetFromString(text + 7, text_len - 8);
		return new TBTextFragmentContentTextColor(color);
	}
	else if (strncmp(text, "</", MIN(text_len, 2)) == 0)
		return new TBTextFragmentContentStylePop();
	return nullptr;
}

// == PHorizontalLineContent ================================================================================

TBTextFragmentContentHR::TBTextFragmentContentHR(int32 width_in_percent, int32 height)
	: width_in_percent(width_in_percent)
	, height(height)
{
}

void TBTextFragmentContentHR::Paint(const TBPaintProps *props, TBTextFragment *fragment)
{
	int x = props->translate_x + fragment->xpos;
	int y = props->translate_y + fragment->ypos;

	int w = props->block->styledit->layout_width * width_in_percent / 100;
	x += (props->block->styledit->layout_width - w) / 2;

	TBStyleEditListener *listener = props->block->styledit->listener;
	listener->DrawRectFill(TBRect(x, y, w, height), props->props->data->text_color);
}

int32 TBTextFragmentContentHR::GetWidth(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) { return Max(block->styledit->layout_width, 0); }

int32 TBTextFragmentContentHR::GetHeight(const TBBlock *block, TBFontFace *font, TBTextFragment *fragment) { return height; }

// ============================================================================

void TBTextFragmentContentUnderline::Paint(const TBPaintProps *props, TBTextFragment *fragment)
{
	if (TBTextProps::Data *data = props->props->Push())
		data->underline = true;
}

void TBTextFragmentContentTextColor::Paint(const TBPaintProps *props, TBTextFragment *fragment)
{
	if (TBTextProps::Data *data = props->props->Push())
		data->text_color = color;
}

void TBTextFragmentContentStylePop::Paint(const TBPaintProps *props, TBTextFragment *fragment)
{
	props->props->Pop();
}

} // namespace tb
