using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net.Sockets;
using System.ComponentModel;

namespace freETarget.comms {
    class TCP : aCommModule {

        frmMainWindow mainWindow;

        public override event CommEventHandler CommDataReceivedEvent;

        public override event CommEventHandler CommDisconnectedEvent;

        private TcpClient tcpclnt;
        private NetworkStream stm;

        private System.Windows.Forms.Timer getShotTimer;
        private System.ComponentModel.BackgroundWorker getShotsBackgroundWorker;

        private string IP;
        private int port;

        public TCP(frmMainWindow mainW) {

            this.mainWindow = mainW;

            this.tcpclnt = new TcpClient();

            this.getShotTimer = new System.Windows.Forms.Timer();
            this.getShotsBackgroundWorker = new System.ComponentModel.BackgroundWorker();

            // getShotTimer
            // 
            this.getShotTimer.Tick += new System.EventHandler(this.getShotTimer_Tick);
            // 
            // getShotsBackgroundWorker
            // 
            this.getShotsBackgroundWorker.DoWork += new System.ComponentModel.DoWorkEventHandler(this.getShotsBackgroundWorker_DoWork);
            this.getShotsBackgroundWorker.RunWorkerCompleted += new System.ComponentModel.RunWorkerCompletedEventHandler(this.getShotsBackgroundWorker_RunWorkerCompleted);

        }


        public override void close() {
            try {
                stm.Close();
                stm = null;
                tcpclnt.Close();
            } catch (Exception ex) {
            }
            getShotTimer.Enabled = false;
        }

        public override void open(OpenParams value) {
            if (value is TcpOpenParams) {
                TcpOpenParams tcpP = (TcpOpenParams)value;
                this.IP = tcpP.IP;
                this.port = tcpP.port;

                this.tcpclnt = new TcpClient();
                tcpclnt.ReceiveTimeout = 1000;
                tcpclnt.SendTimeout = 1000;
                //tcpclnt.Connect(this.IP, this.port);
                var result = tcpclnt.BeginConnect(this.IP, this.port, null, null);
                var success = result.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(20)); //connect timeout set to 20 seconds - using MDNS is very slow

                if (!success) {
                    throw new SocketException(10060);
                }
                
                stm = tcpclnt.GetStream();
                getShotTimer.Enabled = true;
                mainWindow.log("TCP channel open...");
            } else {
                Console.WriteLine("Open params are not for TCP");
            }
        }

        public override void sendData(string text) {
            try {
                byte[] outBuffer = new byte[text.Length];
                stm.Write(Encoding.UTF8.GetBytes(text), 0, text.Length);
                stm.Flush();
            }catch(Exception ex) {
                mainWindow.log("Error sending data over TCP: " + ex.Message);
            }
        }

        protected override void RaiseDataReceivedEvent(string text) {
            // Raise the event in a thread-safe manner using the ?. operator.
            CommDataReceivedEvent?.Invoke(this, new CommEventArgs(text));
        }

        protected override void RaiseDisconnectedEvent(string text) {
            this.close();

            // Raise the event in a thread-safe manner using the ?. operator.
            CommDisconnectedEvent?.Invoke(this, new CommEventArgs(text));
        }

        public override string getCommInfo() {
            return "TCP = " + this.IP + ":" + this.port;
        }


        private void getShotTimer_Tick(object sender, EventArgs e) {

            if (tcpclnt.Client.Poll(0, SelectMode.SelectRead)) {
                byte[] buff = new byte[1];
                try {
                    if (tcpclnt.Client.Receive(buff, SocketFlags.Peek) == 0) {
                        // Client disconnected
                        mainWindow.log("TCP Disconnected!");
                        RaiseDisconnectedEvent("Disconnected!");
                    }
                } catch (SocketException ex) {
                    mainWindow.log("TCP Disconnected with exception! " + ex.Message);
                    RaiseDisconnectedEvent("Disconnected with exception!");
                }
            }

            if ((tcpclnt.Connected) && (!getShotsBackgroundWorker.IsBusy)) {
                getShotsBackgroundWorker.RunWorkerAsync();
            }
        }

        private void getShotsBackgroundWorker_DoWork(object sender, DoWorkEventArgs e) {
            if (stm == null) {
                return;
            }

            if (!stm.DataAvailable) {
                return;
            }

            byte[] myReadBuffer = new byte[1024];
            StringBuilder myCompleteMessage = new StringBuilder();
            int numberOfBytesRead = 0;

            do {
                numberOfBytesRead = stm.Read(myReadBuffer, 0, myReadBuffer.Length);
                myCompleteMessage.AppendFormat("{0}", Encoding.ASCII.GetString(myReadBuffer, 0, numberOfBytesRead));
                // Console.WriteLine("Read: " + myCompleteMessage.ToString());

            }
            while (stm.DataAvailable);


            string buf = myCompleteMessage.ToString();
            //Console.WriteLine("Received: " + buf);
            string indata = buf.Replace("\n\r", Environment.NewLine); ;

            RaiseDataReceivedEvent(indata);

        }

        private void getShotsBackgroundWorker_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e) {

        }
    }

    public class TcpOpenParams : OpenParams {
        public string IP;
        public int port;
    }
}
