@echo off

REM Check if Python is installed
where /q python
set "pythonExecutable=python"
IF ERRORLEVEL 1 (
	where /q py
	IF ERRORLEVEL 1 (
		ECHO Python is missing.
		exit /b 1
	)
	set "pythonExecutable=py"
	exit /b 1
)

REM Count the number of arguments
set argC=0
for %%x in (%*) do Set /A argC+=1

REM Set projectDir to the passed argument
set "projectDir=%*"

REM Call Python with the appropriate parameters
IF "%projectDir%"=="" (
	call %pythonExecutable% Scripts/data/Setup.py
) ELSE (
	call %pythonExecutable% Scripts/data/Setup.py -p="%projectDir%"
)

REM Pause to view any messages
PAUSE