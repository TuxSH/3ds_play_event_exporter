# play_event_exporter

(c) 2025 TuxSH.

Licensed under the ISC License.

## Description

Exports "Play History" (on-device telemetry) to a text file on the SD card.

As it turns out, the 3DS's OS logs a lot more than what it shown in Activity Log. Example output of this program:

```log
2015-01-11 04:41: System shutdown
2015-01-11 04:44: Applet launch (0004003000009802)
2015-01-11 04:45: Shell close
2015-01-11 04:45: Shell open
2015-01-11 04:45: Shell close
2015-01-11 04:45: Shell open
2015-01-11 04:45: Shell close
2015-01-11 04:45: Shell open
2015-01-11 04:45: System shutdown
2015-01-11 04:46: Applet launch (0004003000009802)
2015-01-11 04:47: Leave applet (0004003000009802)
2015-01-11 04:47: Application launch (0004001000022100)
2015-01-11 04:48: Leave application (0004001000022100)
2015-01-11 04:48: Jump to applet (0004003000009802)
2015-01-11 04:48: Application exit (0004001000022100)
2015-01-11 04:48: Leave applet (0004003000009802)
2015-01-11 04:48: Application launch (0004001000022100)
2015-01-11 04:49: Leave application (0004001000022100)
2015-01-11 04:49: Jump to applet (0004003000009802)
2015-01-11 04:49: System shutdown
2015-01-11 04:49: Applet launch (0004003000009802)
2015-01-11 04:50: Leave applet (0004003000009802)
2015-01-11 04:50: Application launch (0004001000022000)
2015-01-11 04:51: Application exit (0004001000022000)
2015-01-11 04:51: Jump to applet (0004003000009802)
2015-01-11 04:51: System shutdown
2015-01-11 04:52: Applet launch (0004003000009802)
2024-08-31 17:45: User time change from 2015-01-11 04:52 to 2024-08-31 17:45
2024-08-31 17:45: Leave applet (0004003000009802)
2024-08-31 17:45: Application launch (0004001000022100)
2024-08-31 17:46: Leave application (0004001000022100)
2024-08-31 17:46: Jump to applet (0004003000009802)
2024-08-31 17:46: Application exit (0004001000022100)
2024-08-31 17:46: Leave applet (0004003000009802)
2024-08-31 17:46: Application launch (0004001000022100)
2024-08-31 17:48: Leave application (0004001000022100)
2024-08-31 17:48: Jump to applet (0004003000009802)
2024-08-31 17:47: System shutdown

...

2025-11-24 04:37: Application launch (0004001000022100)
2025-11-24 04:38: Shell close
2025-11-24 04:42: Shell open
2025-11-24 04:42: Leave application (0004001000022100)
2025-11-24 04:42: Jump to applet (0004003000009802)
2025-11-24 04:42: Application exit (0004001000022100)
2025-11-24 04:42: System shutdown
```

## Type of events

```c
/// Play event types. "Applet" includes "HOME Menu". Title self-chainloading (e.g. Homebrew Menu) is not recorded
typedef enum PtmPlayEventType {
    PTMPLAYEVENT_APPLICATION_LAUNCH         = 0,    ///< Application launch (special meaning for invalid TitleId upon launching DSi title)
    PTMPLAYEVENT_APPLICATION_EXIT           = 1,    ///< Application exit   (likewise)
    PTMPLAYEVENT_APPLET_LAUNCH              = 2,    ///< Applet launch
    PTMPLAYEVENT_APPLET_EXIT                = 3,    ///< Applet exit
    PTMPLAYEVENT_JUMP_TO_APPLICATION        = 4,    ///< Jump to application
    PTMPLAYEVENT_LEAVE_APPLICATION          = 5,    ///< Leave application (e.g. suspend to HOME Menu)
    PTMPLAYEVENT_JUMP_TO_APPLET             = 6,    ///< Jump to applet
    PTMPLAYEVENT_LEAVE_APPLET               = 7,    ///< Leave applet
    PTMPLAYEVENT_SHELL_CLOSE                = 8,    ///< Shell close (no TitleId)
    PTMPLAYEVENT_SHELL_OPEN                 = 9,    ///< Shell open (no TitleId)
    PTMPLAYEVENT_SYSTEM_SHUTDOWN            = 10,   ///< System shutdown (no TitleId)
    PTMPLAYEVENT_USER_TIME_CHANGE_OLD_TIME  = 11,   ///< User time change - old time (no TitleId, old user time stored in \ref minutesSince2000)
    PTMPLAYEVENT_USER_TIME_CHANGE_NEW_TIME  = 12,   ///< User time change - new time (no TitleId, new user time stored in \ref minutesSince2000)
} PtmPlayEventType;
```

## Bad system shutdown detection

PTM updates a `PTMPLAYEVENT_SYSTEM_SHUTDOWN` entry in the RAM of the always-on MCU, every 5 minutes. Therefore, unless battery is removed, unexpected system shutdowns can be detected with good enough precision.

## Handling of GBA/DSi titles

Due to technical restrictions, besides shutdown detection, only shell open -> shell close segments are tracked in `AgbBg` and `TwlBg`. There can only be up to 12 such segments starting on any given day, up to 7 days past the start of the day the game is launched, not included (D+0 00:00 -> D+6 23:59 inclusive range); these segments are stored in MCU RAM as well.

The resolution of the segment start date and time is 10 minutes (counting from 00:00). The resolution of the duration byte depends on the length of the segment, with as low as 20 minutes for 12-to-24h segments.

Furthermore, the way GBA and DSi titles are logged differ. For example, with start app -> shell close -> return to HOME:

**GBA**:

```log
2025-11-24 17:50: Application launch (0004000000075300)
2025-11-24 17:51: Shell close
2025-11-24 18:00: Shell open
2025-11-24 18:01: Application exit (0004000000075300)
2025-11-24 18:01: System shutdown
2025-11-24 17:51: Applet launch (0004003000009802)
```

**DSi**:

```log
2025-11-24 17:51: Application launch (0004800049524546)
2025-11-24 17:51: System shutdown
2025-11-24 17:51: DSi application start
2025-11-24 17:52: Shell close
2025-11-24 18:00: Shell open
2025-11-24 18:01: DSi application exit
2025-11-24 17:52: Applet launch (0004003000009802)
```

## Building the application

This application uses CMake with the default 3DS toolchain file provided by devkitARM. You can use the `catnip` CMake wrapper to make it simple:

```bash
catnip build
```
