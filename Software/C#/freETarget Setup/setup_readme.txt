Android:
After the installation is complete, please go to the selected freETarget installation folder, and in the "drivers" subfolder, run the Arduino USB driver setup exe. 
Run the "dpinst-amd64.exe" file for 64 bit systems, or "dpinst-x86.exe" for 32 bit systems.

ESP32:
After the installation is complete, if Windows does not recognize the ESP32 board, go to drivers\ESP32 subfolder and run the ESP32 driver CDM212364_setup.exe
To upload the firmware, you need to install the uploaded tool ESPTOOL. There are multiple steps:
1)	Install python from https://www.python.org/downloads/
2)	When installing python, make sure to select the option to install the PIP tool 
3)	Open a command window and type: „pip install esptool” 
4)	Make sure the esptool.exe file exists in the python\scripts folder and that it was added to the PATH environment variable
