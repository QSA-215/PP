@echo off
set programName=lab2.exe
set inputFileName=sample2.bmp
set outputFileName=out2.bmp
set blurPower=6

for /l %%i in (1, 1, 4) do (
    echo %%i core
    for /l %%x in (1, 1, 16) do (
        %programName% %inputFileName% %inputFileName% %%i %%x %blurPower%
        echo ;
    )
    echo.
)
pause
exit