# Follow the sun

Switch to Dark mode automatically after sunset, and back to Light mode after sunrise.

This tool was created mainly to experiment with:

- Accessing private frameworks in macOS
- Using CoreFoundation in C
- Using macOS Objective-C runtime library (`objc_msgSend`, `sel_registerName`, etc) in C

## Sun schedule info

Sunrise/Sunset info is provided by a private macOS system framework named *CoreBrightness*.
A bit more info on this undocumented API (objective c): https://github.com/thompsonate/Shifty/issues/20.

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

## Building

**Builds on macOS only**

```
$ make
```

Alternatively, directly with gcc:

```
$ gcc -F/System/Library/PrivateFrameworks -framework Foundation -framework CoreBrightness follow.c -o follow -lobjc
```

## Related

- [dark-mode](https://github.com/sindresorhus/dark-mode) by sindresorhus.
