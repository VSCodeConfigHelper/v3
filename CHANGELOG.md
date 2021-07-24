# 更新日志 Change Log

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