@ECHO off
set "local_directory=%cd%"
set PATH=%local_directory%\..\..\bin;%local_directory%\..\..\opt\cross\bin;

make -j && bochscon.bat
