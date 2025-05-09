﻿// Copyright (c) 2025 RisosEnterprises Ltd. All rights reserved.
// Developed by RisosEnterprises Ltd, Auckland, New Zealand.
// Licensed under Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0).
// See https://creativecommons.org/licenses/by-nc/4.0/ for details.
// Author: Brian Han

using System;
using System.Collections.Concurrent;
using System.Threading;
using SerialSender;


static class Scheduler
{
    private static readonly ConcurrentDictionary<string, Timer> timers = new ConcurrentDictionary<string, Timer>();

    public static void ScheduleSwitch(string SW, string channel, string start, string end)
    {
        // Parse input time (format: HHmm, e.g., "0600")
        if (!TimeSpan.TryParseExact(start.Insert(2, ":"), "hh\\:mm", null, out TimeSpan startTime) ||
            !TimeSpan.TryParseExact(end.Insert(2, ":"), "hh\\:mm", null, out TimeSpan endTime))
        {
            Console.WriteLine("Invalid time format. Use HHmm (e.g., 0600).");
            return;
        }

        DateTime now = DateTime.Now;
        DateTime scheduledStart = now.Date + startTime; // now.date is today's date at 00:00:00
        DateTime scheduledEnd = now.Date + endTime;

        if (scheduledStart <= now)
        {
            scheduledStart = scheduledStart.AddDays(1);
        }

        if (scheduledEnd <= now)
        {
            scheduledEnd = scheduledEnd.AddDays(1);
        }

        if (scheduledEnd <= scheduledStart)
        {
            scheduledEnd = scheduledEnd.AddDays(1);
        }

        Console.WriteLine(SW);
        Console.WriteLine(scheduledStart);
        Console.WriteLine(scheduledEnd);

        TimeSpan delayToStart = scheduledStart - now;
        TimeSpan delayToEnd = scheduledEnd - now;

        Timer timer = new Timer(_ =>
        {
            String action;
            // Temp and motion sensors dont have channels as they aren't linked to actual switches in nextion
            if (SW == "TEMPSENSOR" || SW == "MOTIONSENSOR")
            {
                action = "ENABLE";
            }
            else
            {
                action = "ON";
            }
            Console.WriteLine("SCHEDULE" + SW + action + channel + (char)0x03);


            ContextMenus.EnqueueData("SCHEDULE" + SW + action + channel + (char)0x03);

        }, null, delayToStart, Timeout.InfiniteTimeSpan);

        Timer endTimer = new Timer(__ =>
        {
            String action;
            if (SW == "TEMPSENSOR" || SW == "MOTIONSENSOR")
            {
                action = "DISABLE";
            }
            else
            {
                action = "OFF";
            }
            Console.WriteLine("SCHEDULE" + SW + action + (char)0x03);
            ContextMenus.EnqueueData("SCHEDULE" + SW + action + (char)0x03);

            // Reschedule the same switch for the next day
            ScheduleSwitch(SW, channel, start, end);

        }, null, delayToEnd, Timeout.InfiniteTimeSpan);


        String timerId = SW + channel + start + end;

        // Store the timer
        if (timers.TryAdd(timerId, timer) && timers.TryAdd(timerId + "END", endTimer))
        {
            Console.WriteLine($"Timer ID: '{timerId}' scheduled to start at {scheduledStart:HH:mm:ss} and end at {scheduledEnd:HH:mm:ss}");
        }
    }

    public static void CancelTask(string timerId)
    {
        if (timers.TryRemove(timerId, out Timer timer))
        {
            timer.Dispose();
            Console.WriteLine($"'{timerId}' canceled.");
        }
        else
        {
            Console.WriteLine($"No timer found with ID '{timerId}'.");
        }
    }
}