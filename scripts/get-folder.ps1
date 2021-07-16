###
# Generate by VS Code Config Helper 3. Edit it only if you know what you are doing!

Add-Type -AssemblyName System.Windows.Forms

# Show a dialog to choose folder
Function Open-FolderBrowser($initDir) {

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

Open-FolderBrowser $args[0] | Write-Output