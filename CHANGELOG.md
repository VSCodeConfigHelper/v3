# 更新日志 Change Log

## \[3.1.1\]
*Pending*

- 优化了 macOS 下的外置终端脚本；
- 增加 Windows 下选择 32 位编译器的警告。

## 3.1.0
*2021.9.8*

- 增加 Linux（仅 Debian 系）和 macOS 支持。仅 CLI。
- macOS 下，工具会尝试安装 Xcode Command Line Tools。此功能未经测试。
- 前端增设 `-fexec-charset=GBK` 编译选项。仅当操作系统使用 GBK 代码页时启用。
- 可能的破坏性更新：
    - 更正错误的变量命名。版本 3.0.x 应尽快弃用，后续可能移除对其的支持。

## 3.0.2
*2021.8.14*

- 修复错误的测试文件注释

## 3.0.1
*2021.8.8*

- 修复 [#2](https://github.com/Guyutongxue/VSCodeConfigHelper3/issues/2)

## 3.0.0
*2021.8.7*

- 添加离线下载 C/C++ 扩展的选项

## \[3.0.0~beta0805\]
*2021.8.5*

- 将全部后端代码迁移至 C++，以减少发布文件体积。
 
（提示：建议使用 MSVC 编译器构建，以加快构建速度及优化目标体积。）

## 3.0.0~beta0724
*2021.7.24*

相比 `2.x` 版本（基于 WinForm 框架），`3.x` 使用 .NET（.NET Core）与 Vue.js 重写。其优点包括：
- 维护界面的开发成本降低
- 得益于 .NET 的设计，无需使用庞大的 Visual Studio 开发
- 得益于 .NET 的设计，可以以单可执行文件的形式发布
- 界面美观程度大幅提升

缺点包括：
- 抛弃了 Windows 7 及 32 位操作系统的支持
- 应用体积大幅增大（因几乎不再依赖于 .NET Framework）
- 前后端完全分离的设计可能对部分用户造成困扰

新功能：
- 前端：
    - 常用编译参数勾选
    - 新手模式卸载无关扩展
    - 新手模式安装中文扩展
    - 新手模式调试前检测不合法文件名
    - 不强制修改环境变量
- 后端：
    - 脚本化 ConsolePauser，界面更美观
    - 全 CLI 支持

问题修复：
- 不再覆盖 `keybindings.json`，覆盖前会尝试合并
- 不再移除 Path 中其它 MinGW

移除功能：
- 不再允许管理员权限下的全系统设置（即不会改变系统环境变量的值）

## [2.x](https://github.com/Guyutongxue/VSCodeConfigHelper/blob/v2.x/CHANGELOG.md)