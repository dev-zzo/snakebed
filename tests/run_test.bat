@echo off

python ../tools/sbcompile.py %1.py

<nul (set/p z=Running %1.sb... )
..\win32\debug\sbapp\sbapp.exe %1.sb
