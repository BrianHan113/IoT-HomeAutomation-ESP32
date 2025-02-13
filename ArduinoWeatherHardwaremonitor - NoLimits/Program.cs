// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

using System;
using System.Threading;
using System.Windows.Forms;

namespace SerialSender
{
    class Program
    {
        [STAThread]
        static void Main()
        {

            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            WinAmp.SetupMusicFolder();
            WinAmp.SetupWinamp();

            Console.WriteLine(WinAmp.directoryPath);
            Console.WriteLine(WinAmp.musicFolderPath);


            using (ProcessIcon pi = new ProcessIcon())
            {
                pi.Display();
                Application.Run();
            }
        }
    }
}
