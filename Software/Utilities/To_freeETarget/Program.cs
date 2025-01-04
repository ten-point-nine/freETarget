using System;
using System.IO;

public class InsertTabs
{
    private const int tabSize = 4;
    private const string usageText = "Usage: Squish inputfile.html outputfile.c";
    public static int Main(string[] args)
    {
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
                using (var reader = new StreamReader(args[0]))
                {
                    // Redirect standard output from the console to the output file.
                    Console.SetOut(writer);
                    // Redirect standard input from the console to the input file.
                    Console.SetIn(reader);
                    string line;
                    line = "char " + args[1] + "[]=";
                    Console.WriteLine(line);
                    while ((line = Console.ReadLine()) != null)
                    {
                         line = line.Trim();
                         if (line.Length > 0)
                         { 
                            if ((line.Length < 2) || line.Substring(0, 2) != "//")
                            {
                                string newLine = '"' + line.Replace(('"'), '\"') + '"' + "\\";
                                Console.WriteLine(newLine);
                            }
                        }
                    }
                }
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
        Console.WriteLine($"Squish has completed the processing of {args[0]}.");
        return 0;
    }
}
