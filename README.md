# freETarget

freETarget

What is freETarget?

freETarget is an 'electronic target' (or E-Target) for small calibre (air) rifles and pistols. It uses microphones arranged around the target card to detect the impact of the bullet on the paper and then calculates the point of impact using black magic complicated math.

It does this using relatively inexpensive electronics, based around the Arduino open-source ecosystem. This means you can have your own E-Target for a lot less than the current commercial offerings. How much exactly depends mostly on you own skill and effort. You can source and build everything yourself including the electronics and have a system running for around $200. If you want to have the electronics ready made, this price will be around $300.

And yes, we realize this isn't exactly 'free', but it's still pretty cheap for such a system.

Check out the BOM for the electronics to get a feel for the costs of that part. The enclosure can be as expensive as you want, in it's cheapest form it can be made of sturdy cardboard!

The second part of the solution is actually free and consists of the excellent frETarget software. This software offers incredible functionality and is maintained free of charge! See something you want changed, it's open-source, feel free to improve (and please share back).

How do I work with the firmware?

In order to better sustain the code complexity the code was refactored to a more structured C++ style. This means you can't easily edit the code in the Arduino IDE anymore, but fear not, the Arduino IDE can still be used to build and upload!

I just want to build and upload the latest firmware

You can always download the latest firmware from https://www.freetarget.com/downloads and upload the HEX file through the freETarget PC software. If you want to build from source, just open the freETarget.ino file located in the freETarget subfolder in the Arduino IDE. You won't see any code, but hitting Verify and upload will work just the same.

I want to work on the source code

Great! Open-source software and hardware is made together! You can basically use any IDE to work on the source that supports the C++ language, but we recommend using Visual Studio Code. It's free, fast and supports a ton of extensions.

Just open the root of the GIT repository in Visual Studio Code (not just the frETarget subfolder). And you can start editing the files in the frETarget/src folder.

If you install the Arduino extension it will allow you to build right from the IDE. Install the plugin, open the root folder of the GIT repository and execute the Arduino: Initialize command from the command palette. It might ask some basic questions, and then it will initialize the Arduino extension for this repository for you specific PC. After that, you can use all the commands available in the plugin, such as Arduino: Verify to build and verify the code.

How do I work with the hardware?

The hardware is maintained in the electronics subfolder of the GIT repository and is developed in KiCad. Download and install KiCad EDA to work on the schematics and PCB's.