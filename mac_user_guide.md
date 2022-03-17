# VSCodeConfigHelper Mac 用户指南

## 下载工具

前往[工具主页](https://v3.vscch.tk/)下载此工具；然后，解压它；得到一个 `VSCodeConfigHelper_v3.x.x_mac` 文件夹。

[启动终端应用程序](https://support.apple.com/zh-cn/guide/terminal/apd5265185d-f365-44cb-8b09-71a064a42125/mac)。键入 `cd `（<kbd>c</kbd><kbd>d</kbd><kbd>空格</kbd>），然后将刚才的文件夹拖入终端内，最后按下回车 <kbd>return</kbd>。（即输入了以下命令：
```shell
cd /Users/.../Downloads/VSCodeConfigHelper_v3.x.x_mac
```
。或者，如果你启用了在文件夹内打开终端的服务，你可以直接在该文件夹下打开终端。）

## 首次设置

在终端中键入 `./vscch` 并按回车。第一次启动时会提示因安全原因无法运行。此时打开苹果图标，定位到“系统偏好设置”-“安全性与隐私”，在“通用”选项卡的下方对 `vscch` 程序点击“仍然允许”。

再次尝试启动工具，键入以下命令并按回车：
```shell
./vscch -n -w ~/Desktop/Cpp
```
注意这里 `-w` 后面是你要配置的工作区文件夹路径，这里使用了 `~/Desktop/Cpp`，即桌面下的 `Cpp` 文件夹。`-n` 选项表明启用新手模式。关于新手模式以及更多选项可通过 `./vscch -h` 查看。

> 若使用新手模式，为了更好的体验，建议在终端的菜单栏中定位到“终端”-“偏好设置”-“Shell”，将“当Shell退出时”设置为“关闭窗口”。

如果工具显示 `未安装 Xcode Command Line Tools，将进行安装` 的提示，则按照屏幕说明操作。安装 Xcode Command Line Tools 需要几分钟到数十分钟不等。安装完成后，再次执行上述命令来继续配置。

## 配置完成

工具配置完成后，若为新手模式（或带有 `--open-vscode` 选项），则工具会自动启动 VS Code 并打开工作区文件夹。尝试在该文件夹下编译、运行或调试 C++。

**注意** Mac 并不支持内置终端。换而言之，不论是否启用 `--external-console` 进行配置，运行和调试时都会弹出终端窗口。但 `--external-console` 会使用更加美观的暂停脚本。若不使用 `--external-console`，则“当Shell退出时”不能设置为“关闭窗口”：否则运行时无法观察到程序输出结果。
