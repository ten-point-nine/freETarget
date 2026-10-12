using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.Versioning;
using System.Threading.Tasks;

namespace freETarget.Desktop {

    /** text to speech with what the operating system offers */
    static class PlatformSpeech {

        public static ISpeech create() {
            try {
                if (OperatingSystem.IsWindows()) {
                    return new WindowsSpeech();
                } else if (OperatingSystem.IsMacOS()) {
                    return new CommandSpeech("say");
                } else {
                    foreach (string command in new[] { "spd-say", "espeak-ng", "espeak" }) {
                        if (CommandSpeech.exists(command)) {
                            return new CommandSpeech(command);
                        }
                    }
                }
            } catch (Exception ex) {
                Console.WriteLine("Speech not available: " + ex.Message);
            }
            return Speech.Current; //silent
        }
    }

    [SupportedOSPlatform("windows")]
    class WindowsSpeech : ISpeech {

        private readonly System.Speech.Synthesis.SpeechSynthesizer synth;

        public WindowsSpeech() {
            synth = new System.Speech.Synthesis.SpeechSynthesizer();
            synth.SetOutputToDefaultAudioDevice();
        }

        public void speakAsync(string text) {
            synth.SpeakAsync(text);
        }
    }

    /** speaks by running a command line program. the texts are spoken one after the other */
    class CommandSpeech : ISpeech {

        private readonly string command;
        private readonly object queueLock = new object();
        private Task queue = Task.CompletedTask;

        public CommandSpeech(string command) {
            this.command = command;
        }

        public static bool exists(string command) {
            string path = Environment.GetEnvironmentVariable("PATH") ?? "";
            foreach (string dir in path.Split(Path.PathSeparator)) {
                if (dir != "" && File.Exists(Path.Combine(dir, command))) {
                    return true;
                }
            }
            return false;
        }

        public void speakAsync(string text) {
            lock (queueLock) {
                queue = queue.ContinueWith(_ => speak(text));
            }
        }

        private void speak(string text) {
            try {
                ProcessStartInfo info = new ProcessStartInfo(command) { UseShellExecute = false, CreateNoWindow = true };
                if (command == "spd-say") {
                    info.ArgumentList.Add("--wait");
                }
                info.ArgumentList.Add(text);
                using (Process process = Process.Start(info)) {
                    process.WaitForExit();
                }
            } catch (Exception ex) {
                Console.WriteLine("Could not speak: " + ex.Message);
            }
        }
    }
}
