; Moon Avoidance for Stellarium - Inno Setup Script
#define MyAppName "Moon Avoidance for Stellarium"
#define MyAppPublisher "Stellarium Community"
#define MyAppId "MoonAvoidance"

[Setup]
AppId={{D3E5E6B7-4C1F-4B8A-9D2E-1F2A3B4C5D6E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={userappdata}\Stellarium\modules\MoonAvoidance
DisableDirPage=yes
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputBaseFilename=MoonAvoidance-Setup-{#MyAppVersion}-win64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "MoonAvoidance.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "plugin.ini"; DestDir: "{app}"; Flags: ignoreversion

[Messages]
FinishedLabel=Moon Avoidance has been installed successfully. [newline][newline]Please restart Stellarium and enable the plugin in the Configuration window (F2) -> Plugins.




