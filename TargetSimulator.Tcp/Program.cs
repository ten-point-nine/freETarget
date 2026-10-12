using System;
using System.Collections.Generic;
using System.Globalization;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace TargetSimulator {

    /**
     * pretends to be a freETarget with a network connection: the PC software connects to it over TCP and receives shots.
     * runs on every platform, no hardware and no virtual serial ports needed
     */
    static class Program {

        private static readonly object clientLock = new object();
        private static TcpClient client;
        private static int shotCount = 0;
        private static double spread = 12;      //standard deviation of the shots in mm
        private static readonly Random random = new Random();

        //what a real target keeps in its memory and reports when asked with ECHO. NAME has to be first
        private static readonly string[] settingNames = { "NAME_ID", "ANGLE", "CALIBREx10", "LED_BRIGHT", "PAPER_ECO", "PAPER_TIME", "POWER_SAVE", "SEND_MISS", "SENSOR", "SN",
            "STEP_COUNT", "STEP_TIME", "TARGET_TYPE", "TEST", "X_OFFSET", "Y_OFFSET", "Z_OFFSET", "NORTH_X", "NORTH_Y", "EAST_X", "EAST_Y", "SOUTH_X", "SOUTH_Y", "WEST_X", "WEST_Y" };
        private static readonly Dictionary<string, string> settings = new Dictionary<string, string> {
            { "NAME_ID", "0" }, { "ANGLE", "45" }, { "CALIBREx10", "45" }, { "LED_BRIGHT", "50" }, { "PAPER_ECO", "0" }, { "PAPER_TIME", "0" }, { "POWER_SAVE", "30" },
            { "SEND_MISS", "0" }, { "SENSOR", "230.0" }, { "SN", "1234" }, { "STEP_COUNT", "200" }, { "STEP_TIME", "30" }, { "TARGET_TYPE", "0" }, { "TEST", "0" },
            { "X_OFFSET", "0" }, { "Y_OFFSET", "0" }, { "Z_OFFSET", "13" }, { "NORTH_X", "0" }, { "NORTH_Y", "0" }, { "EAST_X", "0" }, { "EAST_Y", "0" },
            { "SOUTH_X", "0" }, { "SOUTH_Y", "0" }, { "WEST_X", "0" }, { "WEST_Y", "0" } };
        private static readonly string[] names = { "TARGET", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "DOC", "DOPEY", "HAPPY", "GRUMPY", "BASHFUL", "SNEEZEY", "SLEEPY",
            "RUDOLF", "DONNER", "BLITXEM", "DASHER", "PRANCER", "VIXEN", "COMET", "CUPID", "DUNDER", "ODIN", "WODEN", "THOR", "BALDAR" };

        static int Main(string[] args) {
            int port = 1090;
            double auto = 0;

            for (int i = 0; i < args.Length; i++) {
                try {
                    switch (args[i]) {
                        case "--port": port = int.Parse(args[++i], CultureInfo.InvariantCulture); break;
                        case "--spread": spread = double.Parse(args[++i], CultureInfo.InvariantCulture); break;
                        case "--auto": auto = double.Parse(args[++i], CultureInfo.InvariantCulture); break;
                        default: return usage();
                    }
                } catch (Exception) {
                    return usage();
                }
            }

            TcpListener listener = new TcpListener(IPAddress.Any, port);
            listener.Start();
            Console.WriteLine("freETarget simulator listening on port " + port + ".");
            Console.WriteLine("In the PC software choose connection TCP, IP address 127.0.0.1 and TCP port " + port + ", then press Connect.");
            Console.WriteLine();
            Console.WriteLine("Keys:  Enter or s = shot   m = miss   b = burst of 5 shots   a = automatic fire on/off   d = drop the connection   q = quit");
            Console.WriteLine();

            Thread accept = new Thread(() => acceptLoop(listener)) { IsBackground = true };
            accept.Start();

            Timer autoTimer = null;
            double autoInterval = auto > 0 ? auto : 3;
            if (auto > 0) {
                autoTimer = startAuto(autoInterval);
            }

            while (true) {
                string key = readKey();
                if (key == null || key == "q") {
                    break;
                }
                switch (key) {
                    case "":
                    case "s":
                        shot();
                        break;
                    case "m":
                        send("{\"shot\":" + (++shotCount) + ", \"miss\":1, \"x\":0, \"y\":0}");
                        break;
                    case "b":
                        for (int i = 0; i < 5; i++) {
                            shot();
                        }
                        break;
                    case "a":
                        if (autoTimer == null) {
                            autoTimer = startAuto(autoInterval);
                        } else {
                            autoTimer.Dispose();
                            autoTimer = null;
                            Console.WriteLine("automatic fire off");
                        }
                        break;
                    case "d":
                        lock (clientLock) {
                            if (client != null) {
                                client.Close();
                                client = null;
                                Console.WriteLine("connection dropped. the PC software should reconnect by itself");
                            }
                        }
                        break;
                }
            }

            listener.Stop();
            return 0;
        }

        private static int usage() {
            Console.WriteLine("usage: TargetSimulator.Tcp [--port 1090] [--spread 12] [--auto seconds]");
            Console.WriteLine("  --port    TCP port to listen on (default 1090, the default of the PC software)");
            Console.WriteLine("  --spread  how far the shots scatter around the center, in mm (default 12)");
            Console.WriteLine("  --auto    fire a shot every so many seconds without pressing keys");
            return 1;
        }

        private static Timer startAuto(double seconds) {
            Console.WriteLine("automatic fire: a shot every " + seconds.ToString(CultureInfo.InvariantCulture) + " seconds");
            int interval = (int)(seconds * 1000);
            return new Timer(_ => shot(), null, interval, interval);
        }

        //a key when run in a terminal, a line when the input is piped. null at the end of the input
        private static string readKey() {
            if (Console.IsInputRedirected) {
                string line = Console.ReadLine();
                if (line == null) {
                    Thread.Sleep(Timeout.Infinite); //no keyboard: keep running until killed
                }
                return line == null ? null : line.Trim().ToLowerInvariant();
            }
            ConsoleKeyInfo info = Console.ReadKey(true);
            if (info.Key == ConsoleKey.Enter) {
                return "";
            }
            return info.KeyChar.ToString().ToLowerInvariant();
        }

        private static void acceptLoop(TcpListener listener) {
            try {
                while (true) {
                    TcpClient accepted = listener.AcceptTcpClient();
                    lock (clientLock) {
                        client?.Close();
                        client = accepted;
                    }
                    Console.WriteLine("PC software connected from " + accepted.Client.RemoteEndPoint);

                    //the PC software waits for a first text from the target to consider the connection established
                    send("freETarget Simulator");

                    Thread reader = new Thread(() => readLoop(accepted)) { IsBackground = true };
                    reader.Start();
                }
            } catch (SocketException) {
                //listener stopped
            }
        }

        //shows what the PC software sends and answers the commands a target answers
        private static void readLoop(TcpClient from) {
            byte[] buffer = new byte[1024];
            string pending = "";
            try {
                NetworkStream stream = from.GetStream();
                int read;
                while ((read = stream.Read(buffer, 0, buffer.Length)) > 0) {
                    string text = Encoding.UTF8.GetString(buffer, 0, read);
                    Console.WriteLine("  <- " + text.Trim());

                    pending += text;
                    int open;
                    int close;
                    while ((open = pending.IndexOf('{')) >= 0 && (close = pending.IndexOf('}', open)) >= 0) {
                        handleCommand(pending.Substring(open + 1, close - open - 1));
                        pending = pending.Substring(close + 1);
                    }
                }
            } catch (Exception) {
            }
            lock (clientLock) {
                if (client == from) {
                    client = null;
                    Console.WriteLine("PC software disconnected");
                }
            }
        }

        //a command is a list of "KEY":value pairs. settings are stored, ECHO reports all settings
        private static void handleCommand(string command) {
            bool echo = false;
            foreach (string pair in command.Split(',')) {
                string[] parts = pair.Split(':');
                if (parts.Length != 2) {
                    continue;
                }
                string key = parts[0].Trim().Trim('"');
                string value = parts[1].Trim();
                if (key == "ECHO") {
                    echo = true;
                } else {
                    lock (settings) {
                        if (settings.ContainsKey(key)) {
                            settings[key] = value;
                        }
                    }
                }
            }

            if (echo) {
                StringBuilder answer = new StringBuilder();
                lock (settings) {
                    int id;
                    int.TryParse(settings["NAME_ID"], out id);
                    answer.Append("{\"NAME\":\"" + (id >= 0 && id < names.Length ? names[id] : "TARGET") + "\"");
                    foreach (string name in settingNames) {
                        answer.Append(", \"" + name + "\":" + settings[name]);
                    }
                    answer.Append(", \"VERSION\":\"Simulator\"}");
                }
                send(answer.ToString());
            }
        }

        //normally distributed around the center of the target
        private static double scatter() {
            double u1 = 1.0 - random.NextDouble();
            double u2 = random.NextDouble();
            return spread * Math.Sqrt(-2.0 * Math.Log(u1)) * Math.Sin(2.0 * Math.PI * u2);
        }

        private static void shot() {
            double x;
            double y;
            int count;
            lock (random) {
                x = scatter();
                y = scatter();
                count = ++shotCount;
            }
            double radius = Math.Sqrt(x * x + y * y);
            double angle = Math.Atan2(y, x) * 180 / Math.PI;
            send("{\"shot\":" + count + ", \"miss\":0, \"x\":" + x.ToString("F2", CultureInfo.InvariantCulture) + ", \"y\":" + y.ToString("F2", CultureInfo.InvariantCulture)
                + ", \"r\":" + radius.ToString("F2", CultureInfo.InvariantCulture) + ", \"a\":" + angle.ToString("F2", CultureInfo.InvariantCulture) + "}");
        }

        private static void send(string text) {
            lock (clientLock) {
                if (client == null) {
                    Console.WriteLine("  (nobody connected) " + text);
                    return;
                }
                try {
                    byte[] data = Encoding.ASCII.GetBytes(text + "\r\n");
                    client.GetStream().Write(data, 0, data.Length);
                    Console.WriteLine("  -> " + text);
                } catch (Exception ex) {
                    Console.WriteLine("  could not send: " + ex.Message);
                    client = null;
                }
            }
        }
    }
}
