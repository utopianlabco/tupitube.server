; -------------------------------------------------------------------------
; TupiTube Server - InnoSetup Installer Script
; Project: https://tupitube.com
; License: GNU General Public License v2
;
; Steps to generate the installer:
;   1. powershell -ExecutionPolicy Bypass -File .\configure.ps1 -TupitubeDir C:\devel\sources\tupitube.desk
;   2. make -j4
;   3. powershell -ExecutionPolicy Bypass -File .\deploy.ps1 -TupitubeDir C:\devel\sources\tupitube.desk
;   4. "C:\Program Files (x86)\Inno Setup 6\iscc.exe" installer\tupitube_server.iss
; -------------------------------------------------------------------------

#define AppName      "TupiTube Server"
#define AppVersion   "0.1"
#define AppPublisher "Utopian Lab"
#define AppURL       "https://tupitube.com"
#define AppExeName   "tupitube.server.exe"

[Setup]
; NOTE: The AppId value uniquely identifies this application.
; Do not change it once the installer has been released to end users.
AppId={{B3F2A1D4-7C9E-4F0B-A8D5-2E6C1F3B7A90}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}

; Default installation directory
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}

; Output
OutputDir=output
OutputBaseFilename=tupitube-server-{#AppVersion}-setup
SetupIconFile=..\src\shell\data\icons\app_icon.ico
UninstallDisplayIcon={app}\{#AppExeName}

; Compression
Compression=lzma2/ultra64
SolidCompression=yes

; Privileges - prefer user-level install, allow admin promotion via dialog
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

; Architecture
ArchitecturesInstallIn64BitMode=x64

; Wizard appearance
WizardStyle=modern
WizardSizePercent=110

; Minimum OS: Windows 10
MinVersion=10.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

; -------------------------------------------------------------------------
; Optional Tasks (shown on the final wizard page)
; -------------------------------------------------------------------------
[Tasks]
Name: "desktopicon"; \
    Description: "{cm:CreateDesktopIcon}"; \
    GroupDescription: "{cm:AdditionalIcons}"; \
    Flags: unchecked

; -------------------------------------------------------------------------
; Files
; Main executable
; -------------------------------------------------------------------------
[Files]
Source: "..\bin\{#AppExeName}"; \
    DestDir: "{app}"; \
    Flags: ignoreversion

; All DLLs at the top level of bin/ (Qt, FFmpeg, QuaZip, TupiTube Desk, MinGW)
Source: "..\bin\*.dll"; \
    DestDir: "{app}"; \
    Flags: ignoreversion

; Qt platform plugin (required - without this the app will not start)
Source: "..\bin\platforms\*"; \
    DestDir: "{app}\platforms"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

; Qt SQLite driver plugin (critical for the database)
Source: "..\bin\sqldrivers\*"; \
    DestDir: "{app}\sqldrivers"; \
    Flags: ignoreversion recursesubdirs createallsubdirs

; TupiTube export plugins (e.g. tupiffmpegplugin.dll)
Source: "..\bin\plugins\*"; \
    DestDir: "{app}\plugins"; \
    Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; Qt image format plugins (optional but recommended)
Source: "..\bin\imageformats\*"; \
    DestDir: "{app}\imageformats"; \
    Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; Qt style plugins (optional)
Source: "..\bin\styles\*"; \
    DestDir: "{app}\styles"; \
    Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

; -------------------------------------------------------------------------
; Start Menu shortcuts and optional Desktop shortcut
; -------------------------------------------------------------------------
[Icons]
Name: "{group}\{#AppName}"; \
    Filename: "{app}\{#AppExeName}"

Name: "{group}\{cm:UninstallProgram,{#AppName}}"; \
    Filename: "{uninstallexe}"

Name: "{userdesktop}\{#AppName}"; \
    Filename: "{app}\{#AppExeName}"; \
    Tasks: desktopicon

; -------------------------------------------------------------------------
; Registry entries
; -------------------------------------------------------------------------
[Registry]
; Record the install directory so other tools can find the server
Root: HKCU; \
    Subkey: "Software\{#AppPublisher}\{#AppName}"; \
    ValueType: string; \
    ValueName: "InstallDir"; \
    ValueData: "{app}"; \
    Flags: uninsdeletekey

; -------------------------------------------------------------------------
; Post-install: offer to launch the application
; -------------------------------------------------------------------------
[Run]
Filename: "{app}\{#AppExeName}"; \
    Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; \
    Flags: nowait postinstall skipifsilent

; -------------------------------------------------------------------------
; Pascal script - pre-install checks
; -------------------------------------------------------------------------
[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;

  // Warn if a previous installation was found at a different location
  if RegValueExists(HKEY_CURRENT_USER,
      'Software\{#AppPublisher}\{#AppName}', 'InstallDir') then
  begin
    // Nothing to block - just continue
  end;
end;
