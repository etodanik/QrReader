#pragma once

#include "CoreMinimal.h"
#include "MediaAssets/Public/MediaPlayer.h"
#include "MediaAssets/Public/MediaTexture.h"
#include "Components/Widget.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ScaleBox.h"
#include "CameraBarcodeReader.generated.h"

/**
 * 
 */
UCLASS()
class QRREADER_API UCameraBarcodeReader : public UWidget, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UCameraBarcodeReader(const FObjectInitializer& ObjectInitializer);
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* Image;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UOverlay* Overlay;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UScaleBox* ViewFinderScaleBox;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UScaleBox* VideoScaleBox;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UImage* ViewFinderImage;
	
	virtual ~UCameraBarcodeReader() override;
	UTexture2D* GetFrameFromMaterial();

	// FTickableGameObject interface
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override;
    virtual bool IsTickableWhenPaused() const override;
    virtual bool IsTickableInEditor() const override;
    virtual UWorld* GetTickableGameObjectWorld() const override;
    virtual TStatId GetStatId() const override;
    // End FTickableGameObject interface
	
private:
	void InitializeDynamicMaterial();
	
	UMediaPlayer* MediaPlayer;
	UMediaTexture* MediaTexture;	
	FString DeviceUrl;
	FString DeviceDisplayName;
	UTexture2D* CalibrationTexture;	
	UMaterialInterface* BaseMaterial;
	UMaterialInterface* ViewFinderMaterial;
	UMaterialInstanceDynamic* DynamicMaterial;
	UTextureRenderTarget2D* RenderTarget;
	int32 TickCount;

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
