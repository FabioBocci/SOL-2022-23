@echo off
cd C:\msys64
call mingw64.exe bash -c "cd /c/Users/fabio/Desktop/SOL-2022-23 && make && echo '---------------------------' && echo '--------MAIN RUN-----------' && ./main; bash"