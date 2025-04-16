using System;
using System.IO.Ports;
using System.Diagnostics;
using System.Windows.Forms;
using SerialSender.Properties;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using Newtonsoft.Json;
using System.Threading;
using System.Collections.Concurrent;
using System.IO;
using System.Net.Http;
using System.Linq.Expressions;
using System.CodeDom;
using System.Reflection.Emit;
using System.Threading.Tasks;
using static System.Windows.Forms.AxHost;



/// 
/// <summary>
/// New code based on serialsender from github, 25.12.2019 - Merry Christmas!
/// NoLimits
/// 
/// Code is strongly modified for Nextion display + esp32 in my case (but should work with any)
/// It produces a serial string of weather using openweathermap (replace API key - watch the metric units I use) as well as extensive hardware information (currently single GPU)
/// I managed to replace the weak openhardwaremonitor.dll with librehardwaremonitor.dll which has network and otehr stuff in it
/// As a total programming noob, this code may be improved at will
/// Code is free for all but credits for my work (3 weeks coding/learning) apprechiated
/// Advanced Serial Port Monitor can spy on the serial port after activating in the iconprocess process for debugging
/// Have Fun!
/// 
/// 02/02/2025 - Hello
/// Added some stuff check it out c:
/// 
/// </summary>
/// 
namespace SerialSender
{
    //////////////////////////////////////////////////////////////////

    // Class definitions used to deserialize JSON from API calls
    // Weather stuff
    public class WeatherObject
    {
        public Hourly hourly { get; set; }

        public Daily daily { get; set; }

    }

    public class Hourly
    {
        public List<float> temperature_2m { get; set; }
        public List<int> precipitation_probability { get; set; }
        public List<float> precipitation { get; set; }
        public List<int> weather_code { get; set; }
        public List<float> wind_speed_10m { get; set; }
        public List<int> wind_direction_10m { get; set; }
        public List<String> time { get; set; }
    }
    public class Daily
    {
        public List<String> sunrise { get; set; }
        public List<String> sunset { get; set; }
    }

    // Tide stuff
    public class RootTideObject
    {
        public List<Tide> data { get; set; }
    }

    public class Tide
    {
        public String time { get; set; }
        public float height { get; set; }
        public String type { get; set; }

    }


    /////////////////////////////////////////////////////////////////////

    public class ContextMenus
    {
        const int BAUD_RATE = 115200;
        SerialPort SelectedSerialPort;
        ContextMenuStrip menu;
        LibreHardwareMonitor.Hardware.Computer thisComputer;
        private System.Threading.Timer dataCheckTimer;
        private System.Threading.Timer weatherTimer;
        private System.Threading.Timer readSerialTimer;
        private System.Threading.Timer sendDataTimer;
        private System.Threading.Timer tideTimer;
        private System.Threading.Timer restartTimersTimer;


        private int isDataCheckRunning = 0;
        private int isWeatherRunning = 0;
        private int isReadSerialRunning = 0;
        private int isSendDataRunning = 0;
        private int isTideRunning = 0;



        private static readonly ConcurrentQueue<string> sendQueue = new ConcurrentQueue<string>();
        private bool isSending = false;
        private readonly object serialLock = new object();
        private readonly object sendingLock = new object();
        private static readonly int maxQueueSize = 20;

        private static List<string> songs = new List<string>();

        // Set default lat long to Auckland, NZ to match nextion default
        private static double latitude = -36.74;
        private static double longitude = 174.74;
        private static int deltaHours = 1; // Default fetch weather data in 1 hour gaps
        private static readonly object deltaHoursLock = new object();

        public ContextMenuStrip Create()
        {

            thisComputer = new LibreHardwareMonitor.Hardware.Computer() { };
            thisComputer.IsCpuEnabled = true;
            thisComputer.IsGpuEnabled = true;
            thisComputer.IsMemoryEnabled = true;
            thisComputer.IsNetworkEnabled = true;
            thisComputer.Open();

            menu = new ContextMenuStrip();
            CreateMenuItems();
            return menu;
        }

        void CreateMenuItems()
        {

            ToolStripMenuItem item;
            ToolStripSeparator sep;


            item = new ToolStripMenuItem();
            item.Text = "Serial Ports";
            menu.Items.Add(item);

            string[] ports = SerialPort.GetPortNames();

            foreach (string port in ports)
            {
                item = new ToolStripMenuItem();
                item.Text = port;
                item.Click += new EventHandler((sender, e) => Selected_Serial(sender, e, port));
                item.Image = Resources.Serial;
                menu.Items.Add(item);
            }


            sep = new ToolStripSeparator();
            menu.Items.Add(sep);

            item = new ToolStripMenuItem();
            item.Text = "Refresh";
            item.Click += new EventHandler((sender, e) => InvalidateMenu(menu));
            //item.Image = Resources.Exit;
            menu.Items.Add(item);

            sep = new ToolStripSeparator();
            menu.Items.Add(sep);

            item = new ToolStripMenuItem();
            item.Text = "WinAmp Dir";
            item.Click += (sender, e) => WinAmp.SelectInstallDir();
            menu.Items.Add(item);

            sep = new ToolStripSeparator();
            menu.Items.Add(sep);

            item = new ToolStripMenuItem();
            item.Text = "Music Folder";
            item.Click += (sender, e) => WinAmp.SelectMusicFolderDir();
            menu.Items.Add(item);

            sep = new ToolStripSeparator();
            menu.Items.Add(sep);

            item = new ToolStripMenuItem();
            item.Text = "Exit";
            item.Click += new System.EventHandler(Exit_Click);
            item.Image = Resources.Exit;
            menu.Items.Add(item);

        }

        void InvalidateMenu(ContextMenuStrip menu)
        {
            menu.Items.Clear();
            CreateMenuItems();
        }

        public static void EnqueueData(string data)
        {
            if (sendQueue.Count >= maxQueueSize)
            {
                Console.WriteLine("Queue is full. Discarding item.");
                return;
            }

            sendQueue.Enqueue(data);
        }

        public void sendData(object state)
        {

            if (Interlocked.CompareExchange(ref isSendDataRunning, 1, 0) != 0) return;

            try
            {
                if (sendQueue.TryDequeue(out string data))
                {
                    lock (serialLock)
                    {
                        Console.WriteLine("Sending");
                        SelectedSerialPort.Write(data);
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("Exception in sendData");
            }
            finally
            {
                Interlocked.Exchange(ref isSendDataRunning, 0);
            }
        }

        

        void Selected_Serial(object sender, EventArgs e, string selected_port)
        {
            if (SelectedSerialPort?.IsOpen == true)
                SelectedSerialPort.Close();
            
            SelectedSerialPort = new SerialPort(selected_port, BAUD_RATE);
            SelectedSerialPort.Open();

            StartTimers();
            //tideTimer = new System.Threading.Timer(tideData, null, 5000, 6 * 60 * 60 * 1000);
        }

        public void DisposeTimers()
        {
            // Safely dispose each timer if it exists
            dataCheckTimer?.Dispose();
            weatherTimer?.Dispose();
            readSerialTimer?.Dispose();
            sendDataTimer?.Dispose();
            restartTimersTimer?.Dispose();
            //tideTimer?.Dispose();

            // Set to null to avoid using disposed objects
            dataCheckTimer = null;
            weatherTimer = null;
            readSerialTimer = null;
            sendDataTimer = null;
            restartTimersTimer = null;
            //tideTimer = null;
        }

        public void StartTimers()
        {
            // Dispose existing timers if they exist
            DisposeTimers();

            // Create and start new timers
            dataCheckTimer = new System.Threading.Timer(dataCheck, null, 1000, 2500);
            weatherTimer = new System.Threading.Timer(weatherapp, null, 5000, 15*60*1000);
            readSerialTimer = new System.Threading.Timer(readSerial, null, 1000, 1000);
            sendDataTimer = new System.Threading.Timer(sendData, null, 1000, 1000);
            //tideTimer = new System.Threading.Timer(tideData, null, 5000, 6 * 60 * 60 * 1000);

            restartTimersTimer = new System.Threading.Timer(debugTimer, null, 5000, 5000);
        }


        private void debugTimer(object state)
        {
            
            ThreadPool.GetAvailableThreads(out int workerThreads, out int ioThreads);
            Console.WriteLine($"Threads: {workerThreads} worker, {ioThreads} I/O");
        }



        /////////////////////////////////////////////////////////

        private SerialDataReceivedEventHandler _dataReceivedHandler; // Store the handler

        public void readSerial(object state)
        {
            if (Interlocked.CompareExchange(ref isReadSerialRunning, 1, 0) != 0)
                return;

            try
            {
                // Remove old handler (if any)
                if (_dataReceivedHandler != null)
                    SelectedSerialPort.DataReceived -= _dataReceivedHandler;

                // Add new handler
                _dataReceivedHandler = (sender, e) =>
                {
                    string data = SelectedSerialPort.ReadLine().TrimEnd('\r');
                    Console.WriteLine("Received: " + data);
                    ProcessSerialData(data); // Move logic to a separate method
                };

                SelectedSerialPort.DataReceived += _dataReceivedHandler;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Exception in readSerial: {ex.Message}");
            }
            finally
            {
                Interlocked.Exchange(ref isReadSerialRunning, 0);
            }
        }

        private void ProcessSerialData(string data)
        {
            if (data == "REFRESHWEATHER")
            {
                Console.WriteLine("Refreshing Weather info");
                lock (deltaHoursLock)
                {
                    weatherapp(null);
                }
            }
            else if (data == "LOCKPC")
            {
                Console.WriteLine("Lock PC");
                Process.Start("rundll32.exe", "user32.dll,LockWorkStation");
            }
            else if (data == "REFRESHMUSIC")
            {
                SendSongString(WinAmp.musicFolderPath);
            }
            else if (data.StartsWith("PLAYMUSIC"))
            {
                String num = data.Substring(9, data.Length - 9);
                int index = int.Parse(num);
                String songDir;
                try
                {
                    songDir = songs[index];
                    Process.Start(WinAmp.directoryPath + "\\winamp.exe", "\"" + songDir + "\"");
                }
                catch (Exception)
                {
                    Console.WriteLine("Refresh music player nextion");
                }

            }
            else if (data == "PAUSEMUSIC")
            {
                Process.Start(WinAmp.directoryPath + "\\winamp.exe", "/pause");
            }
            else if (data == "INCREASEMUSIC")
            {
                WinAmp.SendMessage(WinAmp.hwnd, WinAmp.WM_COMMAND, (IntPtr)WinAmp.INCREASE_VOLUME, IntPtr.Zero);

            }
            else if (data == "DECREASEMUSIC")
            {
                WinAmp.SendMessage(WinAmp.hwnd, WinAmp.WM_COMMAND, (IntPtr)WinAmp.DECREASE_VOLUME, IntPtr.Zero);
            }
            else if (data == "REFRESHTIDE")
            {
                Console.WriteLine("Refreshing Tide info");
                tideData(null);
            }
            else if (data.StartsWith("SCHEDULE"))
            {
                if (data.Substring(8).StartsWith("CLEAR"))
                {
                    data = data.Substring(5);
                    string start = data.Substring(8, 4);
                    string end = data.Substring(12, 4);

                    int swStartIndex = 16;
                    int channelStartIndex = data.IndexOf("CHANNEL", swStartIndex) + "CHANNEL".Length;
                    string SW = data.Substring(swStartIndex, channelStartIndex - swStartIndex - "CHANNEL".Length);
                    string channel = data.Substring(channelStartIndex);

                    if (SW == "MOTIONSENSOR" || SW == "TEMPSENSOR")
                    {
                        channel = "";
                    }
                    Console.WriteLine("Clearing: " + SW + channel + start + end);
                    Scheduler.CancelTask(SW + channel + start + end);
                    Scheduler.CancelTask(SW + channel + start + end + "END");
                }
                else
                {
                    string start = data.Substring(8, 4);
                    string end = data.Substring(12, 4);

                    int swStartIndex = 16;
                    int channelStartIndex = data.IndexOf("CHANNEL", swStartIndex) + "CHANNEL".Length;
                    string SW = data.Substring(swStartIndex, channelStartIndex - swStartIndex - "CHANNEL".Length);
                    string channel = data.Substring(channelStartIndex);

                    if (SW == "MOTIONSENSOR" || SW == "TEMPSENSOR")
                    {
                        channel = "";
                    }

                    Console.WriteLine($"Start: {start}");
                    Console.WriteLine($"End: {end}");
                    Console.WriteLine($"Switch: {SW}");
                    Console.WriteLine($"Channel: {channel}");

                    Scheduler.ScheduleSwitch(SW, channel, start, end);

                    ////For testing
                    //start = DateTime.Now.AddMinutes(1).ToString("HHmm");
                    //end = DateTime.Now.AddMinutes(2).ToString("HHmm");
                    //Scheduler.ScheduleSwitch(SW, channel, start, end);
                }
            }
            else if (data.StartsWith("LOCATION"))
            {
                String location = data.Substring(8);
                int latStart = location.IndexOf("LAT") + 3;
                int latEnd = location.IndexOf("LONG");
                latitude = double.Parse(location.Substring(latStart, latEnd - latStart));

                int longStart = latEnd + 4;
                longitude = double.Parse(location.Substring(longStart));

                Console.WriteLine($"Latitude: {latitude}, Longitude: {longitude}");
            }
            else if (data.StartsWith("WEATHERDELTA"))
            {
                int deltaHoursLocal = int.Parse(data.Substring(12));

                lock (deltaHoursLock)
                {
                    deltaHours = deltaHoursLocal;
                    Console.WriteLine(deltaHours);
                }
            }
        }

        //public void readSerial(object state)
        //{

        //    if (Interlocked.CompareExchange(ref isReadSerialRunning, 1, 0) != 0) return;

        //    try
        //    {
        //        SelectedSerialPort.DataReceived += (sender, e) =>
        //        {
        //            string data = SelectedSerialPort.ReadLine();
        //            data = data.TrimEnd('\r');
        //            Console.WriteLine("Received: " + data);

        //            if (data == "REFRESHWEATHER")
        //            {
        //                Console.WriteLine("Refreshing Weather info");
        //                lock (deltaHoursLock)
        //                {
        //                    weatherapp(null);
        //                }
        //            }
        //            else if (data == "LOCKPC")
        //            {
        //                Console.WriteLine("Lock PC");
        //                Process.Start("rundll32.exe", "user32.dll,LockWorkStation");
        //            }
        //            else if (data == "REFRESHMUSIC")
        //            {
        //                SendSongString(WinAmp.musicFolderPath);
        //            }
        //            else if (data.StartsWith("PLAYMUSIC"))
        //            {
        //                String num = data.Substring(9, data.Length - 9);
        //                int index = int.Parse(num);
        //                String songDir;
        //                try
        //                {
        //                    songDir = songs[index];
        //                    Process.Start(WinAmp.directoryPath + "\\winamp.exe", "\"" + songDir + "\"");
        //                }
        //                catch (Exception)
        //                {
        //                    Console.WriteLine("Refresh music player nextion");
        //                }

        //            }
        //            else if (data == "PAUSEMUSIC")
        //            {
        //                Process.Start(WinAmp.directoryPath + "\\winamp.exe", "/pause");
        //            }
        //            else if (data == "INCREASEMUSIC")
        //            {
        //                WinAmp.SendMessage(WinAmp.hwnd, WinAmp.WM_COMMAND, (IntPtr)WinAmp.INCREASE_VOLUME, IntPtr.Zero);

        //            }
        //            else if (data == "DECREASEMUSIC")
        //            {
        //                WinAmp.SendMessage(WinAmp.hwnd, WinAmp.WM_COMMAND, (IntPtr)WinAmp.DECREASE_VOLUME, IntPtr.Zero);
        //            }
        //            else if (data == "REFRESHTIDE")
        //            {
        //                Console.WriteLine("Refreshing Tide info");
        //                tideData(null);
        //            }
        //            else if (data.StartsWith("SCHEDULE"))
        //            {
        //                if (data.Substring(8).StartsWith("CLEAR"))
        //                {
        //                    data = data.Substring(5);
        //                    string start = data.Substring(8, 4);
        //                    string end = data.Substring(12, 4);

        //                    int swStartIndex = 16;
        //                    int channelStartIndex = data.IndexOf("CHANNEL", swStartIndex) + "CHANNEL".Length;
        //                    string SW = data.Substring(swStartIndex, channelStartIndex - swStartIndex - "CHANNEL".Length);
        //                    string channel = data.Substring(channelStartIndex);

        //                    if (SW == "MOTIONSENSOR" || SW == "TEMPSENSOR")
        //                    {
        //                        channel = "";
        //                    }
        //                    Console.WriteLine("Clearing: " + SW + channel + start + end);
        //                    Scheduler.CancelTask(SW + channel + start + end);
        //                    Scheduler.CancelTask(SW + channel + start + end + "END");
        //                }
        //                else
        //                {
        //                    string start = data.Substring(8, 4);
        //                    string end = data.Substring(12, 4);

        //                    int swStartIndex = 16;
        //                    int channelStartIndex = data.IndexOf("CHANNEL", swStartIndex) + "CHANNEL".Length;
        //                    string SW = data.Substring(swStartIndex, channelStartIndex - swStartIndex - "CHANNEL".Length);
        //                    string channel = data.Substring(channelStartIndex);

        //                    if (SW == "MOTIONSENSOR" || SW == "TEMPSENSOR")
        //                    {
        //                        channel = "";
        //                    }

        //                    Console.WriteLine($"Start: {start}");
        //                    Console.WriteLine($"End: {end}");
        //                    Console.WriteLine($"Switch: {SW}");
        //                    Console.WriteLine($"Channel: {channel}");

        //                    Scheduler.ScheduleSwitch(SW, channel, start, end);

        //                    ////For testing
        //                    //start = DateTime.Now.AddMinutes(1).ToString("HHmm");
        //                    //end = DateTime.Now.AddMinutes(2).ToString("HHmm");
        //                    //Scheduler.ScheduleSwitch(SW, channel, start, end);
        //                }
        //            }
        //            else if (data.StartsWith("LOCATION"))
        //            {
        //                String location = data.Substring(8);
        //                int latStart = location.IndexOf("LAT") + 3;
        //                int latEnd = location.IndexOf("LONG");
        //                latitude = double.Parse(location.Substring(latStart, latEnd - latStart));

        //                int longStart = latEnd + 4;
        //                longitude = double.Parse(location.Substring(longStart));

        //                Console.WriteLine($"Latitude: {latitude}, Longitude: {longitude}");
        //            }
        //            else if (data.StartsWith("WEATHERDELTA"))
        //            {
        //                int deltaHoursLocal = int.Parse(data.Substring(12));

        //                lock (deltaHoursLock)
        //                {
        //                    deltaHours = deltaHoursLocal;
        //                    Console.WriteLine(deltaHours);
        //                }
        //            }
        //        };
        //    }
        //    catch
        //    {
        //        Console.WriteLine("Exception in readSerial");
        //    }
        //    finally
        //    {
        //        Interlocked.Exchange(ref isReadSerialRunning, 0);
        //    }
        //}

        /////////////////////////////////////////////////////////

        public static void SendSongString(string directoryPath)
        {
            songs.Clear();

            try
            {
                // All playable audio files in WinAmp
                var extensions = new[] { ".mp3", ".midi", ".mod", ".mpg", ".mpeg", ".aac", ".m4a", ".flac", ".wav", ".wma", ".ogg" };
                var files = Directory.GetFiles(directoryPath)
                    .Where(f => extensions.Any(ext => f.ToLower().EndsWith(ext)))
                    .ToList();

                var songOptions = string.Join("BREAK", files.Select(f => Path.GetFileName(f)));
                songs.AddRange(files);

                Console.WriteLine(songOptions);
                EnqueueData("MUSICSTRING" + songOptions + (char)0x03);
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }

        /////////////////////////////////////////////////////////

        public async void weatherapp(object state)
        {
            if (Interlocked.CompareExchange(ref isWeatherRunning, 1, 0) != 0) return;


            Console.WriteLine("Weather App function entered");

            try
            {
                using (HttpClient client = new HttpClient())
                {
                    Console.WriteLine("ACCESSING jsonWeather ...");
                    client.DefaultRequestHeaders.ConnectionClose = true;

                    string jsonWeather = await client.GetStringAsync($"https://api.open-meteo.com/v1/forecast?latitude={latitude}&longitude={longitude}&current=is_day&hourly=temperature_2m,precipitation_probability,precipitation,weather_code,wind_speed_10m,wind_direction_10m&daily=sunrise,sunset&timezone=auto");

                    //Console.WriteLine(jsonWeather);

                    var myweather = JsonConvert.DeserializeObject<WeatherObject>(jsonWeather);

                    int currentHour = DateTime.Now.Hour; // Used as initial index for hourly
                    int currentDay; // Used as index for daily

                    ForeCast currentForecast;

                    List<ForeCast> foreCastList = new List<ForeCast>();


                    for (int i = 1; i < 5; i++)
                    {
                        currentDay = (int)Math.Floor((float)currentHour / 24.0);

                        DateTime currentTime = DateTime.Parse(myweather.hourly.time[currentHour]);
                        DateTime sunsetTime = DateTime.Parse(myweather.daily.sunset[currentDay]);
                        DateTime sunriseTime = DateTime.Parse(myweather.daily.sunrise[currentDay]);

                        bool isDay = (currentTime >= sunriseTime) && (currentTime < sunsetTime);
                        int currentDelta = deltaHours * (i - 1); // how many hours past current forecast

                        currentForecast = new ForeCast
                        {
                            id = i,
                            delta = currentDelta,
                            temp = myweather.hourly.temperature_2m[currentHour],
                            prec_probability = myweather.hourly.precipitation_probability[currentHour],
                            prec = myweather.hourly.precipitation[currentHour],
                            weather_code = myweather.hourly.weather_code[currentHour],
                            wind_speed = myweather.hourly.wind_speed_10m[currentHour],
                            wind_dir = myweather.hourly.wind_direction_10m[currentHour],
                            isDay = isDay,
                        };

                        foreCastList.Add(currentForecast);
                        currentHour += deltaHours;
                    }

                    String location = latitude + ", " + longitude;

                    var locationString = "WEATHERLOCATION" + location + (char)0x03;
                    var weather1 = "WEATHER" + JsonConvert.SerializeObject(foreCastList[0]) + (char)0x03;
                    var weather2 = "WEATHER" + JsonConvert.SerializeObject(foreCastList[1]) + (char)0x03;
                    var weather3 = "WEATHER" + JsonConvert.SerializeObject(foreCastList[2]) + (char)0x03;
                    var weather4 = "WEATHER" + JsonConvert.SerializeObject(foreCastList[3]) + (char)0x03;

                    EnqueueData(locationString);
                    EnqueueData(weather1);
                    EnqueueData(weather2);
                    EnqueueData(weather3);
                    EnqueueData(weather4);

                    Console.WriteLine(locationString);
                    Console.WriteLine(weather1);
                    Console.WriteLine(weather2);
                    Console.WriteLine(weather3);
                    Console.WriteLine(weather4);
                }
            }
            catch (Exception)
            {
                Console.WriteLine("Exception in weather");
            }
            finally
            {
                Interlocked.Exchange(ref isWeatherRunning, 0);
            }
        }
        /////////////////////////////////////////////////////////
        public async void tideData(object state)
        {

            if (Interlocked.CompareExchange(ref isTideRunning, 1, 0) != 0) return;


            Console.WriteLine("ACCESSING tide information ...");
            TideForecast tideForecast = new TideForecast();

            try
            {
                using (var client = new HttpClient())
                {
                    Console.WriteLine("ACCESSING tide information ...");

                    DateTime utcTimeNow = DateTime.UtcNow;
                    DateTime utcTimeYesterday = utcTimeNow.AddHours(-24);

                    Console.WriteLine(utcTimeYesterday.ToString());
                    string isoFormat = utcTimeYesterday.ToString("o"); // Use "o" for RoundtripKind format
                    string urlEncoded = WebUtility.UrlEncode(isoFormat);

                    Console.WriteLine(urlEncoded);

                    client.DefaultRequestHeaders.Add("Authorization", Secret.StromGlassTideAPI);
                    string url = $"https://api.stormglass.io/v2/tide/extremes/point?lat={latitude.ToString()}&lng={longitude.ToString()}&start={urlEncoded}";

                    HttpResponseMessage response = await client.GetAsync(url);

                    if (response.IsSuccessStatusCode)
                    {
                        string jsonTide = await response.Content.ReadAsStringAsync();
                        Console.WriteLine(jsonTide);

                        var myTides = JsonConvert.DeserializeObject<RootTideObject>(jsonTide);
                        var tides = myTides.data.Take(12).ToList();

                        for (int i = 0; i < tides.Count; i++)
                        {
                            string timeString = tides[i].time;
                            DateTime time = DateTime.Parse(timeString);
                            string type = tides[i].type;
                            float height = tides[i].height;

                            if (DateTime.Now < time) // Find first occurence where the time is in the future
                            {
                                if (type == "high")
                                {
                                    String high1Time = DateTime.Parse(tides[i - 2].time).ToString("HH:mm");
                                    String lowTime = DateTime.Parse(tides[i - 1].time).ToString("HH:mm");
                                    String high2Time = time.ToString("HH:mm");

                                    tideForecast = new TideForecast
                                    {
                                        High1 = new TideData { height = tides[i - 2].height, time = high1Time },
                                        Low = new TideData { height = tides[i - 1].height, time = lowTime },
                                        High2 = new TideData { height = height, time = high2Time },
                                    };
                                }
                                else if (type == "low")
                                {
                                    String high1Time = DateTime.Parse(tides[i - 1].time).ToString("HH:mm");
                                    String lowTime = time.ToString("HH:mm");
                                    String high2Time = DateTime.Parse(tides[i + 1].time).ToString("HH:mm");

                                    tideForecast = new TideForecast
                                    {
                                        High1 = new TideData { height = tides[i - 1].height, time = high1Time },
                                        Low = new TideData { height = height, time = lowTime },
                                        High2 = new TideData { height = tides[i + 1].height, time = high2Time },
                                    };
                                }
                                tideForecast.currentTime = DateTime.Now.ToString("HH:mm");
                                break;
                            }
                        }

                        var tideJson = "TIDE" + JsonConvert.SerializeObject(tideForecast) + (char)0x03;
                        EnqueueData(tideJson);
                        Console.WriteLine(tideJson);
                    }
                    else
                    {
                        Console.WriteLine($"Error: {response.StatusCode}");
                    }
                }
            }
            catch (Exception)
            {

                Console.WriteLine("Exception in tideData");
            }
            finally
            {
                Interlocked.Exchange(ref isTideRunning, 0);
            }
        }
        /////////////////////////////////////////////////////////
        public void dataCheck(object state)
        {
            if (Interlocked.CompareExchange(ref isDataCheckRunning, 1, 0) != 0) return;

            try
            {
                float GpuTemp = -1.0f;
                float[] coreNoLoad = new float[20];
                float[] coreNoTemp = new float[20];
                float[] coreNoClock = new float[20];
                float RamUsed = -1.0f;
                float RamAvail = -1.0f;
                int cpuPackageTemp = 0;
                float gpuPower = -1.0f;
                float cpuPackagePower = -1.0f;

                foreach (LibreHardwareMonitor.Hardware.IHardware hw in thisComputer.Hardware)
                {
                    hw.Update();

                    foreach (LibreHardwareMonitor.Hardware.ISensor s in hw.Sensors)
                    {
                        //Console.WriteLine("NAME: " + s.Name + ", TYPE: " + s.SensorType + ", VALUE: " + s.Value);

                        // CPU  
                        if (s.SensorType == LibreHardwareMonitor.Hardware.SensorType.Temperature)
                        {
                            if (s.Value != null)
                            {
                                if (s.Name.StartsWith("CPU Package"))
                                {
                                    cpuPackageTemp = (int)Convert.ToDouble(s.Value);

                                }
                            }
                        }

                        if (s.SensorType == LibreHardwareMonitor.Hardware.SensorType.Power)
                        {
                            if (s.Value != null)
                            {
                                if (s.Name.StartsWith("CPU Package"))
                                {
                                    cpuPackagePower = (float)Convert.ToDouble(s.Value);

                                }
                            }
                        }


                        // GPU
                        if (s.SensorType == LibreHardwareMonitor.Hardware.SensorType.Temperature)
                        {
                            if (s.Value != null)
                            {
                                float gpuTemp = (float)Math.Round((double)s.Value, 2);
                                switch (s.Name)
                                {
                                    case "GPU Core":
                                        GpuTemp = gpuTemp;
                                        break;
                                }
                            }
                        }

                        if (s.SensorType == LibreHardwareMonitor.Hardware.SensorType.Power)
                        {
                            if (s.Value != null)
                            {
                                if (s.Name.StartsWith("GPU Power"))
                                {
                                    gpuPower = (float)Convert.ToDouble(s.Value);

                                }
                            }
                        }

                        // RAM
                        if (s.SensorType == LibreHardwareMonitor.Hardware.SensorType.Data)
                        {
                            if (s.Value != null)
                            {
                                float ramUsed = (float)Math.Round((double)s.Value, 2);
                                switch (s.Name)
                                {
                                    case "Memory Used":
                                        RamUsed = ramUsed;
                                        break;
                                }
                            }
                        }
                        if (s.SensorType == LibreHardwareMonitor.Hardware.SensorType.Data)
                        {
                            if (s.Value != null)
                            {
                                float ramAvail = (float)Math.Round((double)s.Value, 2);
                                switch (s.Name)
                                {
                                    case "Memory Available":
                                        RamAvail = ramAvail;
                                        break;
                                }
                            }
                        }
                    }
                }

                ComputerData computerData = new ComputerData
                {
                    RamUsed = RamUsed,
                    RamAvail = RamAvail,
                    GpuTemp = GpuTemp,
                    CpuPackageTemp = cpuPackageTemp,
                    CpuPackagePower = cpuPackagePower,
                    GpuPower = gpuPower
                };

                var json = "HARDWARE" + JsonConvert.SerializeObject(computerData) + (char)0x03;

                Console.WriteLine(json);
                EnqueueData(json);
            }
            catch
            {
                Console.WriteLine("Excpetion in dataCheck");
            }
            finally
            {
                Interlocked.Exchange(ref isDataCheckRunning, 0);
            }



        }

        void Exit_Click(object sender, EventArgs e)
        {
            WinAmp.SendMessage(WinAmp.hwnd, WinAmp.WM_COMMAND, (IntPtr)WinAmp.TOGGLE_VISIBILITY, IntPtr.Zero);
            WinAmp.SendMessage(WinAmp.hwnd, WinAmp.WM_COMMAND, (IntPtr)WinAmp.CLOSE_WINAMP, IntPtr.Zero);

            Application.Exit();
        }
    }
}