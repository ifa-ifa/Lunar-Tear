@echo off
set "INPUT_FOLDER=%~1"
set "OUTPUT_FOLDER=%INPUT_FOLDER%/extracted_scripts"

if "%INPUT_FOLDER%"=="" (
    echo The input folder is extracted_assets/snow/script/. Run with path to input folder as argument, or drag and drop the input folder onto this script
    pause
    exit /b
)

mkdir "%OUTPUT_FOLDER%" 2>nul

for %%F in ("%INPUT_FOLDER%\*") do (
        
    UnsealedVerses unpack "%%F" "%%F_extracted"
    
    for /f "delims=" %%K in ('dir /b /s /a-d "%%F_extracted\*" 2^>nul') do (
        
        UnsealedVerses unpack-kpk "%%K" "%%K_extracted" >nul 2>&1
        for /f "delims=" %%L in ('dir /b /s /a-d "%%K_extracted\*.lub" 2^>nul') do (
            copy /Y "%%L" "%OUTPUT_FOLDER%\" >nul
        )
    )
    
    rmdir /s /q "%%F_extracted" 2>nul
)

echo.
echo Done
pause