using System;
using System.Runtime.InteropServices;

namespace Nebula
{
    /// <summary>
    /// The ImGui calls the property inspector draws through.
    ///
    /// ImGui has one context, and it is the editor's. A managed drawing library allocates its own, so the
    /// managed side cannot draw with an ImGui of its own and goes through these instead.
    ///
    /// These are only reachable from inside the editor's draw, which is what keeps a build with no editor from
    /// ever calling them.
    ///
    /// A row is the key in column 0 and one widget in column 1. The widget answers with true when the value
    /// changed, and the value it worked on comes back through the buffer the caller gave, because the surface
    /// owns no memory the managed side can read afterwards.
    /// </summary>
    public static class DrawSurface
    {
        /// <summary>
        /// What a section row answered with. A row this side draws can only answer with a number across the
        /// boundary, so this is what the number reads as here.
        /// </summary>
        public enum RowCode
        {
            None = 0,
            Remove = 1,
            Marker = 2,
        }

        // C++ bool is one byte, which is not what a P/Invoke bool marshals as by default
        [DllImport("__Internal", EntryPoint = "DrawSection")]
        private static extern int SectionCall(string title, string propertyName, [MarshalAs(UnmanagedType.I1)] bool authored);

        [DllImport("__Internal", EntryPoint = "DrawBeginTable")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool BeginTableCall(string id);

        [DllImport("__Internal", EntryPoint = "DrawEndTable")]
        private static extern void EndTableCall();

        [DllImport("__Internal", EntryPoint = "DrawRowText")]
        private static extern void RowTextCall(string key, string value, string description);

        [DllImport("__Internal", EntryPoint = "DrawRowTextDisabled")]
        private static extern void RowTextDisabledCall(string key, string value, string description);

        [DllImport("__Internal", EntryPoint = "DrawRowInt")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool RowIntCall(string key, int value, IntPtr outValue, string description);

        [DllImport("__Internal", EntryPoint = "DrawRowFloat")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool RowFloatCall(string key, float value, IntPtr outValue, string description);

        [DllImport("__Internal", EntryPoint = "DrawRowBool")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool RowBoolCall(string key, [MarshalAs(UnmanagedType.I1)] bool value, IntPtr outValue, string description);

        [DllImport("__Internal", EntryPoint = "DrawRowString")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool RowStringCall(string key, string value, IntPtr outBuffer, int outLen, string description);

        [DllImport("__Internal", EntryPoint = "DrawRowCombo")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool RowComboCall(string key, IntPtr options, int index, IntPtr outIndex, string description);

        [DllImport("__Internal", EntryPoint = "DrawRowFloats")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool RowFloatsCall(string key, IntPtr values, int count, string description);

        [DllImport("__Internal", EntryPoint = "DrawPushId")]
        private static extern void PushIdCall(string id);

        [DllImport("__Internal", EntryPoint = "DrawPopId")]
        private static extern void PopIdCall();

        [DllImport("__Internal", EntryPoint = "DrawTextDisabled")]
        private static extern void TextDisabledCall(string text);

        [DllImport("__Internal", EntryPoint = "DrawRowMarker")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool RowMarkerCall(string key, [MarshalAs(UnmanagedType.I1)] bool authored, string tip);

        [DllImport("__Internal", EntryPoint = "DrawIsAnyItemActive")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool IsAnyItemActiveCall();

        /// <summary>
        /// The section row. The remove button is drawn when the property name is given, and the override marker
        /// when the caller says the property has keys this instance authored.
        /// </summary>
        public static RowCode Section(string title, string propertyName, bool authored) => (RowCode)SectionCall(title, propertyName, authored);

        // EndTable is called even when BeginTable answered false, because ImGui still wants the table ended for
        // one that was not drawn
        public static bool BeginTable(string id) => BeginTableCall(id);

        public static void EndTable() => EndTableCall();

        public static void RowText(string key, string value, string description) => RowTextCall(key, value, description);

        public static void RowTextDisabled(string key, string value, string description) => RowTextDisabledCall(key, value, description);

        public static unsafe bool RowInt(string key, int value, out int result, string description)
        {
            int current = value;
            bool changed = RowIntCall(key, value, (IntPtr)(&current), description);
            result = current;
            return changed;
        }

        public static unsafe bool RowFloat(string key, float value, out float result, string description)
        {
            float current = value;
            bool changed = RowFloatCall(key, value, (IntPtr)(&current), description);
            result = current;
            return changed;
        }

        public static unsafe bool RowBool(string key, bool value, out bool result, string description)
        {
            bool current = value;
            bool changed = RowBoolCall(key, value, (IntPtr)(&current), description);
            result = current;
            return changed;
        }

        /// <summary>
        /// The value the user typed is written into the buffer the caller gave.
        /// </summary>
        public static unsafe bool RowString(string key, string value, byte[] buffer, string description)
        {
            bool changed;
            fixed (byte* bufferPtr = &buffer[0])
            {
                changed = RowStringCall(key, value, (IntPtr)bufferPtr, buffer.Length, description);
            }
            return changed;
        }

        /// <summary>
        /// The options are the names separated by a zero byte and terminated by two, which is the form ImGui's
        /// combo takes. The buffer has to be one the caller owns.
        /// </summary>
        public static unsafe bool RowCombo(string key, byte[] options, int index, out int result, string description)
        {
            int current = index;
            bool changed;
            fixed (byte* optionsPtr = &options[0])
            {
                changed = RowComboCall(key, (IntPtr)optionsPtr, index, (IntPtr)(&current), description);
            }
            result = current;
            return changed;
        }

        /// <summary>
        /// The components of one field. The values are written back into the buffer the caller gave, which is
        /// what lets one call cover Vector2, Vector3, Vector4 and Quaternion.
        /// </summary>
        public static unsafe bool RowFloats(string key, float[] values, int count, string description)
        {
            bool changed;
            fixed (float* valuesPtr = &values[0])
            {
                changed = RowFloatsCall(key, (IntPtr)valuesPtr, count, description);
            }
            return changed;
        }

        public static void PushId(string id) => PushIdCall(id);

        public static void PopId() => PopIdCall();

        public static void TextDisabled(string text) => TextDisabledCall(text);

        /// <summary>
        /// The override marker on a row, drawn after the widget the row's value column has. It is only drawn when
        /// the caller says the row is the instance's own, and the answer is true when it was pressed. What a press
        /// means is not this side's decision: it only says which row it was on.
        /// </summary>
        public static bool RowMarker(string key, bool authored, string tip) => RowMarkerCall(key, authored, tip);

        public static bool IsAnyItemActive() => IsAnyItemActiveCall();
    }
}
