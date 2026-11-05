namespace freETarget {
    partial class frmQRcode {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent() {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmQRcode));
            this.pbQRcode = new System.Windows.Forms.PictureBox();
            this.txtLink = new System.Windows.Forms.TextBox();
            this.label1 = new System.Windows.Forms.Label();
            this.button1 = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.btnShowLink = new System.Windows.Forms.Button();
            this.pbDown = new System.Windows.Forms.PictureBox();
            this.pbUp = new System.Windows.Forms.PictureBox();
            ((System.ComponentModel.ISupportInitialize)(this.pbQRcode)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pbDown)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pbUp)).BeginInit();
            this.SuspendLayout();
            // 
            // pbQRcode
            // 
            this.pbQRcode.Location = new System.Drawing.Point(11, 54);
            this.pbQRcode.Name = "pbQRcode";
            this.pbQRcode.Size = new System.Drawing.Size(440, 440);
            this.pbQRcode.TabIndex = 0;
            this.pbQRcode.TabStop = false;
            // 
            // txtLink
            // 
            this.txtLink.Location = new System.Drawing.Point(11, 536);
            this.txtLink.Multiline = true;
            this.txtLink.Name = "txtLink";
            this.txtLink.ReadOnly = true;
            this.txtLink.Size = new System.Drawing.Size(439, 91);
            this.txtLink.TabIndex = 1;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(8, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(237, 13);
            this.label1.TabIndex = 2;
            this.label1.Text = "Upload session score to your Shotmetrix account";
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(11, 25);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(440, 23);
            this.button1.TabIndex = 3;
            this.button1.Text = "Upload to Shotmetrix";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Location = new System.Drawing.Point(12, 504);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(415, 10);
            this.groupBox1.TabIndex = 4;
            this.groupBox1.TabStop = false;
            // 
            // btnShowLink
            // 
            this.btnShowLink.Image = ((System.Drawing.Image)(resources.GetObject("btnShowLink.Image")));
            this.btnShowLink.Location = new System.Drawing.Point(433, 500);
            this.btnShowLink.Name = "btnShowLink";
            this.btnShowLink.Size = new System.Drawing.Size(20, 24);
            this.btnShowLink.TabIndex = 5;
            this.btnShowLink.UseVisualStyleBackColor = true;
            this.btnShowLink.Click += new System.EventHandler(this.btnShowLink_Click);
            // 
            // pbDown
            // 
            this.pbDown.Image = ((System.Drawing.Image)(resources.GetObject("pbDown.Image")));
            this.pbDown.Location = new System.Drawing.Point(483, 104);
            this.pbDown.Name = "pbDown";
            this.pbDown.Size = new System.Drawing.Size(100, 50);
            this.pbDown.TabIndex = 6;
            this.pbDown.TabStop = false;
            // 
            // pbUp
            // 
            this.pbUp.Image = ((System.Drawing.Image)(resources.GetObject("pbUp.Image")));
            this.pbUp.Location = new System.Drawing.Point(483, 171);
            this.pbUp.Name = "pbUp";
            this.pbUp.Size = new System.Drawing.Size(100, 50);
            this.pbUp.TabIndex = 7;
            this.pbUp.TabStop = false;
            // 
            // frmQRcode
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(463, 530);
            this.Controls.Add(this.pbUp);
            this.Controls.Add(this.pbDown);
            this.Controls.Add(this.btnShowLink);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.txtLink);
            this.Controls.Add(this.pbQRcode);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmQRcode";
            this.ShowInTaskbar = false;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Session QR code";
            ((System.ComponentModel.ISupportInitialize)(this.pbQRcode)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pbDown)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pbUp)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox pbQRcode;
        private System.Windows.Forms.TextBox txtLink;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button button1;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Button btnShowLink;
        private System.Windows.Forms.PictureBox pbDown;
        private System.Windows.Forms.PictureBox pbUp;
    }
}