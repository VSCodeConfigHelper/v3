constexpr const char utf8Bom[]{0xEF, 0xBB, 0xBF};

constexpr const char pauseConsoleSrc[]{R"CppRawStr(
###
# Generate by VS Code Config Helper 3. Edit it only if you know what you are doing!

$Host.UI.RawUI.BackgroundColor = 'Black'
Clear-Host
if ($args.Length -eq 0) {
    Write-Host "Usage: $PSCommandPath <Executable> [<Arguments...>]"
    exit
}
$Host.UI.RawUI.WindowTitle = $args[0]
$startTime = $(Get-Date)
$proc = Start-Process -FilePath $args[0] -ArgumentList $args[1..($args.Count-1)] -NoNewWindow -PassThru
$handle = $proc.Handle # https://stackoverflow.com/a/23797762/1479211
$proc.WaitForExit()
[TimeSpan]$elapsedTime = $(Get-Date) - $startTime

Write-Host
Write-Host "----------------" -NoNewline
$exitCode = $proc.ExitCode
if ($exitCode -eq 0) {
    $exitColor = 'Green'
} else {
    $exitColor = 'Red'
}
# PowerLine Glyphs < and >
$gt = [char]0xe0b0
$lt = [char]0xe0b2
Write-Host "$lt" -ForegroundColor $exitColor -NoNewline
Write-Host (" 返回值 {0} " -f $exitCode) -BackgroundColor $exitColor -NoNewline
Write-Host (" 用时 {1:n4}s " -f $exitCode, $elapsedTime.TotalSeconds) -BackgroundColor 'Yellow' -ForegroundColor 'Black' -NoNewline
Write-Host "$gt" -ForegroundColor 'Yellow' -NoNewline
Write-Host "----------------"
Write-Host "进程已退出。按任意键关闭窗口..." -NoNewline
[void][System.Console]::ReadKey($true)
)CppRawStr"};

constexpr const char checkAsciiSrc[]{R"CppRawStr(
###
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
)CppRawStr"};