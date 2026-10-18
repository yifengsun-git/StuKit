#ifndef SourceDir
  #error SourceDir must point to the deployed StuKit directory
#endif

#ifndef OutputDir
  #define OutputDir "."
#endif

#ifndef AppVersion
  #define AppVersion "0.2.2"
#endif

[Setup]
AppId={{8C9756CE-37C7-4EC1-B27F-C96B31842E8C}
AppName=StuKit
AppVersion={#AppVersion}
AppPublisher=yifengsun-git
AppPublisherURL=https://github.com/yifengsun-git/StuKit
AppSupportURL=https://github.com/yifengsun-git/StuKit/issues
DefaultDirName={code:GetApplicationDirectory}
DefaultGroupName=StuKit
DisableDirPage=yes
DisableProgramGroupPage=yes
UsePreviousAppDir=no
LicenseFile={#SourceDir}\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename=StuKit-{#AppVersion}-windows-x64-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile={#SourcePath}\StuKit.ico
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\bin\StuKit.exe
CloseApplications=yes
RestartApplications=yes

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Dirs]
Name: "{code:GetDataDirectory}"; Flags: uninsneveruninstall

[INI]
Filename: "{app}\storage.ini"; Section: "Storage"; Key: "DataDirectory"; String: "..\data"

[Registry]
Root: HKCU; Subkey: "Software\StuKit\Installer"; ValueType: string; ValueName: "InstallRoot"; ValueData: "{code:GetInstallRoot}"

[Icons]
Name: "{autoprograms}\StuKit"; Filename: "{app}\bin\StuKit.exe"
Name: "{autodesktop}\StuKit"; Filename: "{app}\bin\StuKit.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加快捷方式："; Flags: unchecked

[Run]
Filename: "{app}\bin\StuKit.exe"; Description: "启动 StuKit"; Flags: nowait postinstall skipifsilent

[Code]
var
  InstallRootPage: TInputDirWizardPage;

function DefaultInstallRoot: String;
var
  StoredRoot: String;
begin
  if RegQueryStringValue(
       HKCU,
       'Software\StuKit\Installer',
       'InstallRoot',
       StoredRoot) and (Trim(StoredRoot) <> '') then
    Result := StoredRoot
  else
    Result := ExpandConstant('{localappdata}\Programs');
end;

procedure InitializeWizard;
begin
  InstallRootPage := CreateInputDirPage(
    wpSelectDir,
    '选择 StuKit 根目录',
    '程序与个人数据将分别存放',
    '请选择应用根目录。安装后程序位于“根目录\StuKit”，个人数据位于“根目录\data”。'#13#10#13#10 +
      '升级和卸载默认不会删除 data 目录。',
    False,
    '');
  InstallRootPage.Add('根目录：');
  InstallRootPage.Values[0] := DefaultInstallRoot;
end;

function GetInstallRoot(Param: String): String;
begin
  if InstallRootPage <> nil then
    Result := RemoveBackslashUnlessRoot(Trim(InstallRootPage.Values[0]))
  else
    Result := DefaultInstallRoot;
end;

function GetApplicationDirectory(Param: String): String;
begin
  Result := AddBackslash(GetInstallRoot('')) + 'StuKit';
end;

function GetDataDirectory(Param: String): String;
begin
  Result := AddBackslash(GetInstallRoot('')) + 'data';
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if (InstallRootPage <> nil) and (CurPageID = InstallRootPage.ID) then
  begin
    if Trim(InstallRootPage.Values[0]) = '' then
    begin
      MsgBox('请选择 StuKit 根目录。', mbError, MB_OK);
      Result := False;
    end
    else
      WizardForm.DirEdit.Text := GetApplicationDirectory('');
  end;
end;
