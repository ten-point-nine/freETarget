# freETarget

**What is freETarget?**

freETarget is an 'electronic target' (or E-Target) for small calibre (air) rifles and pistols. It uses microphones arranged around the target card to detect the impact of the bullet on the paper and then calculates the point of impact using triangulation.

It does this using relatively inexpensive electronics, based around the Arduino open-source ecosystem. This means you can have your own E-Target for a lot less than the current commercial offerings. How much exactly depends mostly on you own skill and effort. You can source and build everything yourself including the electronics and have a system running for around $200. If you want to have the electronics ready made, this price will be around $300.

And yes, we realize this isn't exactly 'free', but it's still pretty cheap for such a system.

Check out the BOM for the electronics to get a feel for the costs of that part. The enclosure can be as expensive as you want, in it's cheapest form it can be made of sturdy cardboard!

The second part of the solution is actually free and consists of the excellent frETarget software. This software offers incredible functionality and is maintained free of charge! See something you want changed, it's open-source, feel free to improve (and please share back).

**How do I work with the firmware?**

The firmware is written for an Arduino and the Arduino IDE.  This makes it pretty accessible for anybody in the world.  Lots of aftermarket support

The software starts with freETarget.ino.  This is the startup and main control loop.  The other files, gpio.ino are helper files to interface to various hardware elements.  The heavy lifting is done buy the file compute_hit.ino.  


**I just want to build and upload the latest firmware**

You can always download the latest firmware from https://www.freetarget.com/downloads and upload the HEX file through the freETarget PC software. 

If you want to build from source, just open the freETarget.ino file located in the freETarget subfolder in the Arduino IDE. You won't see any code, but hitting Verify and upload will work just the same.

**I want to work on the PC source code**

Great! The PC software is wrttenin C# using Visual Studio.  You can download a free version from the Microsoft web site.  


**How do I work with the hardware?**

The hardware is maintained in the electronics subfolder of the GIT repository and is developed in KiCad. Download and install KiCad EDA to work on the schematics and PCB's.