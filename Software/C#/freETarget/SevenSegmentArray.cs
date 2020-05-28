using System;
using System.Windows.Forms;
using System.Drawing;
using System.ComponentModel;

namespace freETarget
{
    public class SevenSegmentArray : UserControl, ISupportInitialize
    {
        /// <summary>
        /// Array of segment controls that are currently children of this control.
        /// </summary>
        public SevenSegment[] segments = null;

        private int elementWidth = 10;
        private float italicFactor = 0.0F;
        private Color colorBackground = Color.DarkGray;
        private Color colorDark = Color.DimGray;
        private Color colorLight = Color.Red;
        private bool showDot = true;
        private Padding elementPadding;

        private string theValue = null;

        public SevenSegmentArray()
        {
            SuspendLayout();
            Name = "SevenSegmentArray";
            Size = new Size(100, 25);
            Resize += new EventHandler(SevenSegmentArray_Resize);
            ResumeLayout(false);

            TabStop = false;
            elementPadding = new Padding(4, 4, 4, 4);
            RecreateSegments(4);
        }

        /// <summary>
        /// Change the number of elements in our LED array. This destroys
        /// the previous elements, and creates new ones in their place, applying
        /// all the current options to the new ones.
        /// </summary>
        /// <param name="count">Number of elements to create.</param>
        private void RecreateSegments(int count)
        {
            if (segments != null)
                for (int i = 0; i < segments.Length; i++) { segments[i].Parent = null; segments[i].Dispose(); }

            if (count <= 0) return;
            segments = new SevenSegment[count];

            for (int i = 0; i < count; i++)
            {
                segments[i] = new SevenSegment();
                segments[i].Parent = this;
                segments[i].Top = 0;
                segments[i].Height = Height;
                segments[i].Anchor = AnchorStyles.Top | AnchorStyles.Bottom;
                segments[i].Visible = true;
            }

            ResizeSegments();
            UpdateSegments();
            Value = theValue;
        }

        /// <summary>
        /// Align the elements of the array to fit neatly within the
        /// width of the parent control.
        /// </summary>
        public void ResizeSegments()
        {
            int allGridWidth=0;
            foreach(SevenSegment s in segments) {
                if (s.ColonShow) {
                    allGridWidth += 60+1;
                } else {
                    allGridWidth += 48+1;
                }
                
            }

            for (int i = 0; i < segments.Length; i++)
            {
                int gridWidth = (segments[i].ColonShow) ? 60 : 48;
                int segWidth = (gridWidth * Width) / allGridWidth;   //Width / segments.Length;

                if (i == 0) {
                    segments[i].Left = 0;
                } else {
                    segments[i].Left = segments[i - 1].Left + segments[i - 1].Width+2; //Width * (segments.Length - 1 - i) / segments.Length;
                }
                segments[i].Width = segWidth;
            }
        }

        /// <summary>
        /// Update the properties of each element with the properties
        /// we have stored.
        /// </summary>
        private void UpdateSegments()
        {
            for (int i = 0; i < segments.Length; i++)
            {
                segments[i].ColorBackground = colorBackground;
                segments[i].ColorDark = colorDark;
                segments[i].ColorLight = colorLight;
                segments[i].ElementWidth = elementWidth;
                segments[i].ItalicFactor = italicFactor;
                segments[i].DecimalShow = showDot;
                segments[i].Padding = elementPadding;
            }
        }

        private void SevenSegmentArray_Resize(object sender, EventArgs e) { ResizeSegments(); }

        protected override void OnPaintBackground(PaintEventArgs e) { e.Graphics.Clear(colorBackground); }

        public void BeginInit() {

        }

        public void EndInit() {

        }

        /// <summary>
        /// Background color of the LED array.
        /// </summary>
        public Color ColorBackground { get { return colorBackground; } set { colorBackground = value; UpdateSegments(); } }
        /// <summary>
        /// Color of inactive LED segments.
        /// </summary>
        public Color ColorDark { get { return colorDark; } set { colorDark = value; UpdateSegments(); } }
        /// <summary>
        /// Color of active LED segments.
        /// </summary>
        public Color ColorLight { get { return colorLight; } set { colorLight = value; UpdateSegments(); } }

        /// <summary>
        /// Width of LED segments.
        /// </summary>
        public int ElementWidth { get { return elementWidth; } set { elementWidth = value; UpdateSegments(); } }
        /// <summary>
        /// Shear coefficient for italicizing the displays. Try a value like -0.1.
        /// </summary>
        public float ItalicFactor { get { return italicFactor; } set { italicFactor = value; UpdateSegments(); } }
        /// <summary>
        /// Specifies if the decimal point LED is displayed.
        /// </summary>
        public bool DecimalShow { get { return showDot; } set { showDot = value; UpdateSegments(); } }

        /// <summary>
        /// Number of seven-segment elements in this array.
        /// </summary>
        public int ArrayCount { get { return segments.Length; } set { if ((value > 0) && (value <= 100)) RecreateSegments(value); } }
        /// <summary>
        /// Padding that applies to each seven-segment element in the array.
        /// Tweak these numbers to get the perfect appearance for the array of your size.
        /// </summary>
        public Padding ElementPadding { get { return elementPadding; } set { elementPadding = value; UpdateSegments(); } }

        /// <summary>
        /// The value to be displayed on the LED array. This can contain numbers,
        /// certain letters, and decimal points.
        /// </summary>
        public string Value
        {
            get { return theValue; }
            set
            {
                theValue = value;
                for (int i = 0; i < segments.Length; i++) { segments[i].CustomPattern = 0; segments[i].DecimalOn = false; }
                if (theValue != null)
                {
                    int segmentIndex = 0;
                    for (int i = 0; i < theValue.Length; i++) {
                        if (segmentIndex >= segments.Length) break;
                        if (theValue[i] == '.') segments[segmentIndex].DecimalOn = true;
                        else segments[segmentIndex++].Value = theValue[i].ToString();
                    }
                }

                ResizeSegments();
            }
        }

    }
}
