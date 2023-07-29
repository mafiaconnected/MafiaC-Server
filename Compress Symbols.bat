set PATH=%PATH%;%CD%\Tools
set DIR=%TMP%\Symbols.tmp
rmdir /S /Q TEMP
mkdir %DIR%
mkdir %DIR%\x64
mkdir %DIR%\x86
copy "Lib\x64\v142\Release\Server.pdb" %DIR%\x64\
copy "Lib\x64\v142\Debug\Server_d.pdb" %DIR%\x64\
copy "Lib\x86\v142\Release\Server.pdb" %DIR%\x86\
copy "Lib\x86\v142\Debug\Server_d.pdb" %DIR%\x86\
cls
7z a Symbols.7z -mx9 "%DIR%\*"
rmdir /S /Q %DIR%
pause