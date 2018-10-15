#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <objc/objc-runtime.h>
#include <os/log.h>
#include <CoreFoundation/CFRunLoop.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CoreFoundation.h>

#define kTimeInterval (0.1)
#define kTimeOnce (0.0)

#define POLL_INTERVAL 60

static CFRunLoopTimerRef gTimerRef;
static os_log_t log_handle = NULL;

static void timerCallBack(CFRunLoopTimerRef timerRef, void *info); // forward declaration

static id get_nsstring(const char *c_str)
{
    return objc_msgSend((id)objc_getClass("NSString"),
                        sel_registerName("stringWithUTF8String:"), c_str);
}

static char *get_cstring(CFStringRef str)
{
    CFIndex length = CFStringGetLength(str);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
    char *result = (char *)malloc(maxSize);
    CFStringGetCString(str, result, maxSize, kCFStringEncodingUTF8);
    return result;
}

static CFStringRef script = CFSTR(
    "tell application \"System Events\"\n"
    "  tell appearance preferences\n"
    "     set dark mode to %i\n"
    "   end tell\n"
    "end tell\n");

typedef struct
{
    double sunrise;
    double sunset;
    bool isDay;
} t_sundial;

/**
 * Writes message to both standard output and macOS log system
 */
void console_log(const char *fmt, ...)
{
    char *str = NULL;
    va_list ap;

    va_start(ap, fmt);
    vasprintf(&str, fmt, ap);
    va_end(ap);

    os_log(log_handle, "%{public}s", str);
    printf("%s\n", str);
    fflush(stdout);
    free(str);
}

/**
 * Retrieves information about current sun schedule (sunrise/sunset, etc.)
 */
t_sundial getSunInfo()
{
    // Create BrightnessSystemClient object (private API)
    Class bsclientClass = objc_getClass("BrightnessSystemClient");
    id bsClientInstance = class_createInstance(bsclientClass, 0);
    SEL initSel = sel_registerName("init");
    id bsClient = objc_msgSend(bsClientInstance, initSel);

    // Get brightness schedule (result is NSDictionary)
    SEL copySel = sel_registerName("copyPropertyForKey:");
    id scheduleDict = objc_msgSend(bsClient, copySel, CFSTR("BlueLightSunSchedule"));

    // Get sunrise/sunset properties (of dates type __NSTaggedDate)
    SEL objectForKeySel = sel_registerName("objectForKey:");
    id sunriseNSDate = objc_msgSend(scheduleDict, objectForKeySel, get_nsstring("sunrise"));
    id sunsetNSDate = objc_msgSend(scheduleDict, objectForKeySel, get_nsstring("sunset"));
    id isDaylight = objc_msgSend(scheduleDict, objectForKeySel, get_nsstring("isDaylight"));

    // Set sun info fields
    t_sundial data;
    data.sunrise = ((double (*)(id, SEL))objc_msgSend_fpret)(sunriseNSDate, sel_registerName("timeIntervalSince1970"));
    data.sunset = ((double (*)(id, SEL))objc_msgSend_fpret)(sunsetNSDate, sel_registerName("timeIntervalSince1970"));
    data.isDay = CFBooleanGetValue((CFBooleanRef)isDaylight);

    // Cleanup
    SEL release = sel_registerName("release");
    objc_msgSend(scheduleDict, release);
    objc_msgSend(bsClient, release);

    return data;
}

/**
 * Toggle dark mode, by executing AppleScript
 */
static void setDarkMode(int darkMode)
{
    //id err = objc_msgSend((id)objc_getClass("NSDictionary"), sel_registerName("new"));
    CFDictionaryRef err = CFDictionaryCreate(NULL, NULL, NULL, 0, NULL, NULL);
    id scriptString = objc_msgSend((id)objc_getClass("NSString"),
                                   sel_registerName("stringWithFormat:"), script, darkMode);
    id NSAppleScript = (id)objc_getClass("NSAppleScript");
    SEL alloc = sel_registerName("alloc");
    SEL init = sel_registerName("initWithSource:");
    SEL release = sel_registerName("release");
    id allocScript = objc_msgSend(NSAppleScript, alloc);
    id scriptRef = objc_msgSend(allocScript, init, scriptString);
    // Execute script
    console_log("%s", darkMode ? "☾ Darkness is comming" : "☀ Let there be light");
    id res = objc_msgSend(scriptRef, sel_registerName("executeAndReturnError:"), &err);
    if (res == NULL)
    {
        CFStringRef errorMessage = (CFStringRef)CFDictionaryGetValue(err, CFSTR("NSAppleScriptErrorMessage"));
        fprintf(stderr, "AppleScript error:\n%s\n",
                get_cstring(errorMessage));
        exit(1);
    }

    // Cleanup
    objc_msgSend(scriptRef, release);
    objc_msgSend(res, release);
}

/**
 * Creates interval timer for daylight status polling
 */
static void createTimer(t_sundial sunInfo)
{
    gTimerRef = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(), POLL_INTERVAL, 0, 0,
                                     timerCallBack, NULL);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), gTimerRef, kCFRunLoopDefaultMode);
}

int isDarkModeActive()
{
    CFStringRef prop = (CFStringRef)CFPreferencesCopyAppValue(
        CFSTR("AppleInterfaceStyle"),
        kCFPreferencesAnyApplication);
    if (prop == NULL)
    { // when darkmode is not set
        return 0;
    }
    return (CFStringCompare(prop, CFSTR("Dark"), kCFCompareCaseInsensitive) == kCFCompareEqualTo);
}

static void timerCallBack(CFRunLoopTimerRef timerRef, void *info)
{
    // Retrieve current sun status
    t_sundial sunInfo = getSunInfo();
    console_log("Daylight status → %s", sunInfo.isDay ? "☀" : "☾");
    if (isDarkModeActive() != !sunInfo.isDay)
    {
        setDarkMode(!sunInfo.isDay);
    }
}

void signalHandler(int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGQUIT:
    case SIGTERM:
        CFRunLoopStop(CFRunLoopGetCurrent());
        console_log("Stopped following the sun");
        break;
    default:
        fprintf(stderr, "Unhandled signal (%d) %s\n", sig, strsignal(sig));
        break;
    }
}

int main()
{
    log_handle = os_log_create("eu.onderweg", "default");
    console_log("Following the sun... checking every %i seconds", POLL_INTERVAL);

    t_sundial info = getSunInfo();
    createTimer(info);

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGQUIT, signalHandler);

    CFRunLoopRun();

    // Cleanup
    if (CFRunLoopTimerIsValid(gTimerRef))
    {
        CFRunLoopTimerInvalidate(gTimerRef);
        CFRelease(gTimerRef);
    }
    os_release(log_handle);
}
