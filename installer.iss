[Setup]
AppName=Limpia Imagenes
AppVersion=1.0.0
DefaultDirName={pf}\LimpiaImagenes
DefaultGroupName=Limpia Imagenes
SetupIconFile=assets\logo.ico
UninstallDisplayIcon={app}\LimpiaImagenes.exe
OutputDir=dist-installer
OutputBaseFilename=LimpiaImagenes-Setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Files]
Source: "dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Tasks]
Name: "desktopicon"; Description: "Crear icono en el escritorio"; GroupDescription: "Iconos:"; Flags: unchecked

[Icons]
Name: "{group}\Limpia Imagenes"; Filename: "{app}\LimpiaImagenes.exe"
Name: "{userdesktop}\Limpia Imagenes"; Filename: "{app}\LimpiaImagenes.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\LimpiaImagenes.exe"; Description: "Ejecutar Limpia Imagenes"; Flags: nowait postinstall skipifsilent
