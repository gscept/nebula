using System;
using System.Text;

namespace Util
{
    public static class StringInterop
    {
        public static void CopyUtf8(string value, Span<byte> destination, out int byteCount)
        {
            byteCount = Encoding.UTF8.GetBytes(value.AsSpan(), destination);
        }
    }
}
