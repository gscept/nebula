using System;
using System.IO;
using System.Collections.Generic;

namespace Nebula
{
    internal sealed class HotReloadManager
    {
        private sealed class ReloadRequest
        {
            internal string Path;
            internal bool Force;
        }

        private sealed class RetiredContext
        {
            internal WeakReference Reference;
            internal int CollectionAttempts;
        }

        private GamePluginContext gamePlugin;
        private string pluginPath;
        private int generation;
        private readonly object requestLock = new object();
        private ReloadRequest pendingRequest;
        private DateTime loadedWriteTime;
        private long loadedLength;
        private string loadedStagingDirectory;
        private readonly List<RetiredContext> unloadedContexts = [];

        public void Load(string path)
        {
            this.pluginPath = Path.GetFullPath(path);
            this.gamePlugin = GamePluginContext.Load(this.pluginPath);
            this.RememberBuild(this.pluginPath);
        }

        public void RequestReload(string path, bool force = false)
        {
            if (!String.IsNullOrEmpty(path))
            {
                lock (this.requestLock)
                {
                    this.pendingRequest = new ReloadRequest {
                        Path = Path.GetFullPath(path),
                        Force = force || (this.pendingRequest != null && this.pendingRequest.Force)
                    };
                }
            }
        }

        public void RequestReload(bool force)
        {
            this.RequestReload(this.pluginPath, force);
        }

        public void ProcessPendingReload()
        {
            this.CollectUnloadedContexts();
            ReloadRequest request;
            lock (this.requestLock)
            {
                request = this.pendingRequest;
                this.pendingRequest = null;
            }
            if (request == null)
            {
                return;
            }

            string path = request.Path;
            bool force = request.Force;
            if (String.IsNullOrEmpty(path) || !File.Exists(path))
            {
                return;
            }

            if (!force && !this.HasChanged(path))
            {
                return;
            }

            try
            {
                this.Reload(path);
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Full hot reload failed: " + exception);
                if (this.gamePlugin != null)
                {
                    this.RequestReload(path, force);
                }
            }
        }

        public void Unload()
        {
            if (this.gamePlugin != null)
            {
                this.TrackUnloadedContext(this.gamePlugin.Unload());
                this.gamePlugin = null;
            }
            DeleteStagingDirectory(this.loadedStagingDirectory);
            this.loadedStagingDirectory = null;
        }

        private void Reload(string sourcePath)
        {
            string stagedPath = this.Stage(sourcePath);
            GamePluginContext replacement = null;
            bool oldUnloaded = false;

            try
            {
                replacement = GamePluginContext.Load(stagedPath, false);
                HotReloadState state = this.gamePlugin.PrepareMigration(replacement);
                this.TrackUnloadedContext(this.gamePlugin.Unload());
                oldUnloaded = true;
                this.gamePlugin = replacement;
                replacement = null;
                GamePluginContext.AttachMigratedProperties(state);
                this.gamePlugin.Start(true);
                DeleteStagingDirectory(this.loadedStagingDirectory);
                this.loadedStagingDirectory = Path.GetDirectoryName(stagedPath);
                this.pluginPath = sourcePath;
                this.RememberBuild(sourcePath);
                Console.WriteLine("[NSharp] Full hot succeeded!");
            }
            catch
            {
                if (replacement != null)
                {
                    replacement.Discard();
                }

                if (oldUnloaded && this.gamePlugin != null)
                {
                    this.TrackUnloadedContext(this.gamePlugin.Unload());
                    this.gamePlugin = null;
                }
                throw;
            }
        }

        private string Stage(string sourcePath)
        {
            string sourceDirectory = Path.GetDirectoryName(sourcePath);
            string stagingDirectory = Path.Combine(
                Path.GetTempPath(), "nebula-hotreload", Path.GetFileNameWithoutExtension(sourcePath),
                (++this.generation).ToString());
            _ = Directory.CreateDirectory(stagingDirectory);

            string[] files = Directory.GetFiles(sourceDirectory, "*", SearchOption.AllDirectories);
            for (int i = 0; i < files.Length; i++)
            {
                if (!ShouldStage(files[i]))
                {
                    continue;
                }

                string relativePath = Path.GetRelativePath(sourceDirectory, files[i]);
                string destination = Path.Combine(stagingDirectory, relativePath);
                _ = Directory.CreateDirectory(Path.GetDirectoryName(destination));
                File.Copy(files[i], destination, true);
            }

            return Path.Combine(stagingDirectory, Path.GetFileName(sourcePath));
        }

        private bool HasChanged(string path)
        {
            return File.GetLastWriteTimeUtc(path) != this.loadedWriteTime ||
                new FileInfo(path).Length != this.loadedLength;
        }

        private void RememberBuild(string path)
        {
            FileInfo file = new FileInfo(path);
            this.loadedWriteTime = file.LastWriteTimeUtc;
            this.loadedLength = file.Length;
        }

        private static void DeleteStagingDirectory(string path)
        {
            if (String.IsNullOrEmpty(path) || !Directory.Exists(path))
            {
                return;
            }

            try
            {
                Directory.Delete(path, true);
            }
            catch (IOException)
            {
                // A loaded dependency can keep the shadow directory locked until shutdown.
            }
            catch (UnauthorizedAccessException)
            {
            }
        }

        private static bool ShouldStage(string path)
        {
            string extension = Path.GetExtension(path);
            return extension.Equals(".dll", StringComparison.OrdinalIgnoreCase)
                || extension.Equals(".pdb", StringComparison.OrdinalIgnoreCase)
                || extension.Equals(".json", StringComparison.OrdinalIgnoreCase)
                || extension.Equals(".so", StringComparison.OrdinalIgnoreCase)
                || extension.Equals(".dylib", StringComparison.OrdinalIgnoreCase);
        }

        private void CollectUnloadedContexts()
        {
            if (this.unloadedContexts.Count == 0)
            {
                return;
            }

            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
            for (int i = this.unloadedContexts.Count - 1; i >= 0; i--)
            {
                RetiredContext context = this.unloadedContexts[i];
                if (!context.Reference.IsAlive)
                {
                    this.unloadedContexts.RemoveAt(i);
                }
                else if (++context.CollectionAttempts >= 3)
                {
                    Console.Error.WriteLine("[NSharp] A retired plugin AssemblyLoadContext is still alive after three collection attempts.");
                    this.unloadedContexts.RemoveAt(i);
                }
            }
        }

        private void TrackUnloadedContext(WeakReference reference)
        {
            this.unloadedContexts.Add(new RetiredContext { Reference = reference });
        }
    }
}
