// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

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
