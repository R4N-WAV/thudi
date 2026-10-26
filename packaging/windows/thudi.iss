; Thudi for Windows: setup wizard (Inno Setup 6.7 or newer: dark style, custom colours, PNG artwork).
; Artwork: wizard.png, wizard-small.png, thudi.ico are cut from the rendered panel by `thudi_snapshot --wizard-art packaging/windows`.
; Built by GitHub Actions (.github/workflows/build.yml) after the plugin has passed its checks:
;   iscc /DAppVersion=1.0.0 /DPluginDir=<folder holding Thudi.vst3> /DOutDir=<output folder> packaging\windows\thudi.iss
; Installs Thudi.vst3 into the VST3 folder (C:\Program Files\Common Files\VST3 unless chosen otherwise),
; replaces an earlier version cleanly, keeps the user's presets and registers an uninstaller (Settings -> Apps).

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef PluginDir
  #define PluginDir "..\..\dist"
#endif
#ifndef OutDir
  #define OutDir "..\..\installer"
#endif

[Setup]
AppId={{5B1E7C3A-9D24-4F6E-A8C1-2E7F4B9D0C63}
AppName=Thudi
AppVersion={#AppVersion}
AppVerName=Thudi {#AppVersion}
AppPublisher=R4N
AppPublisherURL=https://github.com/R4N-WAV/thudi
AppSupportURL=https://github.com/R4N-WAV/thudi/releases/tag/latest
VersionInfoVersion={#AppVersion}
VersionInfoCompany=R4N
VersionInfoDescription=Thudi setup
DefaultDirName={commoncf64}\VST3
DirExistsWarning=no
AppendDefaultDirName=no
DisableProgramGroupPage=yes
DisableWelcomePage=no
UsePreviousAppDir=yes
UninstallDisplayName=Thudi (VST3) by R4N
UninstallDisplayIcon={uninstallexe}
; the uninstaller and the guide live in Program Files\R4N\Thudi, not in the VST3 folder
UninstallFilesDir={commonpf64}\R4N\Thudi
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
PrivilegesRequired=admin
WizardStyle=modern dark
WizardBackColor=#161618
WizardImageFile=wizard.png
WizardImageBackColor=#161618
WizardSmallImageFile=wizard-small.png
WizardSmallImageBackColor=#161618
SetupIconFile=thudi.ico
OutputDir={#OutDir}
OutputBaseFilename=Thudi-Setup-Windows
Compression=lzma2/max
SolidCompression=yes
CloseApplications=no

[Messages]
WelcomeLabel1=Welcome to Thudi
WelcomeLabel2=The drum synthesizer by R4N.%n%nThudi is a 16-pad drum synthesizer: no samples, every pad is its own synth voice with tone, noise, metal and body layers, an oversampled drive, crush above a crossover and a wide chorus with mono bass. It comes with the RHYNO kit, made to match the drums and 808s of RHYNO.%n%nThis installs Thudi {#AppVersion} (VST3, 64-bit) for FL Studio and every other VST3 host, with 3 kits and 48 pad presets to start from.%n%nPlease close your music app before you continue.
SelectDirLabel3=Thudi.vst3 goes into this VST3 folder. The standard folder is right for almost every host; only change it if your host scans a different one.
FinishedHeadingLabel=Thudi is ready
FinishedLabelNoIcons=Open your music app and rescan your plugins.%n%nFL Studio: Options > Manage plugins > Find installed plugins. Thudi is listed under R4N; load it as an instrument in the Channel rack.%n%nQuick start: click the pads to play the RHYNO kit, right-click a pad to edit, rename, copy or load a sound, and use BROWSE to switch kits.%n%nEnjoy.

[InstallDelete]
; replace an earlier version completely (old and new files are never mixed); the user's presets in Documents stay
Type: filesandordirs; Name: "{app}\Thudi.vst3"

[Files]
Source: "{#PluginDir}\Thudi.vst3\*"; DestDir: "{app}\Thudi.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\INSTALL.md"; DestDir: "{commonpf64}\R4N\Thudi"; Flags: ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{app}\Thudi.vst3"
Type: dirifempty; Name: "{commonpf64}\R4N\Thudi"
Type: dirifempty; Name: "{commonpf64}\R4N"

[Code]
// a host that has the plugin loaded keeps its file locked: say so instead of failing halfway
function PrepareToInstall (var NeedsRestart: Boolean): String;
var
  f: String;
begin
  Result := '';
  f := ExpandConstant ('{app}\Thudi.vst3\Contents\x86_64-win\Thudi.vst3');
  if FileExists (f) and not RenameFile (f, f + '.check') then
    Result := 'Thudi is in use (your music app has it loaded). Close the app and click Back, then Next to try again.'
  else if FileExists (f + '.check') then
    RenameFile (f + '.check', f);
end;
