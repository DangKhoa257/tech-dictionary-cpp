param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$IconFile,
    [Parameter(Mandatory = $true)][string]$SourcePng,
    [string]$OutputDirectory = 'dist',
    [switch]$SkipWindowCapture
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.Windows.Forms
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public struct IconWindowRect {
    public int Left;
    public int Top;
    public int Right;
    public int Bottom;
}

public static class NativeIconWindow {
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindow(string className, string title);
    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr window, out IconWindowRect rect);
    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
}
'@

$exePath = (Resolve-Path -LiteralPath $Executable).Path
$icoPath = (Resolve-Path -LiteralPath $IconFile).Path
$pngPath = (Resolve-Path -LiteralPath $SourcePng).Path
$outputPath = [System.IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null

$bytes = [System.IO.File]::ReadAllBytes($icoPath)
if ($bytes.Length -lt 6 -or [BitConverter]::ToUInt16($bytes, 0) -ne 0 -or
    [BitConverter]::ToUInt16($bytes, 2) -ne 1) { throw 'Invalid ICO header' }
$count = [BitConverter]::ToUInt16($bytes, 4)
$sizes = @()
for ($index = 0; $index -lt $count; ++$index) {
    $entry = 6 + 16 * $index
    $size = if ($bytes[$entry] -eq 0) { 256 } else { [int]$bytes[$entry] }
    $height = if ($bytes[$entry + 1] -eq 0) { 256 } else { [int]$bytes[$entry + 1] }
    $length = [BitConverter]::ToUInt32($bytes, $entry + 8)
    $offset = [BitConverter]::ToUInt32($bytes, $entry + 12)
    if ($size -ne $height -or $offset + $length -gt $bytes.Length -or
        $bytes[$offset] -ne 137 -or $bytes[$offset + 1] -ne 80) {
        throw "Invalid ICO frame at index $index"
    }
    $sizes += $size
}
if (($sizes -join ',') -ne '16,24,32,48,64,128,256') {
    throw "Unexpected ICO resolutions: $($sizes -join ', ')"
}

$source = [System.Drawing.Image]::FromFile($pngPath)
try {
    if ($source.Width -lt 1024 -or $source.Height -lt 1024) {
        throw 'High-resolution PNG source is too small'
    }
} finally { $source.Dispose() }

$icon = [System.Drawing.Icon]::ExtractAssociatedIcon($exePath)
if ($null -eq $icon) { throw 'Windows could not extract an icon from the executable' }
try {
    $bitmap = $icon.ToBitmap()
    try {
        $iconPreview = Join-Path $outputPath 'tech-dictionary-exe-icon.png'
        $bitmap.Save($iconPreview, [System.Drawing.Imaging.ImageFormat]::Png)
        $bluePixels = 0
        $whitePixels = 0
        for ($y = 0; $y -lt $bitmap.Height; ++$y) {
            for ($x = 0; $x -lt $bitmap.Width; ++$x) {
                $pixel = $bitmap.GetPixel($x, $y)
                if ($pixel.A -gt 200 -and $pixel.B -gt ($pixel.R + 25)) { ++$bluePixels }
                if ($pixel.A -gt 200 -and $pixel.R -gt 220 -and
                    $pixel.G -gt 220 -and $pixel.B -gt 220) { ++$whitePixels }
            }
        }
        if ($bluePixels -lt 30 -or $whitePixels -lt 10) {
            throw 'Extracted executable icon lacks the expected blue book artwork'
        }
        Write-Output "Embedded icon: $($bitmap.Width)x$($bitmap.Height); blue=$bluePixels white=$whitePixels"
    } finally { $bitmap.Dispose() }
} finally { $icon.Dispose() }
Write-Output "ICO resolutions: $($sizes -join ', ')"

if ($SkipWindowCapture) { return }

$process = Start-Process -FilePath $exePath -PassThru
try {
    $window = [IntPtr]::Zero
    for ($attempt = 0; $attempt -lt 40; ++$attempt) {
        Start-Sleep -Milliseconds 250
        $process.Refresh()
        $window = $process.MainWindowHandle
        if ($window -ne [IntPtr]::Zero) { break }
    }
    if ($window -eq [IntPtr]::Zero) { throw 'Application did not open a native main window' }

    $wmGetIcon = [uint32]0x7F
    $windowIcon = [NativeIconWindow]::SendMessage($window, $wmGetIcon,
                                                   [IntPtr]::new(1), [IntPtr]::Zero)
    if ($windowIcon -eq [IntPtr]::Zero) {
        $windowIcon = [NativeIconWindow]::SendMessage($window, $wmGetIcon,
                                                       [IntPtr]::Zero, [IntPtr]::Zero)
    }
    if ($windowIcon -eq [IntPtr]::Zero) { throw 'Native window did not expose its application icon' }

    [void][NativeIconWindow]::SetForegroundWindow($window)
    Start-Sleep -Seconds 2
    $desktopBounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $desktop = [System.Drawing.Bitmap]::new($desktopBounds.Width, $desktopBounds.Height)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($desktop)
        try {
            $graphics.CopyFromScreen($desktopBounds.Left, $desktopBounds.Top, 0, 0,
                                      $desktopBounds.Size)
        } finally { $graphics.Dispose() }
        $desktop.Save((Join-Path $outputPath 'tech-dictionary-windows-desktop.png'),
                      [System.Drawing.Imaging.ImageFormat]::Png)

        $rect = [IconWindowRect]::new()
        if (-not [NativeIconWindow]::GetWindowRect($window, [ref]$rect)) {
            throw 'Could not measure application title bar'
        }
        $titleX = [Math]::Max(0, $rect.Left - $desktopBounds.Left)
        $titleY = [Math]::Max(0, $rect.Top - $desktopBounds.Top)
        $titleWidth = [Math]::Min($rect.Right - $rect.Left, $desktop.Width - $titleX)
        $titleHeight = [Math]::Min(95, $desktop.Height - $titleY)
        if ($titleWidth -lt 100 -or $titleHeight -lt 20) {
            throw 'Application title bar is outside the desktop capture'
        }
        $title = $desktop.Clone(
            [System.Drawing.Rectangle]::new($titleX, $titleY, $titleWidth, $titleHeight),
            $desktop.PixelFormat)
        try {
            $title.Save((Join-Path $outputPath 'tech-dictionary-titlebar.png'),
                        [System.Drawing.Imaging.ImageFormat]::Png)
        } finally { $title.Dispose() }

        $taskbar = [NativeIconWindow]::FindWindow('Shell_TrayWnd', $null)
        if ($taskbar -eq [IntPtr]::Zero) {
            throw 'Windows Explorer taskbar is unavailable on this CI desktop'
        }
        $taskbarRect = [IconWindowRect]::new()
        if (-not [NativeIconWindow]::GetWindowRect($taskbar, [ref]$taskbarRect)) {
            throw 'Could not measure Windows taskbar'
        }
        $taskX = [Math]::Max(0, $taskbarRect.Left - $desktopBounds.Left)
        $taskY = [Math]::Max(0, $taskbarRect.Top - $desktopBounds.Top)
        $taskWidth = [Math]::Min($taskbarRect.Right - $taskbarRect.Left,
                                  $desktop.Width - $taskX)
        $taskHeight = [Math]::Min($taskbarRect.Bottom - $taskbarRect.Top,
                                   $desktop.Height - $taskY)
        if ($taskWidth -lt 100 -or $taskHeight -lt 20) {
            throw 'Taskbar is outside the desktop capture'
        }
        $taskImage = $desktop.Clone(
            [System.Drawing.Rectangle]::new($taskX, $taskY, $taskWidth, $taskHeight),
            $desktop.PixelFormat)
        try {
            $taskImage.Save((Join-Path $outputPath 'tech-dictionary-taskbar.png'),
                            [System.Drawing.Imaging.ImageFormat]::Png)
        } finally { $taskImage.Dispose() }
        Write-Output "Captured Windows desktop, title bar, and taskbar; native icon handle $windowIcon"
    } finally { $desktop.Dispose() }
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
}
