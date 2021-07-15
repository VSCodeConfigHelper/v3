// Copyright (C) 2021 Guyutongxue
// 
// This file is part of VSCodeConfigHelper.
// 
// VSCodeConfigHelper is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// VSCodeConfigHelper is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with VSCodeConfigHelper.  If not, see <http://www.gnu.org/licenses/>.

using System;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace VSCodeConfigHelper3 {

    class FolderGetter {


        [DllImport("kernel32.dll", ExactSpelling = true)]
        private static extern IntPtr GetConsoleWindow();

        [DllImport("user32.dll")]
        [return: MarshalAs(UnmanagedType.Bool)]
        private static extern bool SetForegroundWindow(IntPtr hWnd);

        // bring console to front before launching FolderBrowserDialog
        // https://stackoverflow.com/a/12066376
        private void BringConsoleToFront() {
            SetForegroundWindow(GetConsoleWindow());
        }


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
            BringConsoleToFront();
            try {
                var proc = new Process {
                    StartInfo = new ProcessStartInfo {
                        FileName = powershellLocation,
                        ArgumentList = {
                            "-Sta",
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

        public static HttpListener listener = new HttpListener();
        public static string url = "http://localhost:8000/";
        public static string pageData = @"
<!DOCTYPE html>
<html>
<head>Error</head>
<body>Failed to load HTML from assembly. Try to restart the program.</body>
</html>";

        static FolderGetter fg = new FolderGetter();

        public Server(out string servingUrl) {
            Assembly current = typeof(Program).GetTypeInfo().Assembly;
            using (var reader = current.GetManifestResourceStream(@"VSCodeConfigHelper3.pages.config.html")) {
                if (reader is null) throw new FileNotFoundException("Cannot read HTML page from assembly.");
                pageData = (new StreamReader(reader)).ReadToEnd();
            }
            listener.Prefixes.Add(url);
            if (!StartListener()) {
                throw new Exception("Could not start listener.");
            }
            servingUrl = url;
            Console.WriteLine("Listening for connections on {0}", url);
        }

        // Try to start HTTPListener with a free port.
        public static bool StartListener() {
            // Try default port first.
            try {
                listener.Start();
                return true;
            }
            catch (HttpListenerException) { }

            // Try other ports.
            // IANA suggested range for dynamic or private ports
            const int minPort = 49215;
            const int maxPort = 65535;

            for (int port = minPort; port < maxPort; port++) {
                listener = new HttpListener();
                url = $"http://localhost:{port}/";
                listener.Prefixes.Add(url);
                try {
                    listener.Start();
                    return true;
                }
                catch (HttpListenerException) { }
            }
            return false;
        }

        public void Run() {
            // Handle requests
            Task listenTask = HandleIncomingConnections();
            listenTask.GetAwaiter().GetResult();

            // Close the listener
            if (listener is not null) {
                listener.Close();
            }
        }

        public static async Task HandleIncomingConnections() {
            bool runServer = true;

            // While a user hasn't visited the `shutdown` url, keep on handling requests
            while (runServer && listener is not null) {
                // Will wait here until we hear from a connection
                HttpListenerContext ctx = await listener.GetContextAsync();

                // Peel out the requests and response objects
                HttpListenerRequest req = ctx.Request;
                HttpListenerResponse res = ctx.Response;

                // Print out some info about the request
                Console.WriteLine("Request");
                Console.WriteLine(req.Url?.ToString());
                Console.WriteLine(req.HttpMethod);
                Console.WriteLine(req.UserHostName);
                Console.WriteLine(req.UserAgent);
                Console.WriteLine();

                // If `shutdown` url requested w/ POST, then shutdown the server after serving the page
                byte[] data;
                if ((req.HttpMethod == "POST") && (req.Url?.AbsolutePath == "/shutdown")) {
                    Console.WriteLine("Shutdown requested");
                    runServer = false;
                }
                if (req.Url?.AbsolutePath == "/getFolder") {
                    var result = fg.Get();
                    if (result is null) {
                        data = Encoding.UTF8.GetBytes("null");
                    } else {
                        data = Encoding.UTF8.GetBytes(result);
                    }
                    res.ContentType = "text/plain";
                    res.ContentEncoding = Encoding.UTF8;
                    res.ContentLength64 = data.LongLength;
                } else if (req.Url?.AbsolutePath == "/") {
                    // Write the response info
                    data = Encoding.UTF8.GetBytes(pageData);
                    res.ContentType = "text/html";
                    res.ContentEncoding = Encoding.UTF8;
                    res.ContentLength64 = data.LongLength;
                } else {
                    res.StatusCode = 404;
                    data = Encoding.UTF8.GetBytes("not found");
                    res.ContentType = "text/plain";
                    res.ContentEncoding = Encoding.UTF8;
                    res.ContentLength64 = data.LongLength;
                }
                // Write out to the response stream (asynchronously), then close it
                await res.OutputStream.WriteAsync(data, 0, data.Length);
                res.Close();
            }
        }
    }
}