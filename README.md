# VS Code Config Helper (v3)

[![](https://img.shields.io/github/workflow/status/Guyutongxue/VSCodeConfigHelper3/xmake)](https://github.com/Guyutongxue/VSCodeConfigHelper3/actions/workflows/xmake.yml)

## 支持平台

- GUI & CLI: Windows 10 及以上
- CLI:
    - Ubuntu 或其它基于 Debian 的 Linux 发行版
    - Intel 芯片 Mac（注：Apple 芯片配置存在缺陷）

## 使用方法

### GUI（仅 Windows）

按界面提示完成配置。参考此处[演示](https://b23.tv/av292212272)。

### CLI

使用例：

```sh
# 配置文件夹 <Workspace Folder Path>, 外部弹窗终端，配置后启动 vscode
./vscch -eoy -w <Workspace Folder Path>
```

键入 `./vscch -h` 以查看完整选项列表。

## 构建方法

安装 [xmake](https://xmake.io/#/zh-cn/) 。执行以下命令：

```sh
xmake
```

xmake 会处理好包依赖和工程配置，并将可执行文件生成到 `build/bin` 文件夹中。
