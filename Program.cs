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
using System.Reflection;

namespace VSCodeConfigHelper3 {

    class Program {


        static void GetFolder() {
        }

        static void Main(string[] args) {
            // resolve environment
            var er = new EnvResolver();
            Console.WriteLine(er.VscodePath);
            foreach (var i in er.Compilers) {
                Console.WriteLine(i.Path);
                Console.WriteLine(i.VersionNumber);
                Console.WriteLine(i.PackageString);
            }
            // launch server
            var s = new Server(out string url);
            // open browser
            Process.Start(new ProcessStartInfo {
                FileName = Environment.GetEnvironmentVariable("ComSpec") ?? @"C:\Windows\system32\cmd.exe",
                ArgumentList = {
                    "/c",
                    "start",
                    url
                }
            });
            s.Run();
        }
    }
}
