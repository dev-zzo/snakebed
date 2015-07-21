@echo off

python ../tools/sbcompile.py unittest.py

python ../tools/sbcompile.py test_objects.py
..\build\SbApp_d.exe test_objects.sb

python ../tools/sbcompile.py test_str.py
..\build\SbApp_d.exe test_str.sb

python ../tools/sbcompile.py test_socket.py
..\build\SbApp_d.exe test_socket.sb

python ../tools/sbcompile.py test_func.py
..\build\SbApp_d.exe test_func.sb

python ../tools/sbcompile.py test_exceptions.py
..\build\SbApp_d.exe test_exceptions.sb
