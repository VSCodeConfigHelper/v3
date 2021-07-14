using System;
using System.Reflection;
using System.IO;
using System.Diagnostics;

namespace VSCodeConfigHelper3 {

    class FolderGetter {

        private string scriptLocation;
        private string powershellLocation;

        private void GenerateScript() {
            Assembly current = typeof(Program).GetTypeInfo().Assembly;
            using (var reader = current.GetManifestResourceStream(@"VSCodeConfigHelper3.scripts.get-folder.ps1")) {
                if (reader is null) {
                    throw new FileNotFoundException("Cannot read PowerShell script from assembly.");
                }
                if (File.Exists(scriptLocation)) {
                    File.Delete(scriptLocation);
                }
                using (var writer = File.Create(scriptLocation)) {
                    reader.Seek(0, SeekOrigin.Begin);
                    reader.CopyTo(writer);
                }
            }
        }

        public FolderGetter() {
            scriptLocation = Path.Join(Path.GetTempPath(), "get-folder.ps1");
            powershellLocation = Environment.ExpandEnvironmentVariables(@"%SystemRoot%\system32\WindowsPowerShell\v1.0\powershell.exe");
            GenerateScript();
        }

        public string? Get() {
            if (!File.Exists(scriptLocation)) {
                GenerateScript();
            }
            try {
                var proc = new Process {
                    StartInfo = new ProcessStartInfo {
                        FileName = powershellLocation,
                        ArgumentList = {
                            "-ExecutionPolicy",
                            "Bypass",
                            "-File",
                            scriptLocation
                        },
                        RedirectStandardOutput = true,
                        CreateNoWindow = true,
                    }
                };
                proc.Start();
                string output = proc.StandardOutput.ReadToEnd();
                proc.WaitForExit();
                if (string.IsNullOrWhiteSpace(output)) {
                    return null;
                }
                return output;
            }
            catch (Exception ex) {
                Debug.WriteLine(ex.ToString());
                return null;
            }

        }
    }

    class Server {

    }
}