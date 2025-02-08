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
