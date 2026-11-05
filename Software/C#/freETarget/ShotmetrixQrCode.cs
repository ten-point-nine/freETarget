// SPDX-License-Identifier: Apache-2.0
// Copyright 2026 Shotmetrix Kemmer GmbH
//
// .NET Framework 4.7.2 port of the Shotmetrix QR Code v1 reference encoder.
// NuGet dependency: BouncyCastle.Cryptography (2.x)
//
// Example:
// string url = ShotmetrixQrCode.QrCodeUrl(
//     publicKeyBase64, discipline: 1, decimalScoring: true,
//     sessionStart: 1791900000d,
//     shots: new[] {
//         new ShotmetrixShot { X = 0.42, Y = -1.1, Value = 10.6, Time = 1791900312.4 }
//     });

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using Org.BouncyCastle.Crypto;
using Org.BouncyCastle.Crypto.Digests;
using Org.BouncyCastle.Crypto.Engines;
using Org.BouncyCastle.Crypto.Generators;
using Org.BouncyCastle.Crypto.Modes;
using Org.BouncyCastle.Crypto.Parameters;
using Org.BouncyCastle.Security;

public sealed class ShotmetrixShot
{
    public double Value { get; set; }
    public double? X { get; set; }
    public double? Y { get; set; }
    // Unix seconds, or a DateTime/DateTimeOffset converted to Unix seconds.
    public object Time { get; set; }
    public bool Sighter { get; set; }
}

public static class ShotmetrixQrCode
{
    public const int Version = 1;
    public const int PlaintextBytes = 924;
    public const int MaxShots = 120;
    public const int ShotBytes = 7;
    public const string UrlPrefix = "https://shotmetrix.com/api/qrcode/";
    private static readonly byte[] Info = Encoding.ASCII.GetBytes("shotmetrix-qrcode-v1");
    private static readonly double[] Units = { 0.01, 0.1, 0.2 };
    private const int NoPosition = 0x800;
    private const int NoTime = 0xFFFFFF;
    private static readonly SecureRandom Random = new SecureRandom();

    private sealed class DisciplineInfo
    {
        public double[] Zones;
        public int? Tolerance;
        public DisciplineInfo(double[] zones, int? tolerance)
        {
            Zones = zones;
            Tolerance = tolerance;
        }
    }

    private static readonly Dictionary<int, DisciplineInfo> Disciplines =
        new Dictionary<int, DisciplineInfo>
        {
            { 1, new DisciplineInfo(new[] { 0.25 }, 3) }, //10m Air Rifle 
            { 2, new DisciplineInfo(new[] { 0.25 }, 3) }, //10m Air Rifle supported
            { 3, new DisciplineInfo(new[] { 0.8 }, 3) },  //10m Air Pistol
            { 4, new DisciplineInfo(new[] { 0.8 }, 3) },  //10m Air Pistol supported
            { 5, new DisciplineInfo(new[] { 0.8 }, 3) },  //50m Rifle Prone
            { 6, new DisciplineInfo(new[] { 0.8 }, 3) },  //50m Rifle 3 Positions
            { 7, new DisciplineInfo(new[] { 0.8 }, 3) },  //50m Rifle Supported
            { 8, new DisciplineInfo(new[] { 2.5, 4.0 }, 10) }, //25m Pistol
            { 9, new DisciplineInfo(null, null) },  //25m Pistol Rapid Fire
            { 10, new DisciplineInfo(new[] { 2.5 }, 10) },  //25m Pistol Standard
            { 11, new DisciplineInfo(new[] { 2.5 }, 10) },  //50m Pistol
            { 12, new DisciplineInfo(new[] { 2.5 }, 10) }  //25m Pistol Supported
        };

    private static void Fail(string message) { 
        throw new ArgumentException(message); 
    }

    // Rounds halves away from zero, matching the Python reference implementation.
    public static int RoundHalfAway(double value)
    {
        if (double.IsNaN(value) || double.IsInfinity(value))
            throw new ArgumentOutOfRangeException("value");
        return checked((int)(Math.Sign(value) * Math.Floor(Math.Abs(value) + 0.5)));
    }

    private static bool IsFinite(double value)
    {
        return !double.IsNaN(value) && !double.IsInfinity(value);
    }

    private static double? ToTimestamp(object value)
    {
        if (value == null) return null;
        if (value is DateTimeOffset)
            return ((DateTimeOffset)value).ToUnixTimeMilliseconds() / 1000.0;
        if (value is DateTime)
        {
            DateTime dt = (DateTime)value;
            if (dt.Kind == DateTimeKind.Unspecified)
                dt = DateTime.SpecifyKind(dt, DateTimeKind.Local);
            return new DateTimeOffset(dt).ToUnixTimeMilliseconds() / 1000.0;
        }
        if (value is byte || value is sbyte || value is short || value is ushort ||
            value is int || value is uint || value is long || value is ulong ||
            value is float || value is double || value is decimal)
        {
            double d = Convert.ToDouble(value, CultureInfo.InvariantCulture);
            return IsFinite(d) ? (double?)d : null;
        }
        return null;
    }

    private static byte[] UInt16BE(int value)
    {
        return new[] { (byte)(value >> 8), (byte)value };
    }

    private static byte[] UInt32BE(uint value)
    {
        return new[] { (byte)(value >> 24), (byte)(value >> 16), (byte)(value >> 8), (byte)value };
    }

    private static void WriteUInt32BE(byte[] target, int offset, uint value)
    {
        target[offset] = (byte)(value >> 24);
        target[offset + 1] = (byte)(value >> 16);
        target[offset + 2] = (byte)(value >> 8);
        target[offset + 3] = (byte)value;
    }

    private static int ValueFromPosition(double x, double y, double zone)
    {
        double radius = Math.Sqrt(x * x + y * y);
        double roundedRadius = Math.Floor(Math.Floor(radius / zone * 1e6 + 0.5) / 1e6);
        int tenths = 110 - (int)Math.Ceiling(roundedRadius);
        return tenths > 109 ? 109 : (tenths < 10 ? 0 : tenths);
    }

    private static byte[] RandomBytes(int length)
    {
        byte[] result = new byte[length];
        Random.NextBytes(result);
        return result;
    }

    /// <summary>
    /// Builds and validates the 924-byte plaintext payload.
    /// id must be null or exactly 16 bytes. fill, when supplied, provides padding bytes.
    /// nowUnixSeconds is mainly useful for deterministic tests; null uses the current time.
    /// </summary>
    public static byte[] Plaintext(
        int discipline,
        bool decimalScoring,
        object sessionStart,
        IList<ShotmetrixShot> shots,
        int app = 0,
        byte[] appSecret = null,
        byte[] id = null,
        byte[] fill = null,
        double? nowUnixSeconds = null)
    {
        if (shots == null || shots.Count < 1 || shots.Count > MaxShots)
            Fail("shots: 1 to 120 shots");
        DisciplineInfo disciplineInfo;
        if (!Disciplines.TryGetValue(discipline, out disciplineInfo))
            Fail("discipline: unknown number");

        double now = nowUnixSeconds.HasValue && IsFinite(nowUnixSeconds.Value)
            ? nowUnixSeconds.Value
            : (DateTimeOffset.UtcNow.ToUnixTimeMilliseconds() / 1000.0);
        double? startTimestamp = ToTimestamp(sessionStart);
        long start;
        if (!startTimestamp.HasValue || startTimestamp.Value < long.MinValue ||
            startTimestamp.Value > long.MaxValue)
            start = long.MinValue;
        else
            start = (long)Math.Floor(startTimestamp.Value);

        if (start < 946684800 || start > now + 86400)
            Fail("time: session start must be after 2000-01-01 and not in the future");
        if (app < 0 || app > 65535)
            Fail("app: 0 to 65535");
        if (id != null && id.Length != 16)
            Fail("id: 16 bytes");

        int count = shots.Count;
        int[] values = new int[count];
        int[] times = new int[count];
        double maxCoordinate = 0.0;
        int lastTime = -1;
        int scored = 0;

        for (int i = 0; i < count; i++)
        {
            ShotmetrixShot shot = shots[i];
            string prefix = "shot " + (i + 1).ToString(CultureInfo.InvariantCulture) + ": ";
            if (shot == null || !IsFinite(shot.Value))
                Fail(prefix + "value must be a number");
            double v = shot.Value;
            if (Math.Abs(v) > 1000)
                Fail(prefix + "value out of range");

            int value = decimalScoring
                ? RoundHalfAway(v * 10.0)
                : checked((int)Math.Floor(v) * 10);
            if (decimalScoring ? !(value == 0 || (value >= 10 && value <= 109))
                               : !(value >= 0 && value <= 100))
                Fail(prefix + "value out of range");

            if (shot.X.HasValue != shot.Y.HasValue)
                Fail(prefix + "give x and y together");
            if (shot.X.HasValue)
            {
                if (!IsFinite(shot.X.Value) || !IsFinite(shot.Y.Value))
                    Fail(prefix + "position must be a number");
                if (Math.Abs(shot.X.Value) > 409.4 || Math.Abs(shot.Y.Value) > 409.4)
                    Fail(prefix + "position outside ±409.4 mm");
                maxCoordinate = Math.Max(maxCoordinate,
                    Math.Max(Math.Abs(shot.X.Value), Math.Abs(shot.Y.Value)));
            }

            int shotTime = NoTime;
            if (shot.Time != null)
            {
                double? shotTimestamp = ToTimestamp(shot.Time);
                if (!shotTimestamp.HasValue)
                    Fail(prefix + "time must be a number or a date");
                double rounded = (shotTimestamp.Value - start) * 100.0;
                if (!IsFinite(rounded) || rounded < int.MinValue || rounded > int.MaxValue)
                    Fail(prefix + "time before the session start or more than 46 h after it");
                shotTime = RoundHalfAway(rounded);
                if (shotTime < 0 || shotTime >= NoTime)
                    Fail(prefix + "time before the session start or more than 46 h after it");
                if (shotTime < lastTime)
                    Fail(prefix + "time earlier than the shot before");
                if (start + shotTime / 100.0 > now + 86400)
                    Fail(prefix + "time in the future");
                lastTime = shotTime;
            }
            if (!shot.Sighter) scored++;
            values[i] = value;
            times[i] = shotTime;
        }
        if (scored == 0)
            Fail("no scored shot");

        int unitIndex = -1;
        for (int i = 0; i < Units.Length; i++)
        {
            if (maxCoordinate / Units[i] <= 2047)
            {
                unitIndex = i;
                break;
            }
        }
        if (unitIndex < 0)
            throw new ArgumentException("position outside representable range");
        double unit = Units[unitIndex];

        if (disciplineInfo.Tolerance.HasValue)
        {
            int limit = decimalScoring ? disciplineInfo.Tolerance.Value
                                       : Math.Max(10, disciplineInfo.Tolerance.Value);
            for (int i = 0; i < count; i++)
            {
                ShotmetrixShot shot = shots[i];
                if (shot.Sighter || !shot.X.HasValue) continue;
                double xq = RoundHalfAway(shot.X.Value / unit) * unit;
                double yq = RoundHalfAway(shot.Y.Value / unit) * unit;
                bool matches = false;
                foreach (double zone in disciplineInfo.Zones)
                {
                    int expected = ValueFromPosition(xq, yq, zone);
                    if (!decimalScoring) expected = expected / 10 * 10;
                    if (Math.Abs(expected - values[i]) <= limit)
                    {
                        matches = true;
                        break;
                    }
                }
                if (!matches)
                    Fail("shot " + (i + 1).ToString(CultureInfo.InvariantCulture) +
                         ": value does not match the position");
            }
        }

        int total = 0;
        List<byte> body = new List<byte>(ShotBytes * MaxShots);
        for (int i = 0; i < count; i++)
        {
            ShotmetrixShot shot = shots[i];
            if (!shot.Sighter) total += values[i];

            int x, y;
            if (!shot.X.HasValue)
            {
                x = NoPosition;
                y = NoPosition;
            }
            else
            {
                x = RoundHalfAway(shot.X.Value / unit) & 0xFFF;
                y = RoundHalfAway(shot.Y.Value / unit) & 0xFFF;
            }

            uint packed = ((uint)x << 20) | ((uint)y << 8) |
                          ((uint)values[i] << 1) | (shot.Sighter ? 1u : 0u);
            byte[] packedBytes = UInt32BE(packed);
            body.AddRange(packedBytes);
            byte[] timeBytes = UInt32BE((uint)times[i]);
            body.Add(timeBytes[1]);
            body.Add(timeBytes[2]);
            body.Add(timeBytes[3]);
        }

        int rest = ShotBytes * (MaxShots - count);
        if (fill != null)
            body.AddRange(fill.Take(rest));
        else
            body.AddRange(RandomBytes(rest));
        // If fill is shorter than the required padding, the reference implementation also
        // produces a short payload and fails its final length assertion. Match that behavior.
        byte[] header = new byte[36];
        byte[] actualId = id ?? RandomBytes(16);
        Buffer.BlockCopy(actualId, 0, header, 0, 16);
        Buffer.BlockCopy(UInt16BE(app), 0, header, 16, 2);
        // Bytes 18..25 are the 8-byte app_mark, initially zero.
        Buffer.BlockCopy(UInt32BE((uint)start), 0, header, 26, 4);
        header[30] = (byte)discipline;
        header[31] = (byte)((decimalScoring ? 1 : 0) | (unitIndex << 1));
        header[32] = (byte)count;
        Buffer.BlockCopy(UInt16BE(total), 0, header, 33, 2);
        header[35] = 0;

        byte[] pt = new byte[header.Length + body.Count + 48];
        Buffer.BlockCopy(header, 0, pt, 0, header.Length);
        body.CopyTo(pt, header.Length);
        // Last 48 bytes are reserved and zero-initialized.
        if (app != 0 && appSecret != null && appSecret.Length > 0)
        {
            byte[] mac;
            using (HMACSHA256 hmac = new HMACSHA256(appSecret))
                mac = hmac.ComputeHash(pt);
            Buffer.BlockCopy(mac, 0, pt, 18, 8);
        }
        if (pt.Length != PlaintextBytes)
            throw new InvalidOperationException("Invalid plaintext length: " + pt.Length);
        return pt;
    }

    /// <summary>Encrypts plaintext using X25519 + HKDF-SHA256 + AES-256-GCM.</summary>
    public static byte[] Seal(byte[] plaintext, object publicKey, byte[] ephemeralPrivate = null)
    {
        if (plaintext == null) throw new ArgumentNullException("plaintext");
        byte[] server;
        try
        {
            if (publicKey is string)
                server = Convert.FromBase64String((string)publicKey);
            else if (publicKey is byte[])
                server = (byte[])((byte[])publicKey).Clone();
            else
                server = new byte[0];
        }
        catch
        {
            server = new byte[0];
        }

        if (server.Length != 32)
            Fail("publicKey: 32 bytes");
        if (ephemeralPrivate != null && ephemeralPrivate.Length != 32)
            Fail("ephemeralPrivate: 32 bytes");

        X25519PrivateKeyParameters privateKey = ephemeralPrivate != null
            ? new X25519PrivateKeyParameters(ephemeralPrivate, 0)
            : new X25519PrivateKeyParameters(Random);
        byte[] ephemeralPublic = privateKey.GeneratePublicKey().GetEncoded();
        X25519PublicKeyParameters serverPublic = new X25519PublicKeyParameters(server, 0);
        byte[] shared = new byte[32];
        privateKey.GenerateSecret(serverPublic, shared, 0);

        byte[] salt = new byte[ephemeralPublic.Length + server.Length];
        Buffer.BlockCopy(ephemeralPublic, 0, salt, 0, ephemeralPublic.Length);
        Buffer.BlockCopy(server, 0, salt, ephemeralPublic.Length, server.Length);
        HkdfBytesGenerator hkdf = new HkdfBytesGenerator(new Sha256Digest());
        hkdf.Init(new HkdfParameters(shared, salt, Info));
        byte[] okm = new byte[44];
        hkdf.GenerateBytes(okm, 0, okm.Length);

        byte[] key = new byte[32];
        byte[] nonce = new byte[12];
        Buffer.BlockCopy(okm, 0, key, 0, key.Length);
        Buffer.BlockCopy(okm, 32, nonce, 0, nonce.Length);

        GcmBlockCipher cipher = new GcmBlockCipher(new AesEngine());
        cipher.Init(true, new AeadParameters(new KeyParameter(key), 128, nonce, new byte[] { Version }));
        byte[] encrypted = new byte[cipher.GetOutputSize(plaintext.Length)];
        int length = cipher.ProcessBytes(plaintext, 0, plaintext.Length, encrypted, 0);
        length += cipher.DoFinal(encrypted, length); // includes the 16-byte GCM tag

        byte[] result = new byte[1 + ephemeralPublic.Length + length];
        result[0] = Version;
        Buffer.BlockCopy(ephemeralPublic, 0, result, 1, ephemeralPublic.Length);
        Buffer.BlockCopy(encrypted, 0, result, 1 + ephemeralPublic.Length, length);
        return result;
    }

    public static string Base32(byte[] data)
    {
        const string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
        StringBuilder result = new StringBuilder((data.Length * 8 + 4) / 5);
        int buffer = 0, bitsLeft = 0;
        foreach (byte b in data)
        {
            buffer = (buffer << 8) | b;
            bitsLeft += 8;
            while (bitsLeft >= 5)
            {
                result.Append(alphabet[(buffer >> (bitsLeft - 5)) & 31]);
                bitsLeft -= 5;
            }
        }
        if (bitsLeft > 0)
            result.Append(alphabet[(buffer << (5 - bitsLeft)) & 31]);
        return result.ToString();
    }

    public static string QrCodeUrl(
        object publicKey,
        int discipline,
        bool decimalScoring,
        object sessionStart,
        IList<ShotmetrixShot> shots,
        int app = 0,
        byte[] appSecret = null)
    {
        byte[] pt = Plaintext(discipline, decimalScoring, sessionStart, shots, app, appSecret);
        return UrlPrefix + Base32(Seal(pt, publicKey));
    }
}
