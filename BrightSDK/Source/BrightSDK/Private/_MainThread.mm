// LICENSE_CODE ZON

#if PLATFORM_IOS || PLATFORM_TVOS
#import <Foundation/Foundation.h>

extern "C" void _RunOnIOSMainThreadSync(void (^block)(void))
{
    if ([NSThread isMainThread]) {
        return block();
    }
    dispatch_sync(dispatch_get_main_queue(), block);
}
#endif
