using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace SerialSender
{
    public struct ForeCast
    {
        public int id;
        public int delta;
        public float temp;
        public int prec_probability;
        public float prec;
        public int weather_code;
        public float wind_speed;
        public int wind_dir;
        public bool isDay;
    }
}
