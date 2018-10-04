c:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\vsvars32.bat

cd ..\..\out
msbuild Nebula.sln /property:Configuration=RelWithDebInfo
cd ..\work\installer
"c:\Program Files (x86)\Caphyon\Advanced Installer 11.7.1\bin\x86\AdvancedInstaller.com" /edit n3contentsdk.aip /SetVersion -increment
"c:\Program Files (x86)\Caphyon\Advanced Installer 11.7.1\bin\x86\AdvancedInstaller.com" /build n3contentsdk.aip 