using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;

namespace VSCodeConfigHelper3 {

    class Program {


        static void GetFolder() {
        }

        static void Main(string[] args) {
            var fg = new FolderGetter();
            // var folder = fg.Get();
            var er = new EnvResolver();
            Console.WriteLine(er.VscodePath);
            foreach (var i in er.Compilers) {
                Console.WriteLine(i.Path);
                Console.WriteLine(i.Version);
            }
            // Console.WriteLine(folder);
        }
    }
}
