using System;
using System.Collections.Generic;

public static class ListExt
{
    /// Erase by swapping with last value and erasing from end.
    public static void EraseSwap<T>(this List<T> list, int index)
    {
        list[index] = list[list.Count - 1];
        list.RemoveAt(list.Count - 1);
    }

    /// Erase by swapping with last value and erasing from end.
    public static void EraseSwap<T>(this List<T> list, T item)
    {
        int index = list.IndexOf(item);
        EraseSwap(list, index);
    }

    /// Erase by swapping with last value and erasing from end.
    public static void EraseSwap<T>(this List<T> list, Predicate<T> predicate)
    {
        int index = list.FindIndex(predicate);
        EraseSwap(list, index);
    }
}
