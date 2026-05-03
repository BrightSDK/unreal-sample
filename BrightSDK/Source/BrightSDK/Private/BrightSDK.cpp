// LICENSE_CODE ZON

#include "BrightSDK.h"

#if PLATFORM_WINDOWS
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#endif

#define LOCTEXT_NAMESPACE "FBrightSDKModule"

void FBrightSDKModule::StartupModule()
{
#if PLATFORM_WINDOWS
	FString DllDir = FPaths::Combine(
		IPluginManager::Get().FindPlugin(TEXT("BrightSDK"))->GetBaseDir(),
		TEXT("ThirdParty/Win64"));
	FPlatformProcess::PushDllDirectory(*DllDir);
	SdkHandle = FPlatformProcess::GetDllHandle(
		*FPaths::Combine(DllDir, TEXT("lum_sdk64.dll")));
	FPlatformProcess::PopDllDirectory(*DllDir);
#endif
}

void FBrightSDKModule::ShutdownModule()
{
#if PLATFORM_WINDOWS
	if (SdkHandle)
	{
		FPlatformProcess::FreeDllHandle(SdkHandle);
		SdkHandle = nullptr;
	}
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBrightSDKModule, BrightSDK)