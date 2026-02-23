@echo off
setlocal
call "%~dp0PCValidator\PC_VALIDACAO_INTERATIVO.bat"
exit /b %ERRORLEVEL%

