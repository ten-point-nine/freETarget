using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace freETarget.comms {
    public class TCP : aCommModule {

        IAppHost mainWindow;

        public override event CommEventHandler CommDataReceivedEvent;

        public override event CommEventHandler CommDisconnectedEvent;

        private TcpClient tcpclnt;
        private NetworkStream stm;

        private const int pollInterval = 100; //miliseconds
        private Timer getShotTimer;
        private int polling = 0; //1 while a poll is running, so that slow polls do not overlap

        private string IP;
        private int port;

        public TCP(IAppHost mainW) {
            this.mainWindow = mainW;
            this.tcpclnt = new TcpClient();

            //the timer runs on a thread pool thread. the events are raised from there, like the USB module does
            this.getShotTimer = new Timer(this.getShotTimer_Tick, null, Timeout.Infinite, Timeout.Infinite);
        }

        public override void close() {
            getShotTimer.Change(Timeout.Infinite, Timeout.Infinite);
            try {
                stm.Close();
                stm = null;
                tcpclnt.Close();
            } catch (Exception) {
            }
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
                tcpclnt.EndConnect(result);

                stm = tcpclnt.GetStream();
                getShotTimer.Change(pollInterval, pollInterval);
                mainWindow.log("TCP channel open...");
            } else {
                Console.WriteLine("Open params are not for TCP");
            }
        }

        public override void sendData(string text) {
            try {
                byte[] outBuffer = Encoding.UTF8.GetBytes(text);
                stm.Write(outBuffer, 0, outBuffer.Length);
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

        private void getShotTimer_Tick(object state) {
            if (Interlocked.Exchange(ref polling, 1) == 1) {
                return;
            }

            try {
                TcpClient client = tcpclnt;
                if (client.Client == null) {
                    return; //closed in the meantime
                }

                if (client.Client.Poll(0, SelectMode.SelectRead)) {
                    byte[] buff = new byte[1];
                    try {
                        if (client.Client.Receive(buff, SocketFlags.Peek) == 0) {
                            // Client disconnected
                            mainWindow.log("TCP Disconnected!");
                            RaiseDisconnectedEvent("Disconnected!");
                            return;
                        }
                    } catch (SocketException ex) {
                        mainWindow.log("TCP Disconnected with exception! " + ex.Message);
                        RaiseDisconnectedEvent("Disconnected with exception!");
                        return;
                    }
                }

                if (client.Connected) {
                    readShots();
                }
            } catch (ObjectDisposedException) {
                //closed while polling
            } catch (System.IO.IOException ex) {
                mainWindow.log("Error reading data over TCP: " + ex.Message);
            } finally {
                Interlocked.Exchange(ref polling, 0);
            }
        }

        private void readShots() {
            NetworkStream stream = stm;
            if (stream == null) {
                return;
            }

            if (!stream.DataAvailable) {
                return;
            }

            byte[] myReadBuffer = new byte[1024];
            StringBuilder myCompleteMessage = new StringBuilder();
            int numberOfBytesRead = 0;

            do {
                numberOfBytesRead = stream.Read(myReadBuffer, 0, myReadBuffer.Length);
                myCompleteMessage.AppendFormat("{0}", Encoding.ASCII.GetString(myReadBuffer, 0, numberOfBytesRead));
            }
            while (stream.DataAvailable);

            string buf = myCompleteMessage.ToString();

            string indata = buf.Replace("\n\r", Environment.NewLine);
            RaiseDataReceivedEvent(indata);
        }
    }

    public class TcpOpenParams : OpenParams {
        public string IP;
        public int port;
    }
}
