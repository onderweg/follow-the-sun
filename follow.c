#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <objc/objc-runtime.h>
#include <os/log.h>
#include <CoreFoundation/CFRunLoop.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CoreFoundation.h>

#define ANSI_BOLD "\033[1m"
#define ANSI_BLUE "\033[38;5;039m"
#define ANSI_YELLOW "\033[38;5;228m"
#define ANSI_RESET "\033[0m" // To flush out prev settings

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

static void removeAnsiEsc(char *src, char *dest)
{
	bool cut = false;
	for(; *src; src++)
	{
		if (*src == (char)0x1b) cut = true;
		if (!cut) *dest++ = *src;
		if (*src == 'm' || *src == 'K') cut = false;
	}
	*dest = '\0';
}

static void _log(FILE * stream, const char *fmt, va_list ap) {
    char *str = NULL;    
    char plainStr[1024];    
    vasprintf(&str, fmt, ap);
    removeAnsiEsc(str, plainStr);
    if (stream == stdout) {        
        os_log(log_handle, "%{public}s", plainStr);
    } else {
        os_log_error(log_handle, "%{public}s", plainStr);
    }
    fprintf(stream, "%s\n", str);
    fflush(stream);
    free(str);
}    

/**
 * Writes message to both standard output and macOS log system
 */
void console_log(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _log(stdout, fmt, ap);
    va_end(ap);       
}

/**
 * Writes message to both standard err and macOS log system
 */
void console_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _log(stderr, fmt, ap);
    va_end(ap);  
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
    if (scheduleDict == NULL)
    {
        console_error("\nError: Sunset/Sunrise schedule not available.");
        fprintf(stderr, "📍 This might be because the system can't determine your location,\n");
        fprintf(stderr, "this can be the case for example when you're not connected to a wifi network.\n");        
        exit(1);
    }

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
    console_log("%s", darkMode ? "☾ Darkness is coming" : "☀ Let there be light");

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
        return NO;
    }
    return (CFStringCompare(prop, CFSTR("Dark"), kCFCompareCaseInsensitive) == kCFCompareEqualTo);
}

static void timerCallBack(CFRunLoopTimerRef timerRef, void *info)
{    
    t_sundial sunInfo = getSunInfo();
    console_log("Daylight status → %s%s%s", ANSI_YELLOW, sunInfo.isDay ? "☀" : "☾", ANSI_RESET);
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
        console_error("Unhandled signal (%d) %s\n", sig, strsignal(sig));
        break;
    }
}

int main()
{
    log_handle = os_log_create("eu.onderweg", getppid() == 1 ? "daemon" : "default");
    console_log("Following the sun... checking every %s%i%s seconds", ANSI_BOLD, POLL_INTERVAL, ANSI_RESET);

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
