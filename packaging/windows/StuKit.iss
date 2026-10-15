#ifndef SourceDir
  #error SourceDir must point to the deployed StuKit directory
#endif

#ifndef OutputDir
  #define OutputDir "."
#endif

#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif

[Setup]
AppId={{8C9756CE-37C7-4EC1-B27F-C96B31842E8C}
AppName=StuKit
AppVersion={#AppVersion}
AppPublisher=yifengsun-git
AppPublisherURL=https://github.com/yifengsun-git/StuKit
AppSupportURL=https://github.com/yifengsun-git/StuKit/issues
DefaultDirName={localappdata}\Programs\StuKit
DefaultGroupName=StuKit
DisableProgramGroupPage=yes
LicenseFile={#SourceDir}\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=StuKit-{#AppVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\bin\StuKit.exe

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\StuKit"; Filename: "{app}\bin\StuKit.exe"
Name: "{autodesktop}\StuKit"; Filename: "{app}\bin\StuKit.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加快捷方式："; Flags: unchecked

[Run]
Filename: "{app}\bin\StuKit.exe"; Description: "启动 StuKit"; Flags: nowait postinstall skipifsilent
