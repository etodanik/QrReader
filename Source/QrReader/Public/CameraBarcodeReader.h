#pragma once

#include "CoreMinimal.h"
#include "ZXingUnreal.h"
#include "MediaAssets/Public/MediaTexture.h"
#include "Components/Widget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "CameraBarcodeReader.generated.h"

UENUM(BlueprintType)
enum ECameraType : uint8
{
	Unknown,
	Webcam = 9			UMETA(DisplayName = "Unspecified webcam"),
	WebcamFront = 10	UMETA(DisplayName = "Front facing webcam"),
	WebcamRear = 11		UMETA(DisplayName = "Rear facing webcam (default)"),
	Any = 12			UMETA(DisplayName = "Any webcam")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadBarcode, const TArray<UZXingResult*>&, Results);

/**
 * 
 */
UCLASS()
class QRREADER_API UCameraBarcodeReader : public UWidget, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* Image;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UOverlay* Overlay;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UScaleBox* ViewFinderScaleBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UScaleBox* VideoScaleBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UImage* ViewFinderImage;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	FString CameraOverride;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TEnumAsByte<ECameraType> CameraTypeOverride;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnReadBarcode OnBarcodeRead;
	
	virtual ~UCameraBarcodeReader() override;
	void GetFrameFromMaterial(TFunction<void(UTexture2D*)> Callback);

	// FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override;
    virtual bool IsTickableInEditor() const override;
    virtual UWorld* GetTickableGameObjectWorld() const override;
    virtual TStatId GetStatId() const override;
    // End FTickableGameObject interface

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	
private:
	void InitializeDynamicMaterial();
	UCameraBarcodeReader(const FObjectInitializer& ObjectInitializer);
	virtual TSharedRef<SWidget> RebuildWidget() override;

	FString DeviceUrl;
	FString DeviceDisplayName;
	int32 TickCount;

	UPROPERTY()
	UMediaPlayer* MediaPlayer;
	UPROPERTY()
	UMediaTexture* MediaTexture;	
	UPROPERTY()
	UTexture2D* CalibrationTexture;
	UPROPERTY()
	UMaterialInterface* BaseMaterial;
	UPROPERTY()
	UMaterialInterface* ViewFinderMaterial;
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;
	UPROPERTY()
	UTextureRenderTarget2D* RenderTarget;

	void ProcessFrameInBackground();

	UFUNCTION()
	void CatchMediaOpened(FString OpenedUrl);

	UFUNCTION()
	void CatchMediaOpenFailed(FString FailedUrl);

	UFUNCTION()
	void CatchEndReached();

	UFUNCTION()
	void CatchPlaybackSuspended();
};
