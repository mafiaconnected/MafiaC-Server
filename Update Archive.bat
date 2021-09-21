del MafiaCServer.tar
set PATH=%PATH%;%CD%\Tools
set DIR=%CD%
cls
7z a -ttar "%DIR%\MafiaCServer.tar" -xr!*.bak "%DIR%\Data\*"
pause