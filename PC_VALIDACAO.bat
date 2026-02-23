@echo off
setlocal
call "%~dp0PCValidator\PC_VALIDACAO.bat"
exit /b %ERRORLEVEL%

