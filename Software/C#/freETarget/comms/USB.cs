using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

namespace freETarget.comms {
    class USB : aCommModule {

        SerialPort serialPort;

        public override event CommEventHandler CommDataReceivedEvent;

        public USB() {
            this.serialPort = new SerialPort();

             this.serialPort.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.serialPort_DataReceived);
        }

        public override void close() {
            this.serialPort.Close();
        }

        public override void open(OpenParams value) {
            if(value is UsbOpenParams) {
                UsbOpenParams usbP = (UsbOpenParams)value;
                serialPort.PortName = usbP.portName;
                serialPort.BaudRate = usbP.baudRate;
                serialPort.DataBits = usbP.dataBits;
                serialPort.DtrEnable = usbP.dtrEnable;

                serialPort.Open();
            } else {
                Console.WriteLine("Open params are not for USB");
            }
        }

        public override void sendData(string text) {
            this.serialPort.Write(text);
        }

        private void serialPort_DataReceived(object sender, System.IO.Ports.SerialDataReceivedEventArgs e) {
            SerialPort sp = (SerialPort)sender;
            string indata = sp.ReadExisting().Replace("\n\r", Environment.NewLine);
            RaiseDataReceivedEvent(indata);
        }
        protected override void RaiseDataReceivedEvent(string text) {
            // Raise the event in a thread-safe manner using the ?. operator.
            CommDataReceivedEvent?.Invoke(this, new CommEventArgs(text));
        }

        public override string getCommInfo() {
            return serialPort.PortName;
        }
    }

    public class UsbOpenParams: OpenParams {
        public string portName;
        public int baudRate;
        public int dataBits;
        public bool dtrEnable;
    }
}
