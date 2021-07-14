###
# Generate by VS Code Config Helper 3. Edit it only if you know what you are doing!

# Show a dialog to choose folder
Function Get-Folder($initDir) {
    [void] [System.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms')
    $dialog = New-Object System.Windows.Forms.FolderBrowserDialog
    if ($initDir) {
        $dialog.SelectedPath = $initDir
    }
    $result = $dialog.ShowDialog()
    if ($result -eq [System.Windows.Forms.DialogResult]::OK) {
        return $dialog.SelectedPath
    } else {
        return $null
    }
}
Get-Folder | Write-Output