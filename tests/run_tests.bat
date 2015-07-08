@echo off

rem call compile_all.bat

echo Running tests.

for %%T in (*.sb) do (
    rem Kludge kludge...
    <nul (set/p z=Running %%T... )
    ..\build\sbapp_d.exe %%T
    echo()
    )