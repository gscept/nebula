using System;
using System.Runtime.InteropServices;

namespace Util
{
    public unsafe struct String
    {
        public string heap;
        public fixed char local[20];
        public int strLen;
        public int heapBufferSize;
    }

    public class StringMarshaler : ICustomMarshaler {

		private static StringMarshaler Instance = new StringMarshaler ();

		public static ICustomMarshaler GetInstance (string s)
		{
			return Instance;
		}

		public void CleanUpManagedData (object o)
		{
		}

		public void CleanUpNativeData (IntPtr pNativeData)
		{
            Marshal.FreeHGlobal(pNativeData);
		}

		public int GetNativeDataSize ()
		{
			return IntPtr.Size;
		}

		public IntPtr MarshalManagedToNative (object obj)
		{
			string s = obj as string;
			if (s == null)
				return IntPtr.Zero;
                
			Util.String str;
            str.heap = s;
            str.strLen = s.Length;
            str.heapBufferSize = (s.Length + 1) * sizeof(byte); // 8 bit characters

            IntPtr ret = Marshal.AllocHGlobal(System.Runtime.InteropServices.Marshal.SizeOf(typeof(Util.String)));
            Marshal.StructureToPtr(str, ret, false);
			return ret;
		}

		public object MarshalNativeToManaged (IntPtr pNativeData)
		{
			string s = Marshal.PtrToStringAnsi(pNativeData);
            // Console.WriteLine($"# StringMarshaler.MarshalNativeToManaged ({pNativeData:x})=`{s}'\n");
			return s;
		}
	}
}
