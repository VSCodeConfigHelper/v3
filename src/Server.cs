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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using Serilog;

namespace VSCodeConfigHelper3 {


    class Server {

        static HttpListener listener = new HttpListener();
        static string url = "http://localhost:8000/";
        static string pageData = @"
<!DOCTYPE html>
<html>
<head>Error</head>
<body>Failed to load HTML from assembly. Try to restart the program.</body>
</html>";

        Dictionary<string, RequestHandler> handlers = new();

        bool runServer = false;

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
            Log.Information($"开始监听来自 {url} 的请求……");
        }

        public void AddHandler(string path, RequestHandler handler, bool shutdown = false) {
            if (handlers.ContainsKey(path)) {
                throw new Exception("Handler already exists for path: " + path);
            }
            if (shutdown) {
                handlers.Add(path, (req) => {
                    runServer = false;
                    return handler(req);
                });
            } else {
                handlers.Add(path, handler);
            }
        }

        // Try to start HTTPListener with a free port.
        private static bool StartListener() {
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

        public async Task HandleIncomingConnections() {
            runServer = true;

            // While a user hasn't visited the `shutdown` url, keep on handling requests
            while (runServer && listener is not null) {
                HttpListenerContext ctx = await listener.GetContextAsync();
                HttpListenerRequest req = ctx.Request;
                HttpListenerResponse res = ctx.Response;

                if (req.Url is null) continue;

                Log.Debug($"Request received: {req.HttpMethod} {req.Url.ToString()}");

                byte[] resBody, reqBody;
                string resBodyStr, reqBodyStr;

                reqBody = new byte[req.ContentLength64];
                await req.InputStream.ReadAsync(reqBody, 0, reqBody.Length);
                reqBodyStr = Encoding.UTF8.GetString(reqBody);

                if (req.HttpMethod == "POST") {
                    Log.Debug($"Request Body: {reqBodyStr}");
                    if (handlers.ContainsKey(req.Url.AbsolutePath)) {
                        RequestHandler handler = handlers[req.Url.AbsolutePath];
                        resBodyStr = handler(reqBodyStr);
                    } else {
                        resBodyStr = "unknown api";
                        res.StatusCode = 404;
                    }
                    Log.Debug($"Response Body: {resBodyStr}");
                    res.ContentType = "text/plain";
                } else if (req.Url.AbsolutePath == "/") {
                    resBodyStr = pageData;
                    res.ContentType = "text/html";
                } else {
                    res.StatusCode = 404;
                    resBodyStr = "not found";
                    res.ContentType = "text/plain";
                }
                resBody = Encoding.UTF8.GetBytes(resBodyStr);

                res.AddHeader("Access-Control-Allow-Origin", "*");
                res.ContentEncoding = Encoding.UTF8;
                res.ContentLength64 = resBody.LongLength;
                await res.OutputStream.WriteAsync(resBody, 0, resBody.Length);

                res.Close();
            }
        }

        static public void OpenBrowser(string url) {
            Log.Information($"正在启动浏览器，页面 URL：{url}");
            Process.Start(new ProcessStartInfo {
                FileName = Environment.GetEnvironmentVariable("ComSpec") ?? @"C:\Windows\system32\cmd.exe",
                ArgumentList = {
                    "/c",
                    "start",
                    url
                }
            });
        }
    }
}