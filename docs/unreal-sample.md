# unreal-sample – Full Documentation

## Overview

Unreal Engine plugin that bridges BrightSDK iOS/tvOS Swift SDK (distributed as .xcframework) into Unreal using a C-compatible API (`@_cdecl`).

The plugin exposes a UGameInstanceSubsystem (UBrightSDKSubsystem) that you can call from both C++ and Blueprints.

Repository: https://github.com/BrightSDK/unreal-sample
Branch: main

## Requirements

- Unreal Engine 5.x (tested with UE 5.7)
- macOS + Xcode (for iOS/tvOS builds)
- iOS/tvOS Bright Data SDK as brdsdk.xcframework

## Current Repository State

### .gitignore

The repository includes a standard ignore file intended for source-control hygiene.

### License

`LICENSE` is MIT License with copyright:

- Copyright (c) 2026 BrightSDK

## Installation

### Add plugin

1. Copy the plugin folder into your Unreal project:

```
<YourProject>/
    Plugins/
        BrightSDK/
        BrightSDK.uplugin
        Source/
        Resources/
```

2. Enable the plugin:

- Unreal Editor → Edit → Plugins
- Find BrightSDK
- Enable → restart the editor when prompted.

3. Add the module dependency so your game code can include the subsystem header:

`Source/<YourProject>/<YourProject>.Build.cs`

```C#
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core", "CoreUObject", "Engine", "UMG",
    "BrightSDK"
});
```

### Add brdsdk.xcframework

1. Place the framework under your third party libraries folder (or where as you wish)

*For example:*

```
<YourProject>/
    ThirdParty/
```

2. In the plugin or your project module `Build.cs` file add dependency with a path to the framework. In this case we indicate `ThirdParty` folder in the project root

`Source/<YourProject>/<YourProject>.Build.cs`

```C#
if (Target.Platform == UnrealTargetPlatform.IOS)
{
    PublicAdditionalFrameworks.Add(
        new Framework(
            "brdsdk",
            "../../ThirdParty/BrightSdk/brdsdk.xcframework", 
            "",
            true
        )
    );
}
```

## What the plugin provides

### Runtime bridge (Subsystem)

`UBrightSDKSubsystem` (a `UGameInstanceSubsystem`) is the primary API surface.

**Typical capabilities:**

- `TryInitialize(...)` method to initialize SDK
- getters like `GetVersion()` / `GetUUIDOrEmpty()`
- actions like `OptOut(...)`
- events (delegates) like `OnChoiceChanged`

### Callbacks / events

The SDK can call back into Unreal (e.g. on choice change). The plugin maps SDK raw values to Unreal enums and broadcasts on the Game Thread.

## Usage

### Using the plugin in C++

1. Get the subsystem. From any `UObject` with access to a `UWorld`:

```C++
#include "BrightSDKSubsystem.h"
#include "Engine/GameInstance.h"

UBrightSDKSubsystem* GetBrightSDK(UObject* WorldContext)
{
    if (!WorldContext) return nullptr;
    UWorld* World = WorldContext->GetWorld();
    if (!World) return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return nullptr;

    return GI->GetSubsystem<UBrightSDKSubsystem>();
}
```

2. Initialize

*For example, your object is called `USDKWidget`*

```C++
if (UBrightSDKSubsystem* SDK = GetBrightSDK(this))
{
    SDK->OnChoiceChanged.AddDynamic(this, &USDKWidget::HandleChoiceChanged);
    const int64 Code = SDK->TryInitialize(
        TEXT(""), TEXT(""), TEXT(""),
        TEXT(""), TEXT(""),
        /*SkipConsent*/ false,
        TEXT("")
    );

    if (Code != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("BrightSDK init failed with code: %lld"), Code);
    }
    else
    {
        HandleChoiceChanged(SDK->GetCurrentChoice());
    }
}

...

void USDKWidget::HandleChoiceChanged(EBrightSDKChoice Choice)
{
    switch (Choice)
    {
        case EBrightSDKChoice::None:
            ChoiceLabel->SetText(FText::FromString(TEXT("Choice .none")));
            OptInOutButtonLabel->SetText(FText::FromString(TEXT("Opt in")));
            break;
        case EBrightSDKChoice::Peer:
            ChoiceLabel->SetText(FText::FromString(TEXT("Choice .peer")));
            OptInOutButtonLabel->SetText(FText::FromString(TEXT("Opt out")));
            break;
        case EBrightSDKChoice::NotPeer:
            ChoiceLabel->SetText(FText::FromString(TEXT("Choice .notPeer")));
            OptInOutButtonLabel->SetText(FText::FromString(TEXT("Opt in")));
            break;
    }
}
```

## API details

> String conventions:
>
> Empty FString is considered as null pointer.

### Enums

#### EBrightSDKChoice

Represents the user's current consent choice.

- None - user hasn't made a choice
- Peer - user agreed
- NotPeer - user disagreed

#### EBrightSDKChoiceTriggerType

Where an opt-in/opt-out action was triggered from.

- Consent - from consent screen
- Manual - from settings/code

#### EBrightSDKAuthorizationStatus

Result of checks of availability of using SDK. 

> Internally mapped from SDK negative raw status codes.

- Authorized
- SdkNotInitialized
- ParentControlEnabled
- Unknown

### Events (Delegates)

#### OnChoiceChanged

`BlueprintAssignable` dynamic multicast delegate fired when SDK reports a consent choice change.

- Broadcast thread: Game Thread (always)
- Blueprint: bind via "Assign/Bind Event to OnChoiceChanged"
- C++: bind via AddDynamic

**Signature**

```C++
OnChoiceChanged(EBrightSDKChoice NewChoice)
```

### Public Methods

#### TryInitialize

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
int64 TryInitialize(const FString& Benefit,
                    const FString& AgreeButton,
                    const FString& DisagreeButton,
                    const FString& OptOutInstructions,
                    const FString& AppIconNameOrPath,
                    bool SkipConsent,
                    const FString& Campaign) const;
```

#### GetVersion

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
FString GetVersion() const;
```

#### GetUUIDOrEmpty

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
FString GetUUIDOrEmpty() const;
```

#### AuthorizeDevice

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
EBrightSDKAuthorizationStatus AuthorizeDevice() const;
```

#### GetCurrentChoice

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
EBrightSDKChoice GetCurrentChoice() const;
```

#### ExternalOptIn

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
void ExternalOptIn(EBrightSDKChoiceTriggerType Trigger) const;
```

#### OptOut

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
void OptOut(EBrightSDKChoiceTriggerType Trigger) const;
```

#### NotifyConsentShown

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
void NotifyConsentShown() const;
```

#### ShowConsent

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
bool ShowConsent() const;
```

#### SetTrackingId

```C++
UFUNCTION(BlueprintCallable, Category="BrightSDK")
void SetTrackingId(const FString& TrackingId) const;
```