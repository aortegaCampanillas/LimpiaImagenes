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

[Icons]
Name: "{group}\Limpia Imagenes"; Filename: "{app}\LimpiaImagenes.exe"
Name: "{commondesktop}\Limpia Imagenes"; Filename: "{app}\LimpiaImagenes.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Crear icono en el escritorio"; GroupDescription: "Iconos:"; Flags: unchecked
