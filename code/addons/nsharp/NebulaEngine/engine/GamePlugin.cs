using System;
using System.IO;
using System.Reflection;
using System.Runtime.Loader;

namespace Nebula
{
    [AttributeUsage(AttributeTargets.Assembly, AllowMultiple = false)]
    public sealed class NebulaGamePluginAttribute : Attribute
    {
        public Type EntryPointType { get; }

        public NebulaGamePluginAttribute(Type entryPointType)
        {
            this.EntryPointType = entryPointType;
        }
    }

    public interface IGamePlugin
    {
        void OnLoad(GamePluginContext context);
        void OnUnload();
    }

    public sealed class GamePluginContext
    {
        private sealed class PluginLoadContext : AssemblyLoadContext
        {
            private readonly AssemblyDependencyResolver resolver;
            private readonly Assembly engineAssembly;

            public PluginLoadContext(string pluginPath)
                : base(Path.GetFileNameWithoutExtension(pluginPath), true)
            {
                this.resolver = new AssemblyDependencyResolver(pluginPath);
                this.engineAssembly = typeof(IGamePlugin).Assembly;
            }

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

        public Assembly Assembly { get { return this.assembly; } }
        private GamePluginContext()
        {
        }

        public void SetApplication(Nebula.Game.NebulaApp application)
        {
            if (application == null)
            {
                throw new ArgumentNullException(nameof(application));
            }
            if (this.application != null)
            {
                throw new InvalidOperationException("The game plugin has already configured an application.");
            }
            this.application = application;
        }

        internal static GamePluginContext Load(string pluginPath, bool start = true)
        {
            string fullPath = Path.GetFullPath(pluginPath);
            if (!File.Exists(fullPath))
            {
                throw new FileNotFoundException("Could not find the game plugin assembly.", fullPath);
            }

            GamePluginContext context = new GamePluginContext();
            context.loadContext = new PluginLoadContext(fullPath);
            context.assembly = context.loadContext.LoadFromAssemblyPath(fullPath);

            NebulaGamePluginAttribute attribute = context.assembly.GetCustomAttribute<NebulaGamePluginAttribute>();
            if (attribute == null)
            {
                throw new InvalidOperationException("The game plugin assembly is missing NebulaGamePluginAttribute.");
            }
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

        internal HotReloadState CaptureState()
        {
            HotReloadState state = new HotReloadState();
            state.Capture("plugin", this.plugin);
            state.Capture("application", this.application);
            state.CaptureProperties(Nebula.Game.World.Get(Nebula.Game.World.DEFAULT_WORLD));
            return state;
        }

        internal void RestoreState(HotReloadState state)
        {
            state.Restore("plugin", this.plugin);
            state.Restore("application", this.application);
            state.RestoreProperties(this.assembly, Nebula.Game.World.Get(Nebula.Game.World.DEFAULT_WORLD));
        }

        internal void Start(bool resume = false)
        {
            Nebula.Runtime.OverrideNebulaApp(this.application);
            if (resume)
                this.application.Resume();
            else
                Nebula.Runtime.Start();
        }

        internal void Unload()
        {
            Nebula.Runtime.Close();
            this.plugin.OnUnload();
            Nebula.Game.World.Get(Nebula.Game.World.DEFAULT_WORLD).ClearProperties();
            Nebula.Game.PropertyManager.Instance.Clear();
            Nebula.Game.ComponentManager.Instance.ClearPluginComponents(this.assembly);
            Nebula.Runtime.ResetNebulaApp();

            this.application = null;
            this.plugin = null;
            this.assembly = null;

            PluginLoadContext oldLoadContext = this.loadContext;
            this.loadContext = null;
            oldLoadContext.Unload();
        }
    }
}
