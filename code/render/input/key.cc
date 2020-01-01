//------------------------------------------------------------------------------
//  key.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "input/key.h"

namespace Input
{
Util::Dictionary<Util::String, Key::Code> Key::dict;

//------------------------------------------------------------------------------
/**
*/
Util::String
Key::ToString(Code code)
{
	switch (code)
	{
	case Back:          return "Back";
	case Tab:           return "Tab";
	case Clear:         return "Clear";
	case Return:        return "Return";
	case Shift:         return "Shift";
	case Control:       return "Control";
	case Menu:          return "Menu";
	case Pause:         return "Pause";
	case Capital:       return "Capital";
	case Escape:        return "Escape";
	case Convert:       return "Convert";
	case NonConvert:    return "NonConvert";
	case Accept:        return "Accept";
	case ModeChange:    return "ModeChange";
	case Space:         return "Space";
	case Prior:         return "Prior";
	case Next:          return "Next";
	case End:           return "End";
	case Home:          return "Home";
	case Left:          return "Left";
	case Right:         return "Right";
	case Up:            return "Up";
	case Down:          return "Down";
	case Select:        return "Select";
	case Print:         return "Print";
	case Execute:       return "Execute";
	case Snapshot:      return "Snapshot";
	case Insert:        return "Insert";
	case Delete:        return "Delete";
	case Help:          return "Help";
	case LeftWindows:   return "LeftWindows";
	case RightWindows:  return "RightWindows";
	case Apps:          return "Apps";
	case Sleep:         return "Sleep";
	case NumPad0:       return "NumPad0";
	case NumPad1:       return "NumPad1";
	case NumPad2:       return "NumPad2";
	case NumPad3:       return "NumPad3";
	case NumPad4:       return "NumPad4";
	case NumPad5:       return "NumPad5";
	case NumPad6:       return "NumPad6";
	case NumPad7:       return "NumPad7";
	case NumPad8:       return "NumPad8";
	case NumPad9:       return "NumPad9";
	case Multiply:      return "Multiply";
	case Add:           return "Add";
	case Subtract:      return "Subtract";
	case Separator:     return "Separator";
	case Decimal:       return "Decimal";
	case Divide:        return "Divide";
	case F1:            return "F1";
	case F2:            return "F2";
	case F3:            return "F3";
	case F4:            return "F4";
	case F5:            return "F5";
	case F6:            return "F6";
	case F7:            return "F7";
	case F8:            return "F8";
	case F9:            return "F9";
	case F10:           return "F10";
	case F11:           return "F11";
	case F12:           return "F12";
	case F13:           return "F13";
	case F14:           return "F14";
	case F15:           return "F15";
	case F16:           return "F16";
	case F17:           return "F17";
	case F18:           return "F18";
	case F19:           return "F19";
	case F20:           return "F20";
	case F21:           return "F21";
	case F22:           return "F22";
	case F23:           return "F23";
	case F24:           return "F24";
	case NumLock:       return "NumLock";
	case Scroll:        return "Scroll";
	case Semicolon:     return "Semicolon";
	case Slash:         return "Slash";
	case Tilde:         return "Tilde";
	case LeftBracket:   return "LeftBracket";
	case RightBracket:  return "RightBracket";
	case BackSlash:     return "BackSlash";
	case Quote:         return "Quote";
	case Comma:         return "Comma";
	case Underbar:      return "Underbar";
	case Period:        return "Period";
	case Equality:      return "Equality";
	case LeftShift:     return "LeftShift";
	case RightShift:    return "RightShift";
	case LeftControl:   return "LeftControl";
	case RightControl:  return "RightControl";
	case LeftMenu:      return "LeftMenu";
	case RightMenu:     return "RightMenu";
	case BrowserBack:   return "BrowserBack";
	case BrowserForward:    return "BrowserForward";
	case BrowserRefresh:    return "BrowserRefresh";
	case BrowserStop:       return "BrowserStop";
	case BrowserSearch:     return "BrowserSearch";
	case BrowserFavorites:  return "BrowserFavorites";
	case BrowserHome:       return "BrowserHome";
	case VolumeMute:        return "VolumeMute";
	case VolumeDown:        return "VolumeDown";
	case VolumeUp:          return "VolumeUp";
	case MediaNextTrack:    return "MediaNextTrack";
	case MediaPrevTrack:    return "MediaPrevTrack";
	case MediaStop:         return "MediaStop";
	case MediaPlayPause:    return "MediaPlayPause";
	case LaunchMail:        return "LaunchMail";
	case LaunchMediaSelect: return "LaunchMediaSelect";
	case LaunchApp1:        return "LaunchApp1";
	case LaunchApp2:        return "LaunchApp2";
	case Key0:          return "Key0";
	case Key1:          return "Key1";
	case Key2:          return "Key2";
	case Key3:          return "Key3";
	case Key4:          return "Key4";
	case Key5:          return "Key5";
	case Key6:          return "Key6";
	case Key7:          return "Key7";
	case Key8:          return "Key8";
	case Key9:          return "Key9";
	case A:             return "A";
	case B:             return "B";
	case C:             return "C";
	case D:             return "D";
	case E:             return "E";
	case F:             return "F";
	case G:             return "G";
	case H:             return "H";
	case I:             return "I";
	case J:             return "J";
	case K:             return "K";
	case L:             return "L";
	case M:             return "M";
	case N:             return "N";
	case O:             return "O";
	case P:             return "P";
	case Q:             return "Q";
	case R:             return "R";
	case S:             return "S";
	case T:             return "T";
	case U:             return "U";
	case V:             return "V";
	case W:             return "W";
	case X:             return "X";
	case Y:             return "Y";
	case Z:             return "Z";
	default:
		break;
	}
	n_error("Invalid key code!");
	return "";
}

//------------------------------------------------------------------------------
/**
*/
uint
Key::ToDirectInput(Code key)
{
	switch (key)
	{
	case Back:          return 0x0E;
	case Tab:           return 0x0F;
		//case Clear:         return "Clear";
	case Return:        return 0x1C;
		//case Shift:         return "Shift";
		//case Control:       return "Control";
		//case Menu:          return "Menu";
	case Pause:         return 0xC5;
		//case Capital:       return "Capital";
	case Escape:        return 0x01;
		//case Convert:       return "Convert";
		//case NonConvert:    return "NonConvert";
		//case Accept:        return "Accept";
		//case ModeChange:    return "ModeChange";
	case Space:         return 0x39;
		//case Prior:         return "Prior";
		//case Next:          return "Next";
	case End:           return 0xCF;
	case Home:          return 0xC7;
	case Left:          return 0xCB;
	case Right:         return 0xCD;
	case Up:            return 0xC8;
	case Down:          return 0xD0;
		//case Select:        return "Select";
		//case Print:         return "Print";
		//case Execute:       return "Execute";
		//case Snapshot:      return "Snapshot";
	case Insert:        return 0xD2;
	case Delete:        return 0xD3;
		//case Help:          return "Help";
	case LeftWindows:   return 0xDB;
	case RightWindows:  return 0xDC;
	case Apps:          return 0xDD;
	case Sleep:         return 0xDF;
	case NumPad0:       return 0x52;
	case NumPad1:       return 0x4F;
	case NumPad2:       return 0x50;
	case NumPad3:       return 0x51;
	case NumPad4:       return 0x4B;
	case NumPad5:       return 0x4C;
	case NumPad6:       return 0x4D;
	case NumPad7:       return 0x47;
	case NumPad8:       return 0x48;
	case NumPad9:       return 0x49;
	case Multiply:      return 0x37;
	case Add:           return 0x4E;
	case Subtract:      return 0x4A;
	case Separator:     return 0x0C;
	case Decimal:       return 0x53;
	case Divide:        return 0xB5;
	case F1:            return 0x3B;
	case F2:            return 0x3C;
	case F3:            return 0x3D;
	case F4:            return 0x3E;
	case F5:            return 0x3F;
	case F6:            return 0x40;
	case F7:            return 0x41;
	case F8:            return 0x42;
	case F9:            return 0x43;
	case F10:           return 0x44;
	case F11:           return 0x57;
	case F12:           return 0x58;
	case F13:           return 0x64;
	case F14:           return 0x65;
	case F15:           return 0x66;
		//case F16:           return "F16";
		//case F17:           return "F17";
		//case F18:           return "F18";
		//case F19:           return "F19";
		//case F20:           return "F20";
		//case F21:           return "F21";
		//case F22:           return "F22";
		//case F23:           return "F23";
		//case F24:           return "F24";
	case NumLock:       return 0x45;
	case Scroll:        return 0x46;
	case Semicolon:     return 0x27;
	case Slash:         return 0x35;
		//case Tilde:         return "Tilde";
	case LeftBracket:   return 0x1A;
	case RightBracket:  return 0x1B;
	case BackSlash:     return 0x2B;
		//case Quote:         return "Quote";
	case Comma:         return 0x33;
	case Underbar:      return 0x93;
	case Period:        return 0x34;
	case Equality:      return 0x0D;
	case LeftShift:     return 0x2A;
	case RightShift:    return 0x36;
	case LeftControl:   return 0x1D;
	case RightControl:  return 0x9D;
		//case LeftMenu:      return "LeftMenu";
		//case RightMenu:     return "RightMenu";
	case BrowserBack:   return 0xEA;
	case BrowserForward:    return 0xE9;
	case BrowserRefresh:    return 0xE7;
	case BrowserStop:       return 0xE8;
	case BrowserSearch:     return 0xE5;
	case BrowserFavorites:  return 0xE6;
	case BrowserHome:       return 0xB2;
	case VolumeMute:        return 0xA0;
	case VolumeDown:        return 0xAE;
	case VolumeUp:          return 0xB0;
	case MediaNextTrack:    return 0x99;
	case MediaPrevTrack:    return 0x90;
	case MediaStop:         return 0xA4;
	case MediaPlayPause:    return 0xA2;
	case LaunchMail:        return 0xEC;
	case LaunchMediaSelect: return 0xED;
		//case LaunchApp1:        return "LaunchApp1";
		//case LaunchApp2:        return "LaunchApp2";
	case Key0:          return 0x0B;
	case Key1:          return 0x02;
	case Key2:          return 0x03;
	case Key3:          return 0x04;
	case Key4:          return 0x05;
	case Key5:          return 0x06;
	case Key6:          return 0x07;
	case Key7:          return 0x08;
	case Key8:          return 0x09;
	case Key9:          return 0x0A;
	case A:             return 0x1E;
	case B:             return 0x30;
	case C:             return 0x2E;
	case D:             return 0x20;
	case E:             return 0x12;
	case F:             return 0x21;
	case G:             return 0x22;
	case H:             return 0x23;
	case I:             return 0x17;
	case J:             return 0x24;
	case K:             return 0x25;
	case L:             return 0x26;
	case M:             return 0x32;
	case N:             return 0x31;
	case O:             return 0x18;
	case P:             return 0x19;
	case Q:             return 0x10;
	case R:             return 0x13;
	case S:             return 0x1F;
	case T:             return 0x14;
	case U:             return 0x16;
	case V:             return 0x2F;
	case W:             return 0x11;
	case X:             return 0x2D;
	case Y:             return 0x15;
	case Z:             return 0x2C;
	default:
		break;
	}
	//n_error("Invalid key code!");
	return 0;
}


//------------------------------------------------------------------------------
/**
*/
uint
Key::ToRocket(Code key)
{
	switch (key)
	{

	case Space:         return 1;
	case Key0:          return 2;
	case Key1:          return 3;
	case Key2:          return 4;
	case Key3:          return 5;
	case Key4:          return 6;
	case Key5:          return 7;
	case Key6:          return 8;
	case Key7:          return 9;
	case Key8:          return 10;
	case Key9:          return 11;
	case A:             return 12;
	case B:             return 13;
	case C:             return 14;
	case D:             return 15;
	case E:             return 16;
	case F:             return 17;
	case G:             return 18;
	case H:             return 19;
	case I:             return 20;
	case J:             return 21;
	case K:             return 22;
	case L:             return 23;
	case M:             return 24;
	case N:             return 25;
	case O:             return 26;
	case P:             return 27;
	case Q:             return 28;
	case R:             return 29;
	case S:             return 30;
	case T:             return 31;
	case U:             return 32;
	case V:             return 33;
	case W:             return 34;
	case X:             return 35;
	case Y:             return 36;
	case Z:             return 37;
	case Semicolon:		return 38;
	case Add:			return 39;
	case Comma:			return 40;
	case Subtract:		return 41;
	case Period:		return 42;
	case Slash:			return 43;
	case Tilde:			return 44;
	case LeftBracket:	return 45;
	case BackSlash:		return 46;
	case RightBracket:	return 47;
	case Quote:			return 48;


	case NumPad0:       return 51;
	case NumPad1:       return 52;
	case NumPad2:       return 53;
	case NumPad3:       return 54;
	case NumPad4:       return 55;
	case NumPad5:       return 56;
	case NumPad6:       return 57;
	case NumPad7:       return 58;
	case NumPad8:       return 59;
	case NumPad9:       return 60;

	case Back:			return 69;
	case Tab:			return 70;
	case Clear:			return 71;
	case Return:		return 72;
	case Pause:			return 73;
	case Capital:		return 74;

	case Escape:		return 81;

	case Prior:			return 86;
	case Next:			return 87;
	case End:			return 88;
	case Home:			return 89;
	case Left:			return 90;
	case Up:			return 91;
	case Right:			return 92;
	case Down:			return 93;
	case Select:		return 94;
	case Print:			return 95;
	case Execute:		return 96;
	case Snapshot:		return 97;
	case Insert:		return 98;
	case Delete:		return 99;
	case Help:			return 100;
	case LeftWindows:	return 101;
	case RightWindows:	return 102;
	case Apps:			return 103;

	case Sleep:			return 104;

	case F1:			return 107;
	case F2:			return 108;
	case F3:			return 109;
	case F4:			return 110;
	case F5:			return 111;
	case F6:			return 112;
	case F7:			return 113;
	case F8:			return 114;
	case F9:			return 115;
	case F10:			return 116;
	case F11:			return 117;
	case F12:			return 118;
	case F13:			return 119;
	case F14:			return 120;
	case F15:			return 121;
	case F16:			return 122;
	case F17:			return 123;
	case F18:			return 124;
	case F19:			return 125;
	case F20:			return 126;
	case F21:			return 127;
	case F22:			return 128;
	case F23:			return 129;
	case F24:			return 130;

	case NumLock:		return 131;
	case Scroll:		return 132;

	case LeftShift:		return 138;
	case RightShift:	return 139;
	case LeftControl:	return 140;
	case RightControl:	return 141;
	case LeftMenu:		return 142;
	case RightMenu:		return 143;
	case BrowserBack:	return 144;
	case BrowserForward:return 145;
	case BrowserRefresh:return 146;
	case BrowserStop:	return 147;
	case BrowserSearch:	return 148;
	case BrowserFavorites:return 149;
	case BrowserHome:	return 150;
	case VolumeMute:	return 151;
	case VolumeDown:	return 152;
	case VolumeUp:		return 153;
	case MediaNextTrack:return 154;
	case MediaPrevTrack:return 155;
	case MediaStop:		return 156;
	case MediaPlayPause:return 157;
	case LaunchMail:	return 158;
	case LaunchMediaSelect:return 159;
	case LaunchApp1:	return 160;
	case LaunchApp2:	return 161;
	default:
		break;
	}
	//n_error("Invalid key code!");
	return 0;
}


//------------------------------------------------------------------------------
/**
*/
Key::Code Key::FromRocket(uint key)
{
	switch (key)
	{

	case 1: return Space;
	case 2: return Key0;
	case 3: return Key1;
	case 4: return Key2;
	case 5: return Key3;
	case 6: return Key4;
	case 7: return Key5;
	case 8: return Key6;
	case 9: return Key7;
	case 10: return Key8;
	case 11: return Key9;
	case 12: return A;
	case 13: return B;
	case 14: return C;
	case 15: return D;
	case 16: return E;
	case 17: return F;
	case 18: return G;
	case 19: return H;
	case 20: return I;
	case 21: return J;
	case 22: return K;
	case 23: return L;
	case 24: return M;
	case 25: return N;
	case 26: return O;
	case 27: return P;
	case 28: return Q;
	case 29: return R;
	case 30: return S;
	case 31: return T;
	case 32: return U;
	case 33: return V;
	case 34: return W;
	case 35: return X;
	case 36: return Y;
	case 37: return Z;
	case 38: return Semicolon;
	case 39: return Add;
	case 40: return Comma;
	case 41: return Subtract;
	case 42: return Period;
	case 43: return Slash;
	case 44: return Tilde;
	case 45: return LeftBracket;
	case 46: return BackSlash;
	case 47: return RightBracket;
	case 48: return Quote;
	case 51: return NumPad0;
	case 52: return NumPad1;
	case 53: return NumPad2;
	case 54: return NumPad3;
	case 55: return NumPad4;
	case 56: return NumPad5;
	case 57: return NumPad6;
	case 58: return NumPad7;
	case 59: return NumPad8;
	case 60: return NumPad9;
	case 69: return Back;
	case 70: return Tab;
	case 71: return Clear;
	case 72: return Return;
	case 73: return Pause;
	case 74: return Capital;
	case 81: return Escape;
	case 86: return Prior;
	case 87: return Next;
	case 88: return End;
	case 89: return Home;
	case 90: return Left;
	case 91: return Up;
	case 92: return Right;
	case 93: return Down;
	case 94: return Select;
	case 95: return Print;
	case 96: return Execute;
	case 97: return Snapshot;
	case 98: return Insert;
	case 99: return Delete;
	case 100: return Help;
	case 101: return LeftWindows;
	case 102: return RightWindows;
	case 103: return Apps;
	case 104: return Sleep;
	case 107: return F1;
	case 108: return F2;
	case 109: return F3;
	case 110: return F4;
	case 111: return F5;
	case 112: return F6;
	case 113: return F7;
	case 114: return F8;
	case 115: return F9;
	case 116: return F10;
	case 117: return F11;
	case 118: return F12;
	case 119: return F13;
	case 120: return F14;
	case 121: return F15;
	case 122: return F16;
	case 123: return F17;
	case 124: return F18;
	case 125: return F19;
	case 126: return F20;
	case 127: return F21;
	case 128: return F22;
	case 129: return F23;
	case 130: return F24;
	case 131: return NumLock;
	case 132: return Scroll;
	case 138: return LeftShift;
	case 139: return RightShift;
	case 140: return LeftControl;
	case 141: return RightControl;
	case 142: return LeftMenu;
	case 143: return RightMenu;
	case 144: return BrowserBack;
	case 145: return BrowserForward;
	case 146: return BrowserRefresh;
	case 147: return BrowserStop;
	case 148: return BrowserSearch;
	case 149: return BrowserFavorites;
	case 150: return BrowserHome;
	case 151: return VolumeMute;
	case 152: return VolumeDown;
	case 153: return VolumeUp;
	case 154: return MediaNextTrack;
	case 155: return MediaPrevTrack;
	case 156: return MediaStop;
	case 157: return MediaPlayPause;
	case 158: return LaunchMail;
	case 159: return LaunchMediaSelect;
	case 160: return LaunchApp1;
	case 161: return LaunchApp2;
	default:
		break;
	}
	//n_error("Invalid key code!");
	return InvalidKey;
}

//------------------------------------------------------------------------------
/**
*/
char
Key::ToChar(Code key)
{
	switch (key)
	{
	case Back:          return '\b';
	case Tab:           return '\t';
	case Return:        return '\n';
	case Space:         return ' ';
	case NumPad0:       return '0';
	case NumPad1:       return '1';
	case NumPad2:       return '2';
	case NumPad3:       return '3';
	case NumPad4:       return '4';
	case NumPad5:       return '5';
	case NumPad6:       return '6';
	case NumPad7:       return '7';
	case NumPad8:       return '8';
	case NumPad9:       return '9';
	case Multiply:      return '*';
	case Add:           return '+';
	case Subtract:      return '-';
	case Separator:     return '-';
	case Decimal:       return ',';
	case Divide:        return '/';
	case Semicolon:     return ';';
	case Slash:         return '/';
	case Tilde:         return '~';
	case LeftBracket:   return '(';
	case RightBracket:  return ')';
	case BackSlash:     return '\\';
	case Quote:         return '"';
	case Comma:         return ',';
	case Underbar:      return '_';
	case Period:        return '.';
	case Equality:      return '=';
	case Key0:          return '0';
	case Key1:          return '1';
	case Key2:          return '2';
	case Key3:          return '3';
	case Key4:          return '4';
	case Key5:          return '5';
	case Key6:          return '6';
	case Key7:          return '7';
	case Key8:          return '8';
	case Key9:          return '9';
	case A:             return 'A';
	case B:             return 'B';
	case C:             return 'C';
	case D:             return 'D';
	case E:             return 'E';
	case F:             return 'F';
	case G:             return 'G';
	case H:             return 'H';
	case I:             return 'I';
	case J:             return 'J';
	case K:             return 'K';
	case L:             return 'L';
	case M:             return 'M';
	case N:             return 'N';
	case O:             return 'O';
	case P:             return 'P';
	case Q:             return 'Q';
	case R:             return 'R';
	case S:             return 'S';
	case T:             return 'T';
	case U:             return 'U';
	case V:             return 'V';
	case W:             return 'W';
	case X:             return 'X';
	case Y:             return 'Y';
	case Z:             return 'Z';
	default:
		break;
	}
	return 0;
}
//------------------------------------------------------------------------------
/**
*/
Key::Code
Key::FromString(const Util::String& str)
{
	// setup a static dictionary object when called first
	if (dict.IsEmpty())
	{
		SetupDict();
	}
	return dict[str];
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Key::Code>
Key::KeyCodesByGroup(Key::Group group)
{
	Util::Array<Key::Code> keys;
	IndexT index;
	switch (group)
	{
	case Key::Letters:
		for (index = Key::A; index <= Key::Z; index++)
		{
			keys.Append((Key::Code)index);
		}
		break;
	case Key::Numbers:
		for (index = Key::Key0; index <= Key::Key9; index++)
		{
			keys.Append((Key::Code)index);
		}
		break;
	case Key::NumPad:
		for (index = Key::NumPad0; index <= Key::Divide; index++)
		{
			keys.Append((Key::Code)index);
		}
		break;
	case Key::FunctionKeys:
		for (index = Key::F1; index <= Key::F24; index++)
		{
			keys.Append((Key::Code)index);
		}
		break;
	case Key::CursorKeys:
		for (index = Key::Left; index <= Key::Down; index++)
		{
			keys.Append((Key::Code)index);
		}
		break;
	case Key::HomeBlock:
		keys.Append(Key::Prior);
		keys.Append(Key::Next);
		keys.Append(Key::Home);
		keys.Append(Key::End);
		keys.Append(Key::Insert);
		keys.Append(Key::Delete);
		break;

	default:
		n_error("Key::KeyCodesByGroup: Invalid Key Group.");
		break;
	}
	return keys;
}

//------------------------------------------------------------------------------
/**
*/
bool
Key::IsValid(const Util::String& str)
{
	if (dict.IsEmpty())
	{
		SetupDict();
	}

	return dict.Contains(str);
}

//------------------------------------------------------------------------------
/**
*/
void
Key::SetupDict()
{
	dict.Add("Back", Back);
	dict.Add("Tab", Tab);
	dict.Add("Clear", Clear);
	dict.Add("Return", Return);
	dict.Add("Shift", Shift);
	dict.Add("Control", Control);
	dict.Add("Ctrl", Control);
	dict.Add("Menu", Menu);
	dict.Add("Pause", Pause);
	dict.Add("Capital", Capital);
	dict.Add("Escape", Escape);
	dict.Add("Convert", Convert);
	dict.Add("NonConvert", NonConvert);
	dict.Add("Accept", Accept);
	dict.Add("ModeChange", ModeChange);
	dict.Add("Space", Space);
	dict.Add("Prior", Prior);
	dict.Add("Next", Next);
	dict.Add("End", End);
	dict.Add("Home", Home);
	dict.Add("Left", Left);
	dict.Add("Right", Right);
	dict.Add("Up", Up);
	dict.Add("Down", Down);
	dict.Add("Select", Select);
	dict.Add("Print", Print);
	dict.Add("Execute", Execute);
	dict.Add("Snapshot", Snapshot);
	dict.Add("Insert", Insert);
	dict.Add("Delete", Delete);
	dict.Add("Help", Help);
	dict.Add("LeftWindows", LeftWindows);
	dict.Add("RightWindows", RightWindows);
	dict.Add("Apps", Apps);
	dict.Add("Sleep", Sleep);
	dict.Add("NumPad0", NumPad0);
	dict.Add("NumPad1", NumPad1);
	dict.Add("NumPad2", NumPad2);
	dict.Add("NumPad3", NumPad3);
	dict.Add("NumPad4", NumPad4);
	dict.Add("NumPad5", NumPad5);
	dict.Add("NumPad6", NumPad6);
	dict.Add("NumPad7", NumPad7);
	dict.Add("NumPad8", NumPad8);
	dict.Add("NumPad9", NumPad9);
	dict.Add("Multiply", Multiply);
	dict.Add("Add", Add);
	dict.Add("Subtract", Subtract);
	dict.Add("Separator", Separator);
	dict.Add("Decimal", Decimal);
	dict.Add("Divide", Divide);
	dict.Add("F1", F1);
	dict.Add("F2", F2);
	dict.Add("F3", F3);
	dict.Add("F4", F4);
	dict.Add("F5", F5);
	dict.Add("F6", F6);
	dict.Add("F7", F7);
	dict.Add("F8", F8);
	dict.Add("F9", F9);
	dict.Add("F10", F10);
	dict.Add("F11", F11);
	dict.Add("F12", F12);
	dict.Add("F13", F13);
	dict.Add("F14", F14);
	dict.Add("F15", F15);
	dict.Add("F16", F16);
	dict.Add("F17", F17);
	dict.Add("F18", F18);
	dict.Add("F19", F19);
	dict.Add("F20", F20);
	dict.Add("F21", F21);
	dict.Add("F22", F22);
	dict.Add("F23", F23);
	dict.Add("F24", F24);
	dict.Add("NumLock", NumLock);
	dict.Add("Scroll", Scroll);
	dict.Add("Semicolon", Semicolon);
	dict.Add("Slash", Slash);
	dict.Add("Tilde", Tilde);
	dict.Add("LeftBracket", LeftBracket);
	dict.Add("RightBracket", RightBracket);
	dict.Add("BackSlash", BackSlash);
	dict.Add("Quote", Quote);
	dict.Add("Comma", Comma);
	dict.Add("Underbar", Underbar);
	dict.Add("Period", Period);
	dict.Add("Equality", Equality);
	dict.Add("LeftShift", LeftShift);
	dict.Add("RightShift", RightShift);
	dict.Add("LeftControl", LeftControl);
	dict.Add("RightControl", RightControl);
	dict.Add("LeftMenu", LeftMenu);
	dict.Add("RightMenu", RightMenu);
	dict.Add("BrowserBack", BrowserBack);
	dict.Add("BrowserForward", BrowserForward);
	dict.Add("BrowserRefresh", BrowserRefresh);
	dict.Add("BrowserStop", BrowserStop);
	dict.Add("BrowserSearch", BrowserSearch);
	dict.Add("BrowserFavorites", BrowserFavorites);
	dict.Add("BrowserHome", BrowserHome);
	dict.Add("VolumeMute", VolumeMute);
	dict.Add("VolumeDown", VolumeDown);
	dict.Add("VolumeUp", VolumeUp);
	dict.Add("MediaNextTrack", MediaNextTrack);
	dict.Add("MediaPrevTrack", MediaPrevTrack);
	dict.Add("MediaStop", MediaStop);
	dict.Add("MediaPlayPause", MediaPlayPause);
	dict.Add("LaunchMail", LaunchMail);
	dict.Add("LaunchMediaSelect", LaunchMediaSelect);
	dict.Add("LaunchApp1", LaunchApp1);
	dict.Add("LaunchApp2", LaunchApp2);
	dict.Add("Key0", Key0);
	dict.Add("Key1", Key1);
	dict.Add("Key2", Key2);
	dict.Add("Key3", Key3);
	dict.Add("Key4", Key4);
	dict.Add("Key5", Key5);
	dict.Add("Key6", Key6);
	dict.Add("Key7", Key7);
	dict.Add("Key8", Key8);
	dict.Add("Key9", Key9);
	dict.Add("A", A);
	dict.Add("B", B);
	dict.Add("C", C);
	dict.Add("D", D);
	dict.Add("E", E);
	dict.Add("F", F);
	dict.Add("G", G);
	dict.Add("H", H);
	dict.Add("I", I);
	dict.Add("J", J);
	dict.Add("K", K);
	dict.Add("L", L);
	dict.Add("M", M);
	dict.Add("N", N);
	dict.Add("O", O);
	dict.Add("P", P);
	dict.Add("Q", Q);
	dict.Add("R", R);
	dict.Add("S", S);
	dict.Add("T", T);
	dict.Add("U", U);
	dict.Add("V", V);
	dict.Add("W", W);
	dict.Add("X", X);
	dict.Add("Y", Y);
	dict.Add("Z", Z);
}

} // namespace Input