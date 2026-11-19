param([string]$Only = "")  # e.g. -Only moba runs just the runs whose tag starts with "moba" (turn 45)
# Turn 44 playtest runner: every hero in training (NP + default map), deathmatch and MOBA with the hero on autopilot.
# Each match: -ValdoriaPlaytest=<s> logs heroes, kills/deaths, stuck/fallen heroes, navmesh, screenshots, then exits.
# Run from PowerShell (not while the editor is open):  powershell -ExecutionPolicy Bypass -File Scripts\playtest_all.ps1
# Summary: Saved\Logs\Playtest\resumo.txt   Screenshots: Saved\Screenshots\Playtest
$ue = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$proj = '"C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject"'
$win = '-game -windowed -ResX=1600 -ResY=900 -WinX=-1760 -WinY=60 -log -ValdoriaNoIntro'
$out = "C:\Dev\EchoesOfValdoria\Saved\Logs\Playtest"
New-Item -ItemType Directory -Force $out | Out-Null
New-Item -ItemType Directory -Force "C:\Dev\EchoesOfValdoria\Saved\Screenshots\Playtest" | Out-Null
$heroes = "recruta","brasa","kael","iara","vex","lume"
$runs = @()
foreach ($h in $heroes) { $runs += @{ map = "/Game/Maps/NP_NovaPauliceia"; opt = ""; secs = 30; hero = $h; tag = "treino_np_$h" } }
$runs += @{ map = "/Game/ThirdPerson/Lvl_ThirdPerson"; opt = ""; secs = 30; hero = "recruta"; tag = "treino_padrao_recruta" }
foreach ($h in $heroes) { $runs += @{ map = "/Game/Maps/NP_NovaPauliceia"; opt = "?game=/Script/EchoesOfValdoria.ValdoriaDeathmatchMode?bots=1"; secs = 90; hero = $h; tag = "mm_np_$h" } }
$runs += @{ map = "/Game/ThirdPerson/Lvl_ThirdPerson"; opt = "?game=/Script/EchoesOfValdoria.ValdoriaDeathmatchMode?bots=1"; secs = 60; hero = "kael"; tag = "mm_padrao_kael" }
foreach ($h in $heroes) { $runs += @{ map = "/Game/Maps/NP_Moba"; opt = "?game=/Script/EchoesOfValdoria.ValdoriaMobaMode?bots=1"; secs = 120; hero = $h; tag = "moba_$h" } }

if ($Only) { $runs = @($runs | Where-Object { $_.tag -like "$Only*" }) }
$summary = Join-Path $out "resumo.txt"
"Playtest $(Get-Date)" | Out-File $summary -Encoding utf8
foreach ($r in $runs) {
  if (Get-Process | Where-Object { $_.ProcessName -match 'deadlock|project8' }) { "PARADO: Deadlock aberto" | Out-File $summary -Append -Encoding utf8; break }
  $log = Join-Path $out "$($r.tag).log"
  Remove-Item $log -ErrorAction SilentlyContinue
  $args = "$proj $($r.map)$($r.opt) $win -ValdoriaPlaytest=$($r.secs) -ValdoriaHero=$($r.hero) -ABSLOG=`"$log`""
  $t0 = Get-Date
  $p = Start-Process -FilePath $ue -ArgumentList $args -PassThru
  if (-not $p.WaitForExit(($r.secs + 120) * 1000)) { Stop-Process -Id $p.Id -Force; "[$($r.tag)] TEMPO ESGOTADO (fechado a forca)" | Out-File $summary -Append -Encoding utf8 }
  $lines = Get-Content $log -ErrorAction SilentlyContinue | Select-String "PLAYTEST (inicio|PROBLEMA|resumo|fim|navmesh|partida terminou)|Error:|Fatal|Ensure condition" | Where-Object { $_.Line -notmatch 'commandline|Command Line|UnifiedErrorTest|ngx|NGX|Streamline' } | ForEach-Object { $_.Line -replace '^.*(PLAYTEST|Error:|Fatal|Ensure)','$1' }
  "[$($r.tag)] $([int]((Get-Date) - $t0).TotalSeconds) s" | Out-File $summary -Append -Encoding utf8
  $lines | Select-Object -First 25 | Out-File $summary -Append -Encoding utf8
}
"FIM $(Get-Date)" | Out-File $summary -Append -Encoding utf8
