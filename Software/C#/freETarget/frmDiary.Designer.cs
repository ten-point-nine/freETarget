namespace freETarget {
    partial class frmDiary {
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmDiary));
            this.btnOK = new System.Windows.Forms.Button();
            this.btnCancel = new System.Windows.Forms.Button();
            this.contextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.miBold = new System.Windows.Forms.ToolStripMenuItem();
            this.miItalic = new System.Windows.Forms.ToolStripMenuItem();
            this.miIncreaseSize = new System.Windows.Forms.ToolStripMenuItem();
            this.miDecreaseSize = new System.Windows.Forms.ToolStripMenuItem();
            this.trtbPage = new freETarget.TransparentRTB();
            this.contextMenuStrip.SuspendLayout();
            this.SuspendLayout();
            // 
            // btnOK
            // 
            this.btnOK.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnOK.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.btnOK.Location = new System.Drawing.Point(286, 12);
            this.btnOK.Name = "btnOK";
            this.btnOK.Size = new System.Drawing.Size(75, 23);
            this.btnOK.TabIndex = 1;
            this.btnOK.Text = "OK";
            this.btnOK.UseVisualStyleBackColor = true;
            this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
            // 
            // btnCancel
            // 
            this.btnCancel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btnCancel.Location = new System.Drawing.Point(286, 41);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new System.Drawing.Size(75, 23);
            this.btnCancel.TabIndex = 3;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new System.EventHandler(this.btnCancel_Click);
            // 
            // contextMenuStrip
            // 
            this.contextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.miBold,
            this.miItalic,
            this.miIncreaseSize,
            this.miDecreaseSize});
            this.contextMenuStrip.Name = "contextMenuStrip";
            this.contextMenuStrip.Size = new System.Drawing.Size(144, 92);
            // 
            // miBold
            // 
            this.miBold.Name = "miBold";
            this.miBold.Size = new System.Drawing.Size(143, 22);
            this.miBold.Text = "Bold";
            this.miBold.Click += new System.EventHandler(this.miBold_Click);
            // 
            // miItalic
            // 
            this.miItalic.Name = "miItalic";
            this.miItalic.Size = new System.Drawing.Size(143, 22);
            this.miItalic.Text = "Italic";
            this.miItalic.Click += new System.EventHandler(this.miItalic_Click);
            // 
            // miIncreaseSize
            // 
            this.miIncreaseSize.Name = "miIncreaseSize";
            this.miIncreaseSize.Size = new System.Drawing.Size(143, 22);
            this.miIncreaseSize.Text = "Increase size";
            this.miIncreaseSize.Click += new System.EventHandler(this.miIncreaseSize_Click);
            // 
            // miDecreaseSize
            // 
            this.miDecreaseSize.Name = "miDecreaseSize";
            this.miDecreaseSize.Size = new System.Drawing.Size(143, 22);
            this.miDecreaseSize.Text = "Decrease size";
            this.miDecreaseSize.Click += new System.EventHandler(this.miDecreaseSize_Click);
            // 
            // trtbPage
            // 
            this.trtbPage.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.trtbPage.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.trtbPage.ContextMenuStrip = this.contextMenuStrip;
            this.trtbPage.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(238)));
            this.trtbPage.Location = new System.Drawing.Point(0, 0);
            this.trtbPage.Name = "trtbPage";
            this.trtbPage.Size = new System.Drawing.Size(385, 423);
            this.trtbPage.TabIndex = 2;
            this.trtbPage.Text = "";
            // 
            // frmDiary
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("$this.BackgroundImage")));
            this.ClientSize = new System.Drawing.Size(386, 424);
            this.Controls.Add(this.btnCancel);
            this.Controls.Add(this.btnOK);
            this.Controls.Add(this.trtbPage);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MaximumSize = new System.Drawing.Size(490, 650);
            this.MinimumSize = new System.Drawing.Size(300, 300);
            this.Name = "frmDiary";
            this.Text = "Diary Entry";
            this.contextMenuStrip.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.Button btnOK;
        public TransparentRTB trtbPage;
        private System.Windows.Forms.Button btnCancel;
        private System.Windows.Forms.ContextMenuStrip contextMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem miBold;
        private System.Windows.Forms.ToolStripMenuItem miItalic;
        private System.Windows.Forms.ToolStripMenuItem miIncreaseSize;
        private System.Windows.Forms.ToolStripMenuItem miDecreaseSize;
    }
}