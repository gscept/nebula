#pragma once
//------------------------------------------------------------------------------
/**
	@class Input::Key

	Define standard key codes. Note that these are NOT localized character
	codes, but are similar to Windows virtual keys.

	(C) 2006 Radon Labs GmbH
	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "util/dictionary.h"

//------------------------------------------------------------------------------
namespace Input
{
class Key
{
public:
	/// key codes
	enum Code
	{
		Back,
		Tab,
		Clear,
		Return,
		Shift,
		Control,
		Menu,
		Pause,
		Capital,
		Escape,
		Convert,
		NonConvert,
		Accept,
		ModeChange,
		Space,
		Prior,
		Next,
		End,
		Home,
		Left,
		Right,
		Up,
		Down,
		Select,
		Print,
		Execute,
		Snapshot,
		Insert,
		Delete,
		Help,
		LeftWindows,
		RightWindows,
		Apps,
		Sleep,
		NumPad0,
		NumPad1,
		NumPad2,
		NumPad3,
		NumPad4,
		NumPad5,
		NumPad6,
		NumPad7,
		NumPad8,
		NumPad9,
		Multiply,
		Add,
		Subtract,
		Separator,
		Decimal,
		Divide,
		F1,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		F13,
		F14,
		F15,
		F16,
		F17,
		F18,
		F19,
		F20,
		F21,
		F22,
		F23,
		F24,
		NumLock,
		Scroll,
		Semicolon,
		Slash,
		Tilde,
		LeftBracket,
		RightBracket,
		BackSlash,
		Quote,
		Comma,
		Underbar,
		Period,
		Equality,
		LeftShift,
		RightShift,
		LeftControl,
		RightControl,
		LeftMenu,
		RightMenu,
		BrowserBack,
		BrowserForward,
		BrowserRefresh,
		BrowserStop,
		BrowserSearch,
		BrowserFavorites,
		BrowserHome,
		VolumeMute,
		VolumeDown,
		VolumeUp,
		MediaNextTrack,
		MediaPrevTrack,
		MediaStop,
		MediaPlayPause,
		LaunchMail,
		LaunchMediaSelect,
		LaunchApp1,
		LaunchApp2,
		Key0,
		Key1,
		Key2,
		Key3,
		Key4,
		Key5,
		Key6,
		Key7,
		Key8,
		Key9,
		A,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,

		NumKeyCodes,
		InvalidKey,
	};

	/// key group
	enum Group
	{
		Letters, // (A, B, ..., Z)
		Numbers, // (Key_0, ..., Key_9)
		NumPad, // (NumPad_0, ... NumPad9...+-*/)
		FunctionKeys, // (F1, ... , F24)
		CursorKeys,  // (->,... ,<-)
		HomeBlock // (Home, End, Prior, Next, Insert, Delete)
	};

	/// convert to string
	static Util::String ToString(Code code);
	/// convert to cegui scan codes
	static uint ToDirectInput(Code key);
	/// convert to rocket scan codes
	static uint ToRocket(Code key);
	/// convert to rocket scan codes
	static Code FromRocket(uint key);
	/// convert to Unicode character (ASCII 0-127 for now)
	static char ToChar(Code key);
	/// convert from string
	static Code FromString(const Util::String& str);
	/// check if a key string is valid
	static bool IsValid(const Util::String& str);
	/// get key codes by group
	static Util::Array<Key::Code> KeyCodesByGroup(Key::Group group);
private:
	static Util::Dictionary<Util::String, Code> dict;
	static void SetupDict();
};

};
//------------------------------------------------------------------------------
