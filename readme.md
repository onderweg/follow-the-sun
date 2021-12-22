# ☀️ Follow the sun

**[2021 Edit]** When this program was made, the functionality to automatically switch to dark mode was not build into macOS yet. In macOS Catalina however, auto dark mode was introduced, making this program superfluous. I keep the code here as reference anyway.

---

## What is this? 

CLI tool that can run on the background, that switches macOS to Dark mode automatically after sunset, and back to Light mode after sunrise.

This tool was created mainly to experiment with:

- Accessing private frameworks in macOS
- Using CoreFoundation in C
- Using macOS Objective-C runtime library (`objc_msgSend`, `sel_registerName`, etc) in C

## Sun schedule info 🌓

Sunrise/Sunset info is provided by a private macOS system framework named *CoreBrightness*.
A bit more info on this undocumented API (Objective-C): https://github.com/thompsonate/Shifty/issues/20.

Example output from this API:

```
{
    isDaylight = 0;
    nextSunrise = "2018-10-07 05:55:03 +0000";
    nextSunset = "2018-10-06 16:59:34 +0000";
    previousSunrise = "2018-10-05 05:51:38 +0000";
    previousSunset = "2018-10-04 17:04:09 +0000";
    sunrise = "2018-10-06 05:53:21 +0000";
    sunset = "2018-10-05 17:01:52 +0000";
}
```

You can retrieve the above schedule information from the command line:

```shell
$ /usr/bin/corebrightnessdiag sunschedule
```

Note that CoreBrightness needs to know your location in order to calculate the correct sunset and sunrise times. If Wifi is disabled, CoreBrightness (via `CLLocationManager`)
might not be able to determine your location. In that case, the calculated sun schedule is not correct.
So make sure WiFi is turned on. 

## Logging

This tool logs to macOS system log. This is especially useful when being run as an agent. Log messages can be viewed either in Console.app or on terminal:

```
$ log show --predicate 'subsystem CONTAINS "eu.onderweg"' --last 30m
# Or, stream
$ log stream --predicate 'subsystem CONTAINS "eu.onderweg"'
```

## Building

**Builds on macOS only**

```
$ make && make install
```

Alternatively, directly with gcc:

```
$ gcc -F/System/Library/PrivateFrameworks -framework Foundation -framework CoreBrightness follow.c -o follow -lobjc
```

Or, install with [clib](https://github.com/clibs/clib):

```
$ clib install onderweg/follow-the-sun
```

## Running in background

See: [Run in background (via launchctl)](agent/readme.md)

## Similar projects

- [dark-mode](https://github.com/sindresorhus/dark-mode) by sindresorhus.
