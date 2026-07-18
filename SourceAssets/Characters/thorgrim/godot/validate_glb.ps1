[CmdletBinding()]
param(
    [string]$GlbPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Assert-GlbCondition {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw $Message
    }
}

function Get-OptionalPropertyValue {
    param(
        [object]$InputObject,
        [string]$Name,
        [object]$DefaultValue
    )

    $property = $InputObject.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $DefaultValue
    }
    return $property.Value
}

if ([string]::IsNullOrWhiteSpace($GlbPath)) {
    $GlbPath = Join-Path (Split-Path -Parent $PSScriptRoot) 'exports\thorgrim_v0.glb'
}

$resolvedGlbPath = (Resolve-Path -LiteralPath $GlbPath).Path
$bytes = [System.IO.File]::ReadAllBytes($resolvedGlbPath)
Assert-GlbCondition ($bytes.Length -ge 20) 'GLB is too small to contain a valid header and JSON chunk.'

$stream = [System.IO.MemoryStream]::new($bytes, $false)
$reader = [System.IO.BinaryReader]::new($stream)
try {
    $magic = $reader.ReadUInt32()
    $version = $reader.ReadUInt32()
    $declaredLength = $reader.ReadUInt32()
    Assert-GlbCondition ($magic -eq 0x46546C67) 'GLB magic does not equal glTF.'
    Assert-GlbCondition ($version -eq 2) "Expected GLB version 2, found $version."
    Assert-GlbCondition ($declaredLength -eq $bytes.Length) "GLB length header ($declaredLength) does not match file size ($($bytes.Length))."

    $chunks = @()
    while ($stream.Position -lt $stream.Length) {
        Assert-GlbCondition (($stream.Length - $stream.Position) -ge 8) 'Truncated GLB chunk header.'
        $chunkLength = $reader.ReadUInt32()
        $chunkType = $reader.ReadUInt32()
        Assert-GlbCondition (($chunkLength % 4) -eq 0) "GLB chunk length $chunkLength is not four-byte aligned."
        Assert-GlbCondition (($stream.Position + $chunkLength) -le $stream.Length) 'GLB chunk extends beyond the file.'
        $chunkData = $reader.ReadBytes($chunkLength)
        $chunks += [pscustomobject]@{
            Type = $chunkType
            Length = $chunkLength
            Data = $chunkData
        }
    }

    Assert-GlbCondition ($chunks.Count -ge 1) 'GLB contains no chunks.'
    Assert-GlbCondition ($chunks[0].Type -eq 0x4E4F534A) 'First GLB chunk is not JSON.'
    $jsonText = [System.Text.Encoding]::UTF8.GetString($chunks[0].Data).TrimEnd([char]0, [char]32)
    $gltf = $jsonText | ConvertFrom-Json
    Assert-GlbCondition ($gltf.asset.version -eq '2.0') "glTF asset version is not 2.0."

    $buffers = @($gltf.buffers)
    $bufferViews = @($gltf.bufferViews)
    $accessors = @($gltf.accessors)
    $materials = @($gltf.materials)
    $meshes = @($gltf.meshes)
    $nodes = @($gltf.nodes)
    $scenes = @($gltf.scenes)
    Assert-GlbCondition ($buffers.Count -eq 1) "Expected one embedded buffer, found $($buffers.Count)."
    Assert-GlbCondition ($meshes.Count -ge 1) 'glTF contains no meshes.'
    Assert-GlbCondition ($nodes.Count -ge 1) 'glTF contains no nodes.'
    Assert-GlbCondition ($scenes.Count -ge 1) 'glTF contains no scenes.'
    Assert-GlbCondition ($materials.Count -eq 9) "Expected nine Thorgrim palette materials, found $($materials.Count)."

    $expectedMaterialNames = @('Night', 'Steel', 'Ash', 'Bone', 'Hide', 'Fur', 'Skin', 'Wood', 'Cloth')
    $actualMaterialNames = @($materials | ForEach-Object name)
    Assert-GlbCondition (($actualMaterialNames -join '|') -eq ($expectedMaterialNames -join '|')) 'Thorgrim palette material names or ordering changed unexpectedly.'
    foreach ($material in $materials) {
        $pbr = $material.pbrMetallicRoughness
        Assert-GlbCondition ([double]$pbr.metallicFactor -eq 0.0) "Material $($material.name) is not non-metallic."
        Assert-GlbCondition ([double]$pbr.roughnessFactor -eq 1.0) "Material $($material.name) is not fully rough."
        Assert-GlbCondition (@($pbr.baseColorFactor).Count -eq 4) "Material $($material.name) has no complete flat base color."
        Assert-GlbCondition ($null -eq (Get-OptionalPropertyValue $pbr 'baseColorTexture' $null)) "Material $($material.name) unexpectedly references a texture."
    }

    $binaryChunks = @($chunks | Where-Object Type -eq 0x004E4942)
    Assert-GlbCondition ($binaryChunks.Count -eq 1) "Expected one BIN chunk, found $($binaryChunks.Count)."
    Assert-GlbCondition ($buffers[0].byteLength -le $binaryChunks[0].Length) 'Declared glTF buffer exceeds the BIN chunk.'

    for ($viewIndex = 0; $viewIndex -lt $bufferViews.Count; $viewIndex++) {
        $view = $bufferViews[$viewIndex]
        $bufferIndex = [int](Get-OptionalPropertyValue $view 'buffer' 0)
        $byteOffset = [long](Get-OptionalPropertyValue $view 'byteOffset' 0)
        $byteLength = [long]$view.byteLength
        Assert-GlbCondition ($bufferIndex -ge 0 -and $bufferIndex -lt $buffers.Count) "bufferView $viewIndex references an invalid buffer."
        Assert-GlbCondition ($byteOffset -ge 0 -and $byteLength -ge 0) "bufferView $viewIndex has a negative range."
        Assert-GlbCondition (($byteOffset + $byteLength) -le [long]$buffers[$bufferIndex].byteLength) "bufferView $viewIndex exceeds its buffer."
    }

    foreach ($mesh in $meshes) {
        foreach ($primitive in @($mesh.primitives)) {
            $positionAccessor = [int]$primitive.attributes.POSITION
            Assert-GlbCondition ($positionAccessor -ge 0 -and $positionAccessor -lt $accessors.Count) "Mesh primitive references an invalid POSITION accessor."
            $indexAccessorValue = Get-OptionalPropertyValue $primitive 'indices' $null
            if ($null -ne $indexAccessorValue) {
                $indexAccessor = [int]$indexAccessorValue
                Assert-GlbCondition ($indexAccessor -ge 0 -and $indexAccessor -lt $accessors.Count) 'Mesh primitive references an invalid index accessor.'
            }
            $materialIndexValue = Get-OptionalPropertyValue $primitive 'material' $null
            if ($null -ne $materialIndexValue) {
                $materialIndex = [int]$materialIndexValue
                Assert-GlbCondition ($materialIndex -ge 0 -and $materialIndex -lt $materials.Count) 'Mesh primitive references an invalid material.'
            }
        }
    }

    $materialNames = $actualMaterialNames -join ', '
    Write-Output "Validated GLB 2.0: $resolvedGlbPath"
    Write-Output "Size=$($bytes.Length) bytes; Scenes=$($scenes.Count); Nodes=$($nodes.Count); Meshes=$($meshes.Count); Materials=$($materials.Count); BufferViews=$($bufferViews.Count); Accessors=$($accessors.Count)"
    Write-Output "Materials: $materialNames"
}
finally {
    $reader.Dispose()
    $stream.Dispose()
}
