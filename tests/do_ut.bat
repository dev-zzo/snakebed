@echo off

python ../tools/sbcompile.py unittest.py

python ../tools/sbcompile.py test_str.py
..\build\SbApp_d.exe test_str.sb

python ../tools/sbcompile.py test_socket.py
..\build\SbApp_d.exe test_socket.sb

python ../tools/sbcompile.py test_func.py
..\build\SbApp_d.exe test_func.sb
