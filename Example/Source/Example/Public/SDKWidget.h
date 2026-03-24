#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SDKWidget.generated.h"

class UButton;
class UTextBlock;
class UBrightSDKSubsystem;

UCLASS()
class EXAMPLE_API USDKWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnShowConsentButtonClicked();
    UFUNCTION()
    void OnOptInOutButtonClicked();
    UFUNCTION()
    void HandleChoiceChanged(EBrightSDKChoice Choice);

    UPROPERTY(meta = (BindWidget))
    UButton* ShowConsentButton;
    UPROPERTY(meta = (BindWidget))
    UButton* OptInOutButton;

    UPROPERTY(meta = (BindWidget))
    UTextBlock* UUIDLabel;
    UPROPERTY(meta = (BindWidget))
    UTextBlock* ChoiceLabel;
    UPROPERTY(meta = (BindWidget))
    UTextBlock* OptInOutButtonLabel;
private:
    UBrightSDKSubsystem *sdk;
};
