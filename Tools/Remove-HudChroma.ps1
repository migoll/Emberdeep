param(
    [Parameter(Mandatory = $true)]
    [string] $InputPath,

    [Parameter(Mandatory = $true)]
    [string] $OutputPath,

    [switch] $CropToVisible
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$sourcePath = (Resolve-Path -LiteralPath $InputPath).Path
$source = [System.Drawing.Bitmap]::new($sourcePath)
$output = [System.Drawing.Bitmap]::new(
    $source.Width,
    $source.Height,
    [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)

try {
    for ($y = 0; $y -lt $source.Height; ++$y) {
        for ($x = 0; $x -lt $source.Width; ++$x) {
            $pixel = $source.GetPixel($x, $y)
            $otherMaximum = [Math]::Max([int] $pixel.R, [int] $pixel.B)
            $greenDominance = [int] $pixel.G - $otherMaximum

            if ($greenDominance -ge 12) {
				$alpha = if ($pixel.G -ge 80) {
					[int] (255.0 * (1.0 - (($greenDominance - 12.0) / 110.0)))
				}
				else {
					255
				}
				$alpha = [Math]::Max(0, [Math]::Min(255, $alpha))
				$despilledGreen = [Math]::Min([int] $pixel.G, $otherMaximum + 4)
                $output.SetPixel(
                    $x,
                    $y,
                    [System.Drawing.Color]::FromArgb($alpha, $pixel.R, $despilledGreen, $pixel.B))
            }
            else {
                $output.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $pixel.R, $pixel.G, $pixel.B))
            }
        }
    }

    $bitmapToSave = $output
    $cropped = $null
    if ($CropToVisible) {
        $minimumX = $output.Width
        $minimumY = $output.Height
        $maximumX = -1
        $maximumY = -1
        for ($y = 0; $y -lt $output.Height; ++$y) {
            for ($x = 0; $x -lt $output.Width; ++$x) {
                if ($output.GetPixel($x, $y).A -ge 20) {
                    $minimumX = [Math]::Min($minimumX, $x)
                    $minimumY = [Math]::Min($minimumY, $y)
                    $maximumX = [Math]::Max($maximumX, $x)
                    $maximumY = [Math]::Max($maximumY, $y)
                }
            }
        }

        if ($maximumX -ge $minimumX -and $maximumY -ge $minimumY) {
            $padding = 4
            $minimumX = [Math]::Max(0, $minimumX - $padding)
            $minimumY = [Math]::Max(0, $minimumY - $padding)
            $maximumX = [Math]::Min($output.Width - 1, $maximumX + $padding)
            $maximumY = [Math]::Min($output.Height - 1, $maximumY + $padding)
            $cropRectangle = [System.Drawing.Rectangle]::new(
                $minimumX,
                $minimumY,
                $maximumX - $minimumX + 1,
                $maximumY - $minimumY + 1)
            $cropped = $output.Clone($cropRectangle, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $bitmapToSave = $cropped
        }
    }

    $destination = [System.IO.Path]::GetFullPath($OutputPath)
    $destinationDirectory = [System.IO.Path]::GetDirectoryName($destination)
    [System.IO.Directory]::CreateDirectory($destinationDirectory) | Out-Null
    $bitmapToSave.Save($destination, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally {
    if ($null -ne $cropped) {
        $cropped.Dispose()
    }
    $source.Dispose()
    $output.Dispose()
}
