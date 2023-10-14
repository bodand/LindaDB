param ($file)

$hash = Get-FileHash $file
Write-Host -NoNewline $hash.Algorithm "($file)" = $hash.Hash
