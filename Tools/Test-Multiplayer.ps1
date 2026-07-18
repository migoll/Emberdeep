param(
    [int]$HoldSeconds = 40,
    [string]$UnrealEditor = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $ProjectRoot "Emberdeep.uproject"
$ServerLog = "C:\tmp\EmberdeepPhase0BServer.log"
$ClientOneLog = "C:\tmp\EmberdeepPhase0BClientOne.log"
$ClientTwoLog = "C:\tmp\EmberdeepPhase0BClientTwo.log"
$Processes = @()

try {
    $ProjectArgument = '"' + $Project + '"'
    $Server = Start-Process -FilePath $UnrealEditor -ArgumentList "$ProjectArgument /Engine/Maps/Entry?listen -server -unattended -nosplash -nullrhi -port=7777 -SkipMainMenu -AutoRunSmoke -AutoLootSmoke -abslog=$ServerLog" -WindowStyle Hidden -PassThru
    $Processes += $Server
    Start-Sleep -Seconds 7

    $ClientOne = Start-Process -FilePath $UnrealEditor -ArgumentList "$ProjectArgument 127.0.0.1:7777 -game -unattended -nosplash -nullrhi -SkipMainMenu -abslog=$ClientOneLog" -WindowStyle Hidden -PassThru
    $Processes += $ClientOne
    Start-Sleep -Seconds 3

    $ClientTwo = Start-Process -FilePath $UnrealEditor -ArgumentList "$ProjectArgument 127.0.0.1:7777 -game -unattended -nosplash -nullrhi -SkipMainMenu -abslog=$ClientTwoLog" -WindowStyle Hidden -PassThru
    $Processes += $ClientTwo
    Start-Sleep -Seconds $HoldSeconds

    Stop-Process -Id $ClientOne.Id -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 4

    $ServerText = Get-Content -Raw $ServerLog
    $ClientText = Get-Content -Raw $ClientTwoLog
    $Checks = [ordered]@{
        TwoPlayersJoined = $ServerText -match "PlayerJoined.+Players=2"
        DungeonStageReplicated = $ClientText -match "EMBERDEEP_RUN Replicated Stage=THE ASHEN CRYPT.+Remaining=3"
        LootClaimAndEquip = $ServerText -match "EMBERDEEP_LOOT AutoLootEquipped.+Inventory=3.+Damage="
        PlayerRespawned = $ServerText -match "EMBERDEEP_NETWORK PlayerRespawned"
        RunInventorySurvivedRespawn = $ServerText -match "PlayerRespawned.+Inventory=3"
        DisconnectCleaned = $ServerText -match "PlayerLeft.+Players=1"
    }

    $Checks.GetEnumerator() | ForEach-Object {
        Write-Host ("{0}: {1}" -f $_.Key, $(if ($_.Value) { "PASS" } else { "FAIL" }))
    }
    if ($Checks.Values -contains $false) {
        throw "Emberdeep multiplayer smoke test failed. Inspect $ServerLog and $ClientTwoLog."
    }
}
finally {
    foreach ($Process in $Processes) {
        Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
    }
}
