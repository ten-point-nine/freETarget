using System;
using System.IO;
using System.Text;

public class InsertTabs
{
    private const int tabSize = 4;
    private const string usageText = "Usage: Hex inputfile.html outputfile.c";
    public static int Main(string[] args)
    {
        int temp;
        int i;

        if (args.Length < 2)
        {
            Console.WriteLine(usageText);
            return 1;
        }

        try
        {
            // Attempt to open output file.
            using (var writer = new StreamWriter(args[1]))
            {
                Console.SetOut(writer);
                byte[] rawFile = File.ReadAllBytes(args[0]);
                string outFile;
                outFile = "";
                i = 0;
                foreach (byte b in rawFile)
                {
                    temp = b;
                    outFile = outFile + "," + temp.ToString();
                    i++;
                    if ( (i % 64) == 0 )
                    {
                        outFile = outFile + Environment.NewLine;
                    }
                }

                Console.WriteLine(outFile);
            }
        }
        catch (IOException e)
        {
            TextWriter errorWriter = Console.Error;
            errorWriter.WriteLine(e.Message);
            errorWriter.WriteLine(usageText);
            return 1;
        }

        // Recover the standard output stream so that a
        // completion message can be displayed.
        var standardOutput = new StreamWriter(Console.OpenStandardOutput());
        standardOutput.AutoFlush = true;
        Console.SetOut(standardOutput);
        Console.WriteLine($"toNumbers has completed the processing of {args[0]}.");
        return 0;
    }
}
