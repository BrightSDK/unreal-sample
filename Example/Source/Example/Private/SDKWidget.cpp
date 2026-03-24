#include "SDKWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "BrightSDKSubsystem.h"

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
        sdk->TryInitialize(TEXT("THIS IS TEST BENEFIT"),
                           TEXT("ACCEPT"),
                           TEXT("DECLINE"),
                           TEXT("TOUCH `Opt out` BUTTON"),
                           TEXT(""),
                           false,
                           TEXT(""));
        UUIDLabel->SetText(FText::FromString(sdk->GetUUIDOrEmpty()));
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
