using System;
using System.Collections.Generic;

namespace Nebula
{
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Delegate | AttributeTargets.Enum, Inherited = false)]
    public sealed class ExportTypeAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Field | AttributeTargets.Property, Inherited = false)]
    public sealed class NoHotReloadAttribute : Attribute { }

    [AttributeUsage(AttributeTargets.Class, Inherited = false)]
    public sealed class HotReloadFactoryAttribute(string methodName) : Attribute
    {
        public string MethodName { get; } = methodName;
    }

    public interface IHotReloadManaged
    {
        void Persisted() { }
        void Destroyed(IDictionary<string, object> state) { }
        void Created(IReadOnlyDictionary<string, object> state) { }
        void Failed() { }
    }
}
