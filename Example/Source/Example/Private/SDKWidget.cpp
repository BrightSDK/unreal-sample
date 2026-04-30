#include "SDKWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Misc/App.h"
#include "BrightSDKSubsystem.h"

namespace
{
    static FString BuildVersionLine(const FString& Version)
    {
        return FString::Printf(TEXT("Version: %s"), *Version);
    }
}

void USDKWidget::RefreshSdkDetails()
{
    if (!sdk)
    {
        return;
    }

    const FString UUID = sdk->GetUUIDOrEmpty();
    const FString VersionLine = BuildVersionLine(sdk->GetVersion());

    if (UUIDLabel)
    {
        if (VersionLabel)
        {
            UUIDLabel->SetText(FText::FromString(UUID));
        }
        else if (UUID.IsEmpty())
        {
            UUIDLabel->SetText(FText::FromString(VersionLine));
        }
        else
        {
            UUIDLabel->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), *UUID, *VersionLine)));
        }
    }

    if (VersionLabel)
    {
        VersionLabel->SetText(FText::FromString(VersionLine));
    }
}

void USDKWidget::NativeConstruct()
{
    sdk = nullptr;
    Super::NativeConstruct();

    if (ShowConsentButton)
        ShowConsentButton->OnClicked.AddDynamic(this, &USDKWidget::OnShowConsentButtonClicked);
    if (OptInOutButton)
        OptInOutButton->OnClicked.AddDynamic(this, &USDKWidget::OnOptInOutButtonClicked);

    if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        sdk = GI->GetSubsystem<UBrightSDKSubsystem>();
        sdk->OnChoiceChanged.AddDynamic(this, &USDKWidget::HandleChoiceChanged);
        sdk->WindowsPreInit(TEXT("win_brightdata.electron_sample_app"), TEXT("UE SDK Example"));
        sdk->TryInitialize(TEXT("THIS IS TEST BENEFIT"),
                           TEXT("ACCEPT"),
                           TEXT("DECLINE"),
                           TEXT("TOUCH `Opt out` BUTTON"),
                           TEXT(""),
                           false,
                           TEXT(""));
        RefreshSdkDetails();
        HandleChoiceChanged(sdk->GetCurrentChoice());
    }
}

void USDKWidget::OnShowConsentButtonClicked()
{
    if (!sdk) return;

    UE_LOG(LogTemp, Warning, TEXT("==>> SHOW CONSENT"));
    sdk->ShowConsent();
}

void USDKWidget::OnOptInOutButtonClicked()
{
    if (!sdk) return;
    if (sdk->GetCurrentChoice() == EBrightSDKChoice::Peer)
        sdk->OptOut(EBrightSDKChoiceTriggerType::Manual);
    else
        sdk->ExternalOptIn(EBrightSDKChoiceTriggerType::Manual);
}

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
