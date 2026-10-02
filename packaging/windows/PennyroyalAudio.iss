; ============================================================
;  Instalador de Windows para Pennyroyal Audio (Inno Setup 6)
;
;  USO LOCAL:
;   1. Compila en CLion en modo RELEASE
;   2. Abri este archivo con Inno Setup y apreta "Compile" (Ctrl+F9)
;   3. El instalador queda en la carpeta  dist\  del proyecto
; ============================================================

#define MyAppName    "Pennyroyal Audio"
#define MyAppExe     "Pennyroyal Audio.exe"

#ifndef MyAppVersion
  #define MyAppVersion "1.1.0"
#endif

; Carpeta donde esta el .exe compilado (por defecto: build Release de CLion)
#ifndef BuildDir
  #define BuildDir "..\..\cmake-build-release\PennyroyalAudio_artefacts\Release"
#endif

[Setup]
AppId={{8F2C4A1E-6B3D-4E7A-9C5F-2D1B0A9E7C34}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher=Pennyroyal Studio
DefaultDirName={autopf}\Pennyroyal Audio
DefaultGroupName=Pennyroyal Audio
DisableProgramGroupPage=yes
OutputDir=..\..\dist
OutputBaseFilename=PennyRoyalAudioSetup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExe}

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#BuildDir}\{#MyAppExe}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExe}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExe}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent
