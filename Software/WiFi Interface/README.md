# freetargetwifi2com


This is a simple go based app to bridge between the USB/COM port on the freETarget at https://free-e-target.com/ and the wifi enabled freETarget.

This version only forwards messages from the target to the pc software.

It relys on a COM paring driver such as http://com0com.sourceforge.net/

It assumes that the target is using the default ip address of 192.168.10.9 port 1090, and that the pc software is listening on COM8.

To use the bridge first configure the pc software to listen on COM8, click the connect button in the top left on the pc software, then start wifi2com.exe either by double clicking on it in explorer or run it from a command line.

Once the bridge has started it will display a message from the target with the version number, the PC Software will then show that it is connected after a few seconds.

Each shot data will be displayed in the bridge output as it forwards it to the PC Software.

If COM8 and COM9 are not available then you need to create a different pair with the COM pairing driver, for example COM1 and COM2, configure the PC software to listen on COM1, then you can use the following syntac to run the bridge to connect to COM2.

wifi2com.exe -port=COM2


