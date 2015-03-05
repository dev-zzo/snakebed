@echo off

call compile_all.bat

echo Running tests.

for %%T in (*.sb) do (
    rem Kludge kludge...
    <nul (set/p z=Running %%T... )
    ..\win32\debug\sbapp\sbapp.exe %%T
    echo()
    )