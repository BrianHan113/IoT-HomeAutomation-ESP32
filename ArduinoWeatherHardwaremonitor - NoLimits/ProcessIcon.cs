// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

using System;
using System.Diagnostics;
using System.Windows.Forms;
using SerialSender.Properties;

namespace SerialSender
{
    class ProcessIcon : IDisposable
    {
        NotifyIcon ni;
        public ProcessIcon()
        {
            ni = new NotifyIcon();
        }
        public void Display()
        {

            ni.MouseClick += new MouseEventHandler(ni_MouseClick);
            ni.Icon = Resources.TrayIcon;
            ni.Text = "Serial Sender";
            ni.Visible = true;

            // Attach a context menu.
            ni.ContextMenuStrip = new ContextMenus().Create();
        }
        public void Dispose()
        {

            ni.Dispose();
        }
        void ni_MouseClick(object sender, MouseEventArgs e)
        {

        }
    }
}
