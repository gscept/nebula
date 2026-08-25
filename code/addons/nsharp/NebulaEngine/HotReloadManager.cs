using System;
using System.IO;
using System.Threading;

namespace Nebula
{
    internal sealed class HotReloadManager
    {
        private GamePluginContext gamePlugin;
        private string pluginPath;
        private int generation;
        private string requestedPath;
        private int reloadRequested;
        private DateTime loadedWriteTime;
        private long loadedLength;

        public void Load(string path)
        {
            this.pluginPath = Path.GetFullPath(path);
            this.gamePlugin = GamePluginContext.Load(this.pluginPath);
            this.RememberBuild(this.pluginPath);
        }

        public void RequestReload(string path)
        {
            if (!String.IsNullOrEmpty(path))
            {
                this.requestedPath = Path.GetFullPath(path);
                Interlocked.Exchange(ref this.reloadRequested, 1);
            }
        }

        public void ProcessPendingReload()
        {
            if (Interlocked.Exchange(ref this.reloadRequested, 0) == 0)
                return;

            string path = this.requestedPath;
            this.requestedPath = null;
            if (String.IsNullOrEmpty(path) || !File.Exists(path))
                return;
            if (!this.HasChanged(path))
                return;

            try
            {
                this.Reload(path);
            }
            catch (Exception exception)
            {
                Console.Error.WriteLine("[NSharp] Full hot reload failed: " + exception);
                this.RequestReload(path);
            }
        }

        public void Unload()
        {
            if (this.gamePlugin != null)
            {
                this.gamePlugin.Unload();
                this.gamePlugin = null;
            }
        }

        private void Reload(string sourcePath)
        {
            string stagedPath = this.Stage(sourcePath);
            HotReloadState state = this.gamePlugin.CaptureState();
            GamePluginContext replacement = null;

            try
            {
                replacement = GamePluginContext.Load(stagedPath, false);
                this.gamePlugin.Unload();
                this.gamePlugin = replacement;
                replacement.RestoreState(state);
                replacement.Start(true);
                this.pluginPath = sourcePath;
                this.RememberBuild(sourcePath);
                Console.WriteLine("[NSharp] Full hot succeeded!");
            }
            catch
            {
                if (replacement != null)
                    replacement.Unload();
                throw;
            }
        }

        private string Stage(string sourcePath)
        {
            string sourceDirectory = Path.GetDirectoryName(sourcePath);
            string stagingDirectory = Path.Combine(
                Path.GetTempPath(), "nebula-hotreload", Path.GetFileNameWithoutExtension(sourcePath),
                (++this.generation).ToString());
            Directory.CreateDirectory(stagingDirectory);

            string[] files = Directory.GetFiles(sourceDirectory);
            for (int i = 0; i < files.Length; i++)
            {
                string destination = Path.Combine(stagingDirectory, Path.GetFileName(files[i]));
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
    }
}
