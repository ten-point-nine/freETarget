using PdfSharp.Fonts;
using SkiaSharp;
using System;
using System.Collections.Generic;

namespace freETarget {

    /**
     * gives PDFsharp the fonts for the reports. the platform independent build of PDFsharp does not look for fonts itself,
     * so the font files are located with SkiaSharp, which knows the fonts of every operating system
     */
    public class PdfFonts : IFontResolver {

        /** the font family that the reports ask for. Arial where it is installed, otherwise the closest sans serif font of the system */
        public const string Family = "Arial";

        private static readonly string[] candidates = { "Arial", "Liberation Sans", "Helvetica Neue", "DejaVu Sans", "Noto Sans", "FreeSans", "Verdana", "Tahoma" };

        private static readonly object initLock = new object();
        private static bool initialized = false;

        private readonly Dictionary<string, byte[]> fonts = new Dictionary<string, byte[]>();

        /** registers the fonts with PDFsharp. call before creating a font. can be called any number of times */
        public static void init() {
            lock (initLock) {
                if (!initialized) {
                    if (GlobalFontSettings.FontResolver == null) {
                        GlobalFontSettings.FontResolver = new PdfFonts();
                    }
                    initialized = true;
                }
            }
        }

        public FontResolverInfo ResolveTypeface(string familyName, bool bold, bool italic) {
            string face = "freETarget" + (bold ? "#b" : "#r") + (italic ? "i" : "");
            lock (fonts) {
                if (!fonts.ContainsKey(face)) {
                    byte[] data = find(bold, italic);
                    if (data == null) {
                        return null;
                    }
                    fonts[face] = data;
                }
            }
            return new FontResolverInfo(face);
        }

        public byte[] GetFont(string faceName) {
            lock (fonts) {
                return fonts[faceName];
            }
        }

        private static byte[] find(bool bold, bool italic) {
            SKFontStyle style = new SKFontStyle(bold ? SKFontStyleWeight.Bold : SKFontStyleWeight.Normal, SKFontStyleWidth.Normal, italic ? SKFontStyleSlant.Italic : SKFontStyleSlant.Upright);

            foreach (string family in candidates) {
                using (SKTypeface typeface = SKFontManager.Default.MatchFamily(family, style)) {
                    //a family that is not installed gives nothing, or a substitute with another name
                    if (typeface != null && string.Equals(typeface.FamilyName, family, StringComparison.OrdinalIgnoreCase)) {
                        byte[] data = read(typeface);
                        if (data != null) {
                            return data;
                        }
                    }
                }
            }

            //none of the known fonts: whatever the system uses by default
            using (SKTypeface fallback = SKFontManager.Default.MatchFamily(null, style) ?? SKTypeface.CreateDefault()) {
                return fallback == null ? null : read(fallback);
            }
        }

        //the bytes of the font file. null for font collections (several fonts in one file), which PDFsharp cannot embed
        private static byte[] read(SKTypeface typeface) {
            using (SKStreamAsset stream = typeface.OpenStream()) {
                if (stream == null || stream.Length < 4) {
                    return null;
                }
                byte[] data = new byte[stream.Length];
                if (stream.Read(data, data.Length) != data.Length) {
                    return null;
                }
                bool collection = data[0] == (byte)'t' && data[1] == (byte)'t' && data[2] == (byte)'c' && data[3] == (byte)'f';
                return collection ? null : data;
            }
        }
    }
}
