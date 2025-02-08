using System;
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

            WinAmp.SetupWinamp();
            WinAmp.SetupMusicFolder();
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
