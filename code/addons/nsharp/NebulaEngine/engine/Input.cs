using System;
using System.Text;
using System.Runtime.InteropServices;

namespace Nebula
{
    namespace Game
    {
        /// <summary>
        /// The key codes the engine's Input::Key::Code has. The values are the contract between the two sides,
        /// so they are written out here rather than left to the compiler, and the test checks them against the
        /// engine.
        /// </summary>
        public enum Key
        {
            Back = 0,
            Tab = 1,
            Clear = 2,
            Return = 3,
            Shift = 4,
            Control = 5,
            Menu = 6,
            Pause = 7,
            Capital = 8,
            Escape = 9,
            Convert = 10,
            NonConvert = 11,
            Accept = 12,
            ModeChange = 13,
            Space = 14,
            PageUp = 15,
            PageDown = 16,
            End = 17,
            Home = 18,
            Left = 19,
            Right = 20,
            Up = 21,
            Down = 22,
            Select = 23,
            Print = 24,
            Execute = 25,
            Snapshot = 26,
            Insert = 27,
            Delete = 28,
            Help = 29,
            LeftWindows = 30,
            RightWindows = 31,
            Apps = 32,
            Sleep = 33,
            NumPad0 = 34,
            NumPad1 = 35,
            NumPad2 = 36,
            NumPad3 = 37,
            NumPad4 = 38,
            NumPad5 = 39,
            NumPad6 = 40,
            NumPad7 = 41,
            NumPad8 = 42,
            NumPad9 = 43,
            Multiply = 44,
            Add = 45,
            Subtract = 46,
            Separator = 47,
            Decimal = 48,
            Divide = 49,
            F1 = 50,
            F2 = 51,
            F3 = 52,
            F4 = 53,
            F5 = 54,
            F6 = 55,
            F7 = 56,
            F8 = 57,
            F9 = 58,
            F10 = 59,
            F11 = 60,
            F12 = 61,
            NumLock = 62,
            ScrollLock = 63,
            Semicolon = 64,
            Slash = 65,
            Tilde = 66,
            LeftBracket = 67,
            RightBracket = 68,
            BackSlash = 69,
            Quote = 70,
            Comma = 71,
            Underbar = 72,
            Period = 73,
            Minus = 74,
            Equality = 75,
            LeftShift = 76,
            RightShift = 77,
            LeftControl = 78,
            RightControl = 79,
            LeftMenu = 80,
            RightMenu = 81,
            BrowserBack = 82,
            BrowserForward = 83,
            BrowserRefresh = 84,
            BrowserStop = 85,
            BrowserSearch = 86,
            BrowserFavorites = 87,
            BrowserHome = 88,
            VolumeMute = 89,
            VolumeDown = 90,
            VolumeUp = 91,
            MediaNextTrack = 92,
            MediaPrevTrack = 93,
            MediaStop = 94,
            MediaPlayPause = 95,
            LaunchMail = 96,
            LaunchMediaSelect = 97,
            LaunchApp1 = 98,
            LaunchApp2 = 99,
            Key0 = 100,
            Key1 = 101,
            Key2 = 102,
            Key3 = 103,
            Key4 = 104,
            Key5 = 105,
            Key6 = 106,
            Key7 = 107,
            Key8 = 108,
            Key9 = 109,
            A = 110,
            B = 111,
            C = 112,
            D = 113,
            E = 114,
            F = 115,
            G = 116,
            H = 117,
            I = 118,
            J = 119,
            K = 120,
            L = 121,
            M = 122,
            N = 123,
            O = 124,
            P = 125,
            Q = 126,
            R = 127,
            S = 128,
            T = 129,
            U = 130,
            V = 131,
            W = 132,
            X = 133,
            Y = 134,
            Z = 135,

            NumKeyCodes = 136,
            InvalidKey = 137,
        }

        /// <summary>
        /// The mouse buttons the engine's Input::MouseButton::Code has, with the same values.
        /// </summary>
        public enum MouseButton
        {
            LeftButton = 0,
            RightButton = 1,
            MiddleButton = 2,

            NumMouseButtons = 3,
            InvalidMouseButton = 4,
        }

        /// <summary>
        /// The modifier bits the engine's Input::Modifier::Code has. The values are the contract between
        /// the two sides, same as the key codes. A group is the pair of sides, and a side is one bit.
        /// </summary>
        public enum Modifier : uint
        {
            NoModifier = 0,

            LeftControlModifier = 1 << 0,
            RightControlModifier = 1 << 1,
            LeftShiftModifier = 1 << 2,
            RightShiftModifier = 1 << 3,
            LeftAltModifier = 1 << 4,
            RightAltModifier = 1 << 5,
            LeftSuperModifier = 1 << 6,
            RightSuperModifier = 1 << 7,

            ControlModifier = LeftControlModifier | RightControlModifier,
            ShiftModifier = LeftShiftModifier | RightShiftModifier,
            AltModifier = LeftAltModifier | RightAltModifier,
            SuperModifier = LeftSuperModifier | RightSuperModifier,
        }

        /// <summary>
        /// The mirror of the engine's default keyboard and mouse handlers. The engine has authority over the
        /// state, and this holds what the last sync saw, so a read is a field read rather than a call through
        /// the api.
        ///
        /// The mirror is only as good as the frame it was read in. The engine clears the per-frame flags at the
        /// start of the input frame and sets them again from the events that arrive during the pump, so the sync
        /// has to happen after the pump and before the end of the frame. NebulaApp.OnBeginFrame is that point:
        /// the render feature unit pumps the window events at the start of its OnBeginFrame, and the script
        /// feature unit is attached after it.
        ///
        /// What is here is what the input server received. ImGui gets the keys first and the window drops the
        /// events it consumes, so a key typed into a text field never reaches the handlers. HasFocus and the two
        /// capture flags are what tell a caller that a key is missing because the UI took it rather than because
        /// nobody pressed it.
        /// </summary>
        public static class Input
        {
            private const int MaxCharInput = 256;

            /// <summary>
            /// One key as three words: held through the frame, pressed during it, released during it. Three of
            /// these cover the 135 key codes, and a code decides its word and its bit.
            /// </summary>
            [StructLayout(LayoutKind.Sequential)]
            private struct KeyWords
            {
                internal ulong held;
                internal ulong down;
                internal ulong up;
            }

            /// <summary>
            /// The layout the engine writes with. The eight byte fields come first and then the four byte ones,
            /// which is the order that puts the entry at 136 bytes on both sides: 132 of fields and 4 bytes of
            /// padding at the end, because the eight byte fields put the entry on an eight byte boundary.
            /// </summary>
            [StructLayout(LayoutKind.Sequential)]
            private struct Snapshot
            {
                internal KeyWords keys0;
                internal KeyWords keys1;
                internal KeyWords keys2;

                internal uint mouseHeld;
                internal uint mouseDown;
                internal uint mouseUp;
                internal uint mouseDoubleClicked;
                internal int wheel;

                internal uint hasFocus;
                internal uint mouseCaptured;
                internal uint keyCaptured;
                internal uint mods;

                internal float pixelX;
                internal float pixelY;
                internal float screenX;
                internal float screenY;
                internal float moveX;
                internal float moveY;
            }

            private static readonly Snapshot[] buffer = new Snapshot[1];
            private static readonly GCHandle handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
            private static readonly byte[] charBuffer = new byte[MaxCharInput];
            private static readonly GCHandle charHandle = GCHandle.Alloc(charBuffer, GCHandleType.Pinned);

            [DllImport("__Internal", EntryPoint = "InputSync")]
            private static extern int SyncCall(IntPtr buffer, int bufferSize);

            [DllImport("__Internal", EntryPoint = "InputCharInput")]
            private static extern int CharInputCall(IntPtr outBuffer, int outLen);

            /// <summary>
            /// The size of the layout on this side. The engine asserts the same number, and the test compares
            /// the two, because the field order is the only thing the contract has.
            /// </summary>
            public static int SnapshotSize => Marshal.SizeOf<Snapshot>();

            /// <summary>
            /// Refresh the mirror from the engine. Called once per frame at the beginning of the frame, after
            /// the events have been pumped. A run with no window has no input server, and the mirror is cleared
            /// for it rather than left with the state the previous frame had.
            /// </summary>
            public static void Sync()
            {
                int written = SyncCall(handle.AddrOfPinnedObject(), Marshal.SizeOf<Snapshot>());
                if (written == 0)
                    Array.Clear(buffer, 0, buffer.Length);
            }

            /// <summary>
            /// The word a key code falls in. A code outside the codes the engine has is not in any word, so the
            /// answer is no bit rather than a read outside the buffer.
            /// </summary>
            private static KeyWords Words(Key key)
            {
                switch ((int)key / 64)
                {
                    case 0: return buffer[0].keys0;
                    case 1: return buffer[0].keys1;
                    default: return buffer[0].keys2;
                }
            }

            private static ulong Bit(Key key) => 1UL << ((int)key % 64);

            /// <summary>
            /// Held at the time of the sync. A key that was pressed and released inside one frame is held for
            /// that frame, because the engine keeps the pressed state until the frame after the release.
            /// </summary>
            public static bool IsKeyHeld(Key key)
            {
                if ((int)key >= (int)Key.NumKeyCodes)
                    return false;

                return (Words(key).held & Bit(key)) != 0;
            }

            /// <summary>
            /// Went down during this frame. This is the edge, so it is true for one frame only.
            /// </summary>
            public static bool IsKeyPressed(Key key)
            {
                if ((int)key >= (int)Key.NumKeyCodes)
                    return false;

                return (Words(key).down & Bit(key)) != 0;
            }

            /// <summary>
            /// Went up during this frame. Also an edge, and also true for one frame only.
            /// </summary>
            public static bool IsKeyReleased(Key key)
            {
                if ((int)key >= (int)Key.NumKeyCodes)
                    return false;

                return (Words(key).up & Bit(key)) != 0;
            }

            /// <summary>
            /// Held at the time of the sync.
            /// </summary>
            public static bool IsMouseHeld(MouseButton button)
            {
                if ((int)button >= (int)MouseButton.NumMouseButtons)
                    return false;

                return (buffer[0].mouseHeld & (1u << (int)button)) != 0;
            }

            public static bool IsMousePressed(MouseButton button)
            {
                if ((int)button >= (int)MouseButton.NumMouseButtons)
                    return false;

                return (buffer[0].mouseDown & (1u << (int)button)) != 0;
            }

            public static bool IsMouseReleased(MouseButton button)
            {
                if ((int)button >= (int)MouseButton.NumMouseButtons)
                    return false;

                return (buffer[0].mouseUp & (1u << (int)button)) != 0;
            }

            public static bool IsMouseDoubleClicked(MouseButton button)
            {
                if ((int)button >= (int)MouseButton.NumMouseButtons)
                    return false;

                return (buffer[0].mouseDoubleClicked & (1u << (int)button)) != 0;
            }

            /// <summary>
            /// The wheel as the net movement for the frame: 1 forward, -1 backward, 0 when it did not move.
            /// </summary>
            public static int Wheel => buffer[0].wheel;

            public static Vector2 PixelPosition => new Vector2(buffer[0].pixelX, buffer[0].pixelY);

            /// <summary>
            /// The same position as 0.0 through 1.0.
            /// </summary>
            public static Vector2 ScreenPosition => new Vector2(buffer[0].screenX, buffer[0].screenY);

            /// <summary>
            /// The movement since the frame started. The engine builds it as the position the frame started at
            /// minus the position the event carried, which is the negative of the delta, and the raw move path
            /// assigns the delta itself. See ISSUES.md.
            /// </summary>
            public static Vector2 Movement => new Vector2(buffer[0].moveX, buffer[0].moveY);

            public static bool HasFocus => buffer[0].hasFocus != 0;

            /// <summary>
            /// Whether a handler holds the mouse. The handler that holds the capture is the one the events go
            /// to, so this is what tells a caller that the mouse is somewhere else.
            /// </summary>
            public static bool MouseCaptured => buffer[0].mouseCaptured != 0;

            public static bool KeyCaptured => buffer[0].keyCaptured != 0;

            /// <summary>
            /// The modifiers held at the time of the sync. The engine builds this from its own key states,
            /// which is what answers for a modifier key on its own.
            /// </summary>
            public static Modifier Modifiers => (Modifier)buffer[0].mods;

            /// <summary>
            /// Held at the time of the sync. A group is held when either side of it is.
            /// </summary>
            public static bool HasModifier(Modifier mod) => (buffer[0].mods & (uint)mod) != 0;

            /// <summary>
            /// The text the keyboard produced during this frame. Text only crosses through a buffer this side
            /// owns, so this is the call that reads it. Empty when nothing was typed.
            /// </summary>
            public static string CharInput()
            {
                int length = CharInputCall(charHandle.AddrOfPinnedObject(), MaxCharInput);
                return length > 0 ? Encoding.UTF8.GetString(charBuffer, 0, length) : string.Empty;
            }
        }
    }
}
