// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

namespace SerialSender
{
    public struct TideForecast
    {
        public TideData High1;
        public TideData Low;
        public TideData High2;
        public string currentTime;
    }

    public struct TideData
    {
        public string time;
        public float height;
    }
}
