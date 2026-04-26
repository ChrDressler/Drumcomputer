# Konfiguration
$outputFile = "Mein_Drumcomputer_Code.txt"
$sourceDir = ".\src"  # Hier schauen wir jetzt nur in den src-Ordner!

# Prüfen, ob src existiert
if (-not (Test-Path $sourceDir)) {
    Write-Host "Fehler: Ordner 'src' nicht gefunden. Bist du im richtigen Projektverzeichnis?" -ForegroundColor Red
    Read-Host
    exit
}

# Bestehende Ausgabedatei löschen
if (Test-Path $outputFile) { Remove-Item $outputFile }

# Alle .ino, .cpp und .h Dateien im src-Ordner finden
$files = Get-ChildItem -Path $sourceDir -Recurse -Include *.ino, *.cpp, *.h, *.hpp

foreach ($file in $files) {
    Add-Content -Path $outputFile -Value "`n// =========================================="
    Add-Content -Path $outputFile -Value "// Datei: $($file.Name)"
    Add-Content -Path $outputFile -Value "// ==========================================`n"
    
    Get-Content -Path $file.FullName | Add-Content -Path $outputFile
    Add-Content -Path $outputFile -Value "`n"
}

Write-Host "Fertig! Datei '$outputFile' wurde erstellt." -ForegroundColor Green
Read-Host