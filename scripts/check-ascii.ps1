﻿###
# Generate by VS Code Config Helper 3. Edit it only if you know what you are doing!

if ($args.Length -eq 0) {
    Write-Host "Usage: $PSCommandPath <Filename>"
    exit
}
Add-Type -AssemblyName PresentationCore,PresentationFramework
function isAscii($str) {
    return $str -match '^[\x20-\x7F]*$';
}
if (isAscii($args[0])) {
    exit
} else {
    $result = [System.Windows.MessageBox]::Show('您当前的文件名包含非 ASCII 字符（中文、特殊字符等），会导致调试器无法正常工作。是否继续调试？', '警告', [System.Windows.MessageBoxButton]::YesNo, [System.Windows.MessageBoxImage]::Warning);
    if ($result -eq [System.Windows.MessageBoxResult]::Yes) {
        exit
    } else {
        exit 1
    }
}