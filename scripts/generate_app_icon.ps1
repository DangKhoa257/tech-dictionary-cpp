param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '../resources/icons')
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies @('System.Drawing.Common', 'System.Drawing.Primitives', 'System.Private.Windows.GdiPlus', 'System.Private.Windows.Core') -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.IO;

public static class TechDictionaryIconBuilder {
    private const int MasterSize = 1024;

    private static GraphicsPath RoundedSquare(float x, float y, float size, float radius) {
        var path = new GraphicsPath();
        float diameter = radius * 2;
        path.AddArc(x, y, diameter, diameter, 180, 90);
        path.AddArc(x + size - diameter, y, diameter, diameter, 270, 90);
        path.AddArc(x + size - diameter, y + size - diameter, diameter, diameter, 0, 90);
        path.AddArc(x, y + size - diameter, diameter, diameter, 90, 90);
        path.CloseFigure();
        return path;
    }

    private static Bitmap DrawMaster() {
        var image = new Bitmap(MasterSize, MasterSize, PixelFormat.Format32bppArgb);
        using (var graphics = Graphics.FromImage(image)) {
            graphics.Clear(Color.Transparent);
            graphics.SmoothingMode = SmoothingMode.AntiAlias;
            graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
            using (var backdrop = RoundedSquare(28, 28, 968, 215))
            using (var gradient = new LinearGradientBrush(
                new PointF(80, 80), new PointF(940, 940),
                Color.FromArgb(33, 118, 244), Color.FromArgb(103, 45, 206))) {
                var blend = new ColorBlend();
                blend.Positions = new float[] { 0f, 0.48f, 1f };
                blend.Colors = new Color[] {
                    Color.FromArgb(35, 126, 247),
                    Color.FromArgb(55, 92, 232),
                    Color.FromArgb(108, 44, 205)
                };
                gradient.InterpolationColors = blend;
                graphics.FillPath(gradient, backdrop);
            }

            // A wide, filled book remains legible in the 16-pixel representation.
            using (var leftPage = new GraphicsPath())
            using (var rightPage = new GraphicsPath())
            using (var white = new SolidBrush(Color.White)) {
                leftPage.StartFigure();
                leftPage.AddBezier(496, 326, 389, 274, 284, 286, 191, 330);
                leftPage.AddLine(191, 330, 191, 692);
                leftPage.AddBezier(191, 692, 287, 650, 396, 665, 496, 722);
                leftPage.CloseFigure();
                rightPage.StartFigure();
                rightPage.AddBezier(528, 326, 635, 274, 740, 286, 833, 330);
                rightPage.AddLine(833, 330, 833, 692);
                rightPage.AddBezier(833, 692, 737, 650, 628, 665, 528, 722);
                rightPage.CloseFigure();
                graphics.FillPath(white, leftPage);
                graphics.FillPath(white, rightPage);
            }

            // Three connected nodes make the technology cue without competing with the book.
            using (var circuit = new Pen(Color.FromArgb(73, 66, 212), 25))
            using (var node = new SolidBrush(Color.FromArgb(73, 66, 212))) {
                circuit.StartCap = LineCap.Round;
                circuit.EndCap = LineCap.Round;
                circuit.LineJoin = LineJoin.Round;
                graphics.DrawLine(circuit, 615, 485, 704, 485);
                graphics.DrawLine(circuit, 704, 485, 704, 560);
                graphics.DrawLine(circuit, 704, 560, 765, 560);
                graphics.FillEllipse(node, 592, 462, 46, 46);
                graphics.FillEllipse(node, 742, 537, 46, 46);
            }
        }
        return image;
    }

    private static byte[] EncodePng(Bitmap image) {
        using (var stream = new MemoryStream()) {
            image.Save(stream, ImageFormat.Png);
            return stream.ToArray();
        }
    }

    private static Bitmap Resize(Bitmap source, int size) {
        var result = new Bitmap(size, size, PixelFormat.Format32bppArgb);
        using (var graphics = Graphics.FromImage(result)) {
            graphics.Clear(Color.Transparent);
            graphics.CompositingMode = CompositingMode.SourceCopy;
            graphics.InterpolationMode = InterpolationMode.HighQualityBicubic;
            graphics.SmoothingMode = SmoothingMode.HighQuality;
            graphics.PixelOffsetMode = PixelOffsetMode.HighQuality;
            graphics.DrawImage(source, new Rectangle(0, 0, size, size),
                new Rectangle(0, 0, source.Width, source.Height), GraphicsUnit.Pixel);
        }
        return result;
    }

    public static void Generate(string outputDirectory) {
        Directory.CreateDirectory(outputDirectory);
        string pngPath = Path.Combine(outputDirectory, "tech-dictionary.png");
        string icoPath = Path.Combine(outputDirectory, "tech-dictionary.ico");
        int[] sizes = new int[] { 16, 24, 32, 48, 64, 128, 256 };
        var images = new byte[sizes.Length][];
        using (var master = DrawMaster()) {
            master.Save(pngPath, ImageFormat.Png);
            for (int index = 0; index < sizes.Length; ++index) {
                using (var scaled = Resize(master, sizes[index])) images[index] = EncodePng(scaled);
            }
        }

        using (var stream = File.Create(icoPath))
        using (var writer = new BinaryWriter(stream)) {
            writer.Write((ushort)0);
            writer.Write((ushort)1);
            writer.Write((ushort)sizes.Length);
            int offset = 6 + sizes.Length * 16;
            for (int index = 0; index < sizes.Length; ++index) {
                int size = sizes[index];
                writer.Write((byte)(size == 256 ? 0 : size));
                writer.Write((byte)(size == 256 ? 0 : size));
                writer.Write((byte)0);
                writer.Write((byte)0);
                writer.Write((ushort)1);
                writer.Write((ushort)32);
                writer.Write((uint)images[index].Length);
                writer.Write((uint)offset);
                offset += images[index].Length;
            }
            foreach (byte[] image in images) writer.Write(image);
        }
    }
}
'@

[TechDictionaryIconBuilder]::Generate([System.IO.Path]::GetFullPath($OutputDirectory))
Write-Output "Generated application PNG and ICO in $OutputDirectory"
