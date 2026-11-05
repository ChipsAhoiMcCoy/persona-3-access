@echo off
rem Play P3R with the mod's dev server on, so Claude can hot reload changes while you play.
cd /d "%~dp0.."
echo Starting Persona 3 Reload...
python tools\p3.py play %*
if errorlevel 1 pause
