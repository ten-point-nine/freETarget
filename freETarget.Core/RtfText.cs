using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace freETarget {

    /** the diary of a session is stored as RTF. this gets the plain text out of it */
    public static class RtfText {

        //groups that hold settings, not text
        private static readonly HashSet<string> skippedGroups = new HashSet<string>(StringComparer.Ordinal) {
            "fonttbl", "colortbl", "stylesheet", "info", "pict", "object", "header", "footer", "generator", "themedata", "listtable", "listoverridetable"
        };

        public static string toPlainText(string rtf) {
            if (string.IsNullOrEmpty(rtf)) {
                return "";
            }
            if (!rtf.TrimStart().StartsWith("{\\rtf", StringComparison.Ordinal)) {
                return rtf; //plain text already
            }

            Encoding ansi = Encoding.Latin1;
            StringBuilder text = new StringBuilder();
            Stack<bool> skipStack = new Stack<bool>();
            bool skip = false;
            int unicodeSkip = 0; //characters that follow a \u as replacement for old readers

            int i = 0;
            while (i < rtf.Length) {
                char c = rtf[i];
                if (c == '{') {
                    skipStack.Push(skip);
                    i++;
                } else if (c == '}') {
                    if (skipStack.Count > 0) {
                        skip = skipStack.Pop();
                    }
                    i++;
                } else if (c == '\\') {
                    i++;
                    if (i >= rtf.Length) {
                        break;
                    }
                    char next = rtf[i];
                    if (next == '\\' || next == '{' || next == '}') {
                        append(text, next, skip, ref unicodeSkip);
                        i++;
                    } else if (next == '\'') {
                        //character as two hex digits
                        if (i + 2 < rtf.Length) {
                            byte value;
                            if (byte.TryParse(rtf.Substring(i + 1, 2), NumberStyles.HexNumber, CultureInfo.InvariantCulture, out value)) {
                                append(text, ansi.GetString(new byte[] { value })[0], skip, ref unicodeSkip);
                            }
                        }
                        i += 3;
                    } else if (next == '*') {
                        skip = true; //optional group that only newer readers understand
                        i++;
                    } else if (next == '~') {
                        append(text, ' ', skip, ref unicodeSkip);
                        i++;
                    } else if (next == '\r' || next == '\n') {
                        append(text, '\n', skip, ref unicodeSkip);
                        i++;
                    } else if (char.IsLetter(next)) {
                        int start = i;
                        while (i < rtf.Length && char.IsLetter(rtf[i])) {
                            i++;
                        }
                        string word = rtf.Substring(start, i - start);

                        int numberStart = i;
                        if (i < rtf.Length && (rtf[i] == '-' || char.IsDigit(rtf[i]))) {
                            i++;
                            while (i < rtf.Length && char.IsDigit(rtf[i])) {
                                i++;
                            }
                        }
                        string number = rtf.Substring(numberStart, i - numberStart);
                        if (i < rtf.Length && rtf[i] == ' ') {
                            i++; //the space that ends a control word is not text
                        }

                        if (skippedGroups.Contains(word)) {
                            skip = true;
                        } else if (word == "par" || word == "line") {
                            append(text, '\n', skip, ref unicodeSkip);
                        } else if (word == "tab") {
                            append(text, '\t', skip, ref unicodeSkip);
                        } else if (word == "u" && number != "") {
                            int code;
                            if (int.TryParse(number, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out code)) {
                                if (code < 0) {
                                    code += 65536;
                                }
                                unicodeSkip = 0;
                                append(text, (char)code, skip, ref unicodeSkip);
                                unicodeSkip = 1;
                            }
                        }
                    } else {
                        i++; //other control symbols
                    }
                } else if (c == '\r' || c == '\n') {
                    i++; //line breaks in the file mean nothing
                } else {
                    append(text, c, skip, ref unicodeSkip);
                    i++;
                }
            }

            return text.ToString().Trim().Replace("\n", Environment.NewLine);
        }

        /** the text of a new diary page, with the usual headings to fill in */
        public static string diaryTemplate() {
            string[] headings = { "Gun:", "Gun adjustments:", "Pellets:", "Location:", "Weather:", "Goal:", "Influences:", "Exercises:", "Problems/Solutions:", "Positive/Learned:" };
            return string.Join(Environment.NewLine, headings) + Environment.NewLine;
        }

        /**
         * plain text as RTF, the format the diary is stored in. the original Windows application of the freETarget project can only read a diary that is RTF
         */
        public static string fromPlainText(string text) {
            StringBuilder rtf = new StringBuilder();
            rtf.Append("{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fnil Arial;}}\\uc1\\pard\\f0\\fs20 ");
            foreach (char c in (text ?? "").Replace("\r\n", "\n").Replace('\r', '\n')) {
                if (c == '\n') {
                    rtf.Append("\\par" + Environment.NewLine);
                } else if (c == '\t') {
                    rtf.Append("\\tab ");
                } else if (c == '\\' || c == '{' || c == '}') {
                    rtf.Append('\\').Append(c);
                } else if (c < 128) {
                    rtf.Append(c);
                } else {
                    //every other character as its number, with a question mark for readers that do not know unicode
                    rtf.Append("\\u").Append(((short)c).ToString(CultureInfo.InvariantCulture)).Append('?');
                }
            }
            rtf.Append('}');
            return rtf.ToString();
        }

        private static void append(StringBuilder text, char c, bool skip, ref int unicodeSkip) {
            if (unicodeSkip > 0) {
                unicodeSkip--;
                return;
            }
            if (!skip) {
                text.Append(c);
            }
        }
    }
}
