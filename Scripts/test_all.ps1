# Turn 44: runs the main automated tests one after another on monitor 2, then the full playtest.
# Stops before each run if Deadlock is open. Summary: Saved\Logs\t44_summary.txt (+ Saved\Logs\Playtest\resumo.txt).
$ue = "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$proj = '"C:\Dev\EchoesOfValdoria\EchoesOfValdoria.uproject"'
$win = '-game -windowed -ResX=1600 -ResY=900 -WinX=-1760 -WinY=60 -log -ValdoriaNoIntro'
$sum = "C:\Dev\EchoesOfValdoria\Saved\Logs\t44_summary.txt"
"Testes $(Get-Date)" | Out-File $sum -Encoding utf8
$tests = @(
  @{ n = "admin";  a = "$proj $win -ValdoriaAdminTest";  f = "ADMINTEST \[(Brasa|Kael Rook|Iara|Vex|Lume|Recruta \(teste\)|Voar normal)|ADMINTEST fim" },
  @{ n = "voo";    a = "$proj $win -ValdoriaFlightTest"; f = "FLIGHTTEST passo|FLIGHTTEST fim" },
  @{ n = "lume";   a = "$proj $win -ValdoriaBeamTest";   f = "BEAMTEST|LUMETEST" },
  @{ n = "geral";  a = "$proj /Game/Maps/NP_NovaPauliceia $win -ValdoriaAutoTest"; f = "AUTOTEST" },
  @{ n = "teclas"; a = "$proj $win -ValdoriaInputTest";  f = "INPUTTEST" }
)
foreach ($t in $tests) {
  if (Get-Process | Where-Object { $_.ProcessName -match 'deadlock|project8' }) { "PARADO: Deadlock aberto" | Out-File $sum -Append -Encoding utf8; exit }
  $log = "C:\Dev\EchoesOfValdoria\Saved\Logs\t44_$($t.n).log"
  Remove-Item $log -ErrorAction SilentlyContinue
  $p = Start-Process -FilePath $ue -ArgumentList "$($t.a) -ABSLOG=`"$log`"" -PassThru
  if (-not $p.WaitForExit(420000)) { Stop-Process -Id $p.Id -Force; "[$($t.n)] TEMPO ESGOTADO" | Out-File $sum -Append -Encoding utf8 }
  "===== $($t.n)" | Out-File $sum -Append -Encoding utf8
  Get-Content $log -ErrorAction SilentlyContinue | Select-String $t.f | Where-Object { $_.Line -notmatch 'commandline|Command Line' } |
    ForEach-Object { ($_.Line -replace '^.*Display: ','' -replace ' pos=\([^)]*\)','' -replace ' \| bonecos=[^|]*','' -replace ' \| alvo=.*','') } |
    Select-Object -First 45 | Out-File $sum -Append -Encoding utf8
}
& "C:\Dev\EchoesOfValdoria\Scripts\playtest_all.ps1"
"FIM $(Get-Date)" | Out-File $sum -Append -Encoding utf8
