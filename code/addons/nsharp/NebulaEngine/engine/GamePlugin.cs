using System;
using System.IO;
using System.Reflection;
using System.Runtime.Loader;

namespace Nebula
{
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = false)]
    public sealed class NebulaGamePluginAttribute(Type entryPointType) : Attribute
    {
        public Type EntryPointType { get; } = entryPointType;
    }

    public interface IGamePlugin
    {
        void OnLoad(GamePluginContext context);
        void OnUnload();
    }

    public sealed class GamePluginContext
    {
        private sealed class PluginLoadContext(string pluginPath) : AssemblyLoadContext(Path.GetFileNameWithoutExtension(pluginPath), true)
        {
            private readonly AssemblyDependencyResolver resolver = new AssemblyDependencyResolver(pluginPath);
            private readonly Assembly engineAssembly = typeof(IGamePlugin).Assembly;

            protected override Assembly Load(AssemblyName assemblyName)
            {
                if (assemblyName.Name == this.engineAssembly.GetName().Name)
                {
                    return this.engineAssembly;
                }

                string assemblyPath = this.resolver.ResolveAssemblyToPath(assemblyName);
                return assemblyPath != null ? this.LoadFromAssemblyPath(assemblyPath) : null;
            }

            protected override IntPtr LoadUnmanagedDll(string unmanagedDllName)
            {
                string libraryPath = this.resolver.ResolveUnmanagedDllToPath(unmanagedDllName);
                return libraryPath != null ? this.LoadUnmanagedDllFromPath(libraryPath) : IntPtr.Zero;
            }
        }

        private PluginLoadContext loadContext;
        private Assembly assembly;
        private IGamePlugin plugin;
        private Nebula.Game.NebulaApp application;

        public Assembly Assembly => this.assembly;
        private GamePluginContext()
        {
        }

        public void SetApplication(Nebula.Game.NebulaApp application)
        {
            if (this.application != null)
            {
                throw new InvalidOperationException("The game plugin has already configured an application.");
            }
            this.application = application ?? throw new ArgumentNullException(nameof(application));
        }

        internal static GamePluginContext Load(string pluginPath, bool start = true)
        {
            string fullPath = Path.GetFullPath(pluginPath);
            if (!File.Exists(fullPath))
            {
                throw new FileNotFoundException("Could not find the game plugin assembly.", fullPath);
            }

            GamePluginContext context = new GamePluginContext();
            try
            {
                context.loadContext = new PluginLoadContext(fullPath);
                context.assembly = context.loadContext.LoadFromAssemblyPath(fullPath);

                NebulaGamePluginAttribute attribute = context.assembly.GetCustomAttribute<NebulaGamePluginAttribute>() ?? throw new InvalidOperationException("The game plugin assembly is missing NebulaGamePluginAttribute.");
                if (!typeof(IGamePlugin).IsAssignableFrom(attribute.EntryPointType) || attribute.EntryPointType.IsAbstract)
                {
                    throw new InvalidOperationException("The configured game plugin entrypoint must be a non-abstract IGamePlugin type.");
                }

                context.plugin = (IGamePlugin)Activator.CreateInstance(attribute.EntryPointType);
                Nebula.Runtime.Setup(context.assembly);
                context.plugin.OnLoad(context);

                if (context.application == null)
                {
                    throw new InvalidOperationException("The game plugin did not configure a NebulaApp.");
                }

                if (start)
                {
                    context.Start();
                }

                return context;
            }
            catch
            {
                context.Discard();
                throw;
            }
        }

        internal HotReloadState PrepareMigration(GamePluginContext replacement)
        {
            HotReloadState state = new HotReloadState(this.assembly, replacement.assembly);
            state.PrepareProperties();
            state.Migrate(this.plugin, replacement.plugin, this.application, replacement.application);
            return state;
        }

        internal static void AttachMigratedProperties(HotReloadState state)
        {
            for (int i = 0; i < state.Properties.Count; i++)
            {
                HotReloadState.PropertyReplacement replacement = state.Properties[i];
                replacement.NewProperty.Entity = replacement.Entity;
            }
            state.Complete();
            for (int i = 0; i < state.Properties.Count; i++)
            {
                HotReloadState.PropertyReplacement replacement = state.Properties[i];
                replacement.Entity.AddProperty(replacement.NewProperty, replacement.Active);
            }
        }

        internal void Start(bool resume = false)
        {
            Nebula.Runtime.OverrideNebulaApp(this.application);
            if (resume)
            {
                this.application.Resume();
            }
            else
            {
                Nebula.Runtime.Start();
            }
        }

        internal WeakReference Unload()
        {
            PluginLoadContext oldLoadContext = this.loadContext;
            WeakReference weakReference = new WeakReference(oldLoadContext);
            try
            {
                TryCleanup(Runtime.Close, "application shutdown");
                TryCleanup(() => this.plugin.OnUnload(), "plugin unload");
                TryCleanup(Nebula.Game.World.ClearAllProperties, "property detach");
                TryCleanup(Game.PropertyManager.Instance.Clear, "property manager cleanup");
                TryCleanup(() => Nebula.Game.ComponentManager.Instance.ClearPluginComponents(this.assembly), "component cleanup");
                TryCleanup(Runtime.ResetNebulaApp, "application reset");
            }
            finally
            {
                this.application = null;
                this.plugin = null;
                this.assembly = null;
                this.loadContext = null;
                if (oldLoadContext != null)
                {
                    oldLoadContext.Unload();
                }
            }
            return weakReference;
        }

        internal void Discard()
        {
            PluginLoadContext oldLoadContext = this.loadContext;
            try
            {
                if (this.plugin != null)
                {
                    TryCleanup(() => this.plugin.OnUnload(), "cleanup");
                }

                if (this.assembly != null)
                {
                    TryCleanup(() => Nebula.Game.ComponentManager.Instance.ClearPluginComponents(this.assembly), "component cleanup");
                }
            }
            finally
            {
                this.application = null;
                this.plugin = null;
                this.assembly = null;
                this.loadContext = null;
                if (oldLoadContext != null)
                {
                    oldLoadContext.Unload();
                }
            }
        }

        private static void TryCleanup(Action action, string operation)
        {
            try
            {
                action();
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Plugin " + operation + " failed: " + exception);
            }
        }
    }
}
