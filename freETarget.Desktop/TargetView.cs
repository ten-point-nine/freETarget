using Avalonia;
using Avalonia.Controls;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using SkiaSharp;
using System;

namespace freETarget.Desktop {

    /** shows the target of the current session with its shots. the picture is square and uses the smaller side of the control */
    public class TargetView : Control {

        private MainController controller;
        private WriteableBitmap bitmap;
        private int zoom = 1;

        public void attach(MainController controller) {
            this.controller = controller;
        }

        public int Zoom {
            get { return zoom; }
            set { zoom = value; refresh(); }
        }

        /** side of the picture in the units of the layout */
        private int getDimension() {
            return (int)Math.Floor(Math.Min(Bounds.Width, Bounds.Height));
        }

        /** paints the target again. call whenever the shots, the session or the connection change */
        public void refresh() {
            int dimension = getDimension();
            if (controller == null || controller.currentSession == null || dimension <= 0) {
                return;
            }

            //paint with the pixels of the screen, so that the target is sharp on high resolution displays
            double scaling = TopLevel.GetTopLevel(this)?.RenderScaling ?? 1.0;
            int pixels = Math.Max(1, (int)Math.Round(dimension * scaling));

            if (bitmap == null || bitmap.PixelSize.Width != pixels) {
                bitmap?.Dispose();
                bitmap = new WriteableBitmap(new PixelSize(pixels, pixels), new Vector(96, 96), PixelFormat.Bgra8888, AlphaFormat.Premul);
            }

            using (ILockedFramebuffer buffer = bitmap.Lock()) {
                SKImageInfo info = new SKImageInfo(pixels, pixels, SKColorType.Bgra8888, SKAlphaType.Premul);
                using (SKSurface surface = SKSurface.Create(info, buffer.Address, buffer.RowBytes)) {
                    SKCanvas canvas = surface.Canvas;
                    canvas.Clear(SKColors.Transparent);
                    canvas.Scale((float)pixels / dimension);
                    bool notConnected = controller.currentStatus == MainController.Status.NOT_CONNECTED;
                    TargetRenderer.paint(canvas, controller.currentSession.getTarget(), dimension, zoom, notConnected, controller.currentSession, controller.getShotList());
                    canvas.Flush();
                }
            }

            InvalidateVisual();
        }

        protected override void OnSizeChanged(SizeChangedEventArgs e) {
            base.OnSizeChanged(e);
            refresh();
        }

        public override void Render(DrawingContext context) {
            base.Render(context);
            if (bitmap == null) {
                return;
            }
            int dimension = getDimension();
            double left = (Bounds.Width - dimension) / 2;
            double top = (Bounds.Height - dimension) / 2;
            context.DrawImage(bitmap, new Rect(0, 0, bitmap.PixelSize.Width, bitmap.PixelSize.Height), new Rect(left, top, dimension, dimension));
        }
    }
}
