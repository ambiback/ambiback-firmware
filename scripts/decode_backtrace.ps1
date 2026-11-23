# Decode ESP32 Guru Meditation backtrace using addr2line.
# Supports: Backtrace:0x4009165d:0x3ffb89100x4008ff4a:0x3ffb8950 0x4008dbe0:0x3ffb8990 0x4008db92:0xa5a5a5a5
# Usage:
#   .\decode_backtrace.ps1 -Elf "D:\path\to\AmbiBack.Controller.ino.elf" -CrashFile crashdump.txt
#   .\decode_backtrace.ps1 -Elf "D:\path\to\elf.elf" 0x4009165d 0x4008ff4a 0x4008dbe0
#   .\decode_backtrace.ps1 -BuildPath "D:\!cvsroot\_Misc\AmbiBack.USBFlasher\build\_build" (reads stdin)
#   $env:AMIBACK_BUILD_PATH="D:\path"; .\decode_backtrace.ps1 (reads stdin)

param(
    [string]$Elf = "",
    [string]$BuildPath = "",
    [string]$CrashFile = "",
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$RemainingArgs = @()
)

$CorruptedPatterns = @("0xa5a5a5a5", "0xcdcdcdcd", "0xabababab", "0xdeadbeef", "0xffffffff")

function Read-ToolchainBinConfig {
    if ($env:AMIBACK_TOOLCHAIN_BIN) { return $env:AMIBACK_TOOLCHAIN_BIN }
    $conf = Join-Path $PSScriptRoot "decode_backtrace.conf"
    if (Test-Path $conf) {
        $line = Get-Content $conf -ErrorAction SilentlyContinue | Where-Object { $_ -match '^AMIBACK_TOOLCHAIN_BIN=(.+)$' } | Select-Object -First 1
        if ($line) {
            $val = $line -replace '^AMIBACK_TOOLCHAIN_BIN=(.+)$', '$1' -replace '^["\s]+|["\s]+$'
            if ($val) { return $val }
        }
    }
    return $null
}

function Find-Addr2Line {
    $toolchainBin = Read-ToolchainBinConfig
    if ($toolchainBin) {
        $exe = Join-Path $toolchainBin "xtensa-esp32-elf-addr2line.exe"
        if (Test-Path $exe) { return $exe }
    }
    $addr2line = Get-Command xtensa-esp32-elf-addr2line -ErrorAction SilentlyContinue
    if ($addr2line) {
        return $addr2line.Source
    }
    # IDF_PATH
    if ($env:IDF_PATH) {
        $base = $env:IDF_PATH
        $candidates = @(
            (Join-Path $base "tools\xtensa-esp32-elf\*\xtensa-esp32-elf\bin\xtensa-esp32-elf-addr2line.exe"),
            (Join-Path $base ".espressif\tools\xtensa-esp32-elf\*\xtensa-esp32-elf\bin\xtensa-esp32-elf-addr2line.exe")
        )
        foreach ($pat in $candidates) {
            $found = Get-Item $pat -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($found) { return $found.FullName }
        }
    }
    # User .espressif
    $espressif = Join-Path $env:USERPROFILE ".espressif\tools\xtensa-esp32-elf"
    if (Test-Path $espressif) {
        $found = Get-Item (Join-Path $espressif "*\xtensa-esp32-elf\bin\xtensa-esp32-elf-addr2line.exe") -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) { return $found.FullName }
    }
    # Arduino packages
    $arduino = $env:LOCALAPPDATA
    if (-not $arduino) { $arduino = Join-Path $env:USERPROFILE "AppData\Local" }
    $packages = Join-Path $arduino "Arduino15\packages\esp32\hardware\esp32"
    if (Test-Path $packages) {
        $found = Get-Item (Join-Path $packages "*\tools\xtensa-esp32-elf-gcc\*\xtensa-esp32-elf\bin\xtensa-esp32-elf-addr2line.exe") -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) { return $found.FullName }
    }
    # Program Files Arduino
    $pf = "C:\Program Files (x86)\Arduino\hardware\esp32"
    if (Test-Path $pf) {
        $found = Get-Item (Join-Path $pf "*\tools\xtensa-esp32-elf-gcc\*\xtensa-esp32-elf\bin\xtensa-esp32-elf-addr2line.exe") -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) { return $found.FullName }
    }
    return $null
}

function Read-BuildPathConfig {
    $conf = Join-Path $PSScriptRoot "decode_backtrace.conf"
    if (Test-Path $conf) {
        $line = Get-Content $conf -ErrorAction SilentlyContinue | Where-Object { $_ -match '^AMIBACK_BUILD_PATH=(.+)$' } | Select-Object -First 1
        if ($line) {
            $val = $line -replace '^AMIBACK_BUILD_PATH=(.+)$', '$1' -replace '^["\s]+|["\s]+$' -replace '\\$'
            if ($val) { return $val }
        }
    }
    return $null
}

function Find-ElfInBuild {
    param([string]$BuildRoot)
    if (-not (Test-Path $BuildRoot)) {
        Write-Error "Build path not found: $BuildRoot"
        return $null
    }
    $elfs = [System.Collections.ArrayList]@()
    foreach ($d in Get-ChildItem $BuildRoot -Directory -ErrorAction SilentlyContinue) {
        $found = Get-Item (Join-Path $d.FullName "*.elf") -ErrorAction SilentlyContinue
        foreach ($f in $found) {
            if ($elfs -notcontains $f.FullName) { [void]$elfs.Add($f.FullName) }
        }
    }
    # Also check build root directly
    $found = Get-Item (Join-Path $BuildRoot "*.elf") -ErrorAction SilentlyContinue
    foreach ($f in $found) {
        if ($elfs -notcontains $f.FullName) { [void]$elfs.Add($f.FullName) }
    }
    if ($elfs.Count -eq 0) {
        Write-Error "No .elf files found under $BuildRoot"
        return $null
    }
    if ($elfs.Count -eq 1) {
        return $elfs[0]
    }
    Write-Host "Select ELF file:" -ForegroundColor Yellow
    for ($i = 0; $i -lt $elfs.Count; $i++) {
        $subdir = [System.IO.Path]::GetFileName([System.IO.Path]::GetDirectoryName($elfs[$i]))
        $fname = [System.IO.Path]::GetFileName($elfs[$i])
        Write-Host "  $($i+1)) [$subdir] $fname"
    }
    $choice = Read-Host "Choice [1-$($elfs.Count)]"
    $idx = 0
    if ($choice -match '^\d+$') {
        $idx = [int]$choice - 1
        if ($idx -lt 0 -or $idx -ge $elfs.Count) { $idx = 0 }
    }
    return $elfs[$idx]
}

function Extract-Addresses {
    param([string]$Text)
    $addrs = @()
    $matches = [regex]::Matches($Text, '0x[0-9a-fA-F]+:0x[0-9a-fA-F]+')
    foreach ($m in $matches) {
        $pair = $m.Value
        $parts = $pair -split ':'
        $addr = $parts[0]
        $sp = $parts[1]
        if ($CorruptedPatterns -notcontains $sp) {
            $addrs += $addr
        }
    }
    return $addrs
}

# Resolve ELF: from -Elf, first RemainingArg that looks like .elf, or BuildPath
if (-not $Elf -and $RemainingArgs.Count -gt 0) {
    $first = $RemainingArgs[0]
    if (($first -match '\.elf$') -or ((Test-Path $first) -and (Get-Item $first) -is [System.IO.FileInfo])) {
        $Elf = $first
        $RemainingArgs = $RemainingArgs[1..($RemainingArgs.Count-1)]
    }
}
if (-not $Elf -and -not $BuildPath) {
    $BuildPath = $env:AMIBACK_BUILD_PATH
    if (-not $BuildPath) {
        $BuildPath = Read-BuildPathConfig
    }
}

if ($BuildPath) {
    $Elf = Find-ElfInBuild -BuildRoot $BuildPath
}

if (-not $Elf -or -not (Test-Path $Elf)) {
    Write-Host @"
Usage: .\decode_backtrace.ps1 -Elf <elf_file> [-CrashFile <file>] [addresses...]
       .\decode_backtrace.ps1 -BuildPath <build_root> [-CrashFile <file>]
       `$env:AMIBACK_BUILD_PATH="D:\path"; .\decode_backtrace.ps1

ELF file required. Set AMIBACK_BUILD_PATH or create scripts/decode_backtrace.conf
with: AMIBACK_BUILD_PATH=D:\path\to\build\_build
"@ -ForegroundColor Red
    exit 1
}

$addr2line = Find-Addr2Line
if (-not $addr2line) {
    Write-Error "xtensa-esp32-elf-addr2line not found. Install ESP32 toolchain or add to PATH."
    exit 1
}

# Collect addresses (from RemainingArgs, -CrashFile, -CrashContent, or stdin/prompt)
$allAddrs = @()
$crashContent = ""
# RemainingArgs may contain: addresses (0x...), crash file path, or raw crash dump text
foreach ($arg in $RemainingArgs) {
    if ($arg -match '^0x[0-9a-fA-F]+$') {
        $allAddrs += $arg
    } elseif (-not $CrashFile -and (Test-Path $arg) -and $arg -notmatch '\.elf$') {
        $CrashFile = $arg
    } elseif ($arg -match 'Backtrace|0x[0-9a-fA-F]+:0x[0-9a-fA-F]+') {
        $crashContent += if ($crashContent) { " $arg" } else { $arg }
    }
}
if ($allAddrs.Count -eq 0 -and $CrashFile -and (Test-Path $CrashFile)) {
    $text = Get-Content $CrashFile -Raw
    $allAddrs = Extract-Addresses -Text $text
}
if ($allAddrs.Count -eq 0 -and $crashContent) {
    $allAddrs = Extract-Addresses -Text $crashContent
}
if ($allAddrs.Count -eq 0) {
    try {
        $stdin = [Console]::In.ReadToEnd()
        if ($stdin) { $allAddrs = Extract-Addresses -Text $stdin }
    } catch { }
    if ($allAddrs.Count -eq 0 -and $input) {
        $text = $input | Out-String
        if ($text) { $allAddrs = Extract-Addresses -Text $text }
    }
    # Interactive: prompt if still no input
    if ($allAddrs.Count -eq 0 -and [Environment]::UserInteractive) {
        Write-Host "Paste crash dump (or path to file), then Ctrl+Z + Enter:" -ForegroundColor Yellow
        try {
            $pasted = ([Console]::In.ReadToEnd()).Trim()
        } catch { $pasted = "" }
        if ($pasted -and (Test-Path $pasted) -and $pasted -notmatch '\.elf$') {
            $allAddrs = Extract-Addresses -Text (Get-Content $pasted -Raw)
        } elseif ($pasted) {
            $allAddrs = Extract-Addresses -Text $pasted
        }
    }
}

if ($allAddrs.Count -eq 0) {
    Write-Error "No backtrace addresses found. Paste crash dump, pass a file, or pass addresses."
    exit 1
}

Write-Host "Decoding $($allAddrs.Count) frames with $Elf" -ForegroundColor Cyan
& $addr2line -pfiaC -e $Elf $allAddrs
