#pragma once

#include "CameraBarcodeReader.h"
#include "MediaCaptureSupport.h"
#include "MediaAssets/Public/MediaPlayer.h"
#include "CoreMinimal.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "UObject/Object.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/OverlaySlot.h"

UCameraBarcodeReader::UCameraBarcodeReader(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  TickCount(0)
{
	static ConstructorHelpers::FObjectFinder<UMaterial> MFMediaDisplay(
		TEXT("/Script/Engine.Material'/QrReader/M_MediaDisplay.M_MediaDisplay'"));

	static ConstructorHelpers::FObjectFinder<UMaterial> MFViewFinder(
		TEXT("/Script/Engine.Material'/QrReader/M_ViewFinder.M_ViewFinder'"));

	static ConstructorHelpers::FObjectFinder<UTexture2D> TextureFinder(
		TEXT("/Script/Engine.Texture2D'/Engine/EngineMaterials/DefaultCalibrationColor.DefaultCalibrationColor'"));

	
	if (MFMediaDisplay.Succeeded())
	{
		BaseMaterial = MFMediaDisplay.Object;
	}

	if (MFViewFinder.Succeeded())
	{
		ViewFinderMaterial = MFViewFinder.Object;
	}

	if (TextureFinder.Succeeded())
	{
		CalibrationTexture = TextureFinder.Object;
	}
}

void UCameraBarcodeReader::InitializeDynamicMaterial()
{
	if (BaseMaterial && !DynamicMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		RenderTarget = NewObject<UTextureRenderTarget2D>(this);

		if (MediaTexture)
		{
			DynamicMaterial->SetTextureParameterValue(FName("MediaTexture"), MediaTexture);

			if (Image)
			{
				Image->SetBrushFromMaterial(DynamicMaterial);
				auto AspectRatio = MediaPlayer->GetVideoTrackAspectRatio(0, 0);
				UE_LOG(LogTemp, Log, TEXT("AspectRatio: %f"), AspectRatio);
				UE_LOG(LogTemp, Log, TEXT("Image Size: %d x %d"), MediaTexture->GetWidth(), MediaTexture->GetHeight());
				Image->Brush.SetImageSize(FVector2D(MediaTexture->GetHeight(), MediaTexture->GetHeight() / AspectRatio));
				MediaTexture->UpdateResource();
			}
		}
	}
}

TSharedRef<SWidget> UCameraBarcodeReader::RebuildWidget()
{
	if (Overlay == nullptr)
	{
		Overlay = NewObject<UOverlay>(this);		
	}
	
	if (VideoScaleBox == nullptr)
	{
		VideoScaleBox = NewObject<UScaleBox>(this);
		VideoScaleBox->SetStretch(EStretch::ScaleToFill);		
	}

	if (VideoScaleBox->GetParent() != Overlay)
	{
		Overlay->AddChild(VideoScaleBox);
		if (const auto ScaleBoxSlot = Cast<UOverlaySlot>(VideoScaleBox->Slot))
		{
			ScaleBoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
			ScaleBoxSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);				
		}
	}

	if (Image == nullptr)
	{
		Image = NewObject<UImage>(this);		
	}
	
	if (Image->GetParent() != VideoScaleBox)
	{
		VideoScaleBox->AddChild(Image);
	}	

	if (ViewFinderImage == nullptr)
	{
		ViewFinderImage = NewObject<UImage>(this);		
		ViewFinderImage->SetBrushFromMaterial(ViewFinderMaterial);		
		ViewFinderImage->SetRenderScale(FVector2D(0.7, 0.7));
	}

	if (ViewFinderScaleBox == nullptr)
	{
		ViewFinderScaleBox = NewObject<UScaleBox>(this);
		ViewFinderScaleBox->SetStretch(EStretch::ScaleToFit);
	}

	if (ViewFinderScaleBox->GetParent() != Overlay)
	{
		Overlay->AddChild(ViewFinderScaleBox);
		if (const auto ViewFinderScaleBoxSlot = Cast<UOverlaySlot>(ViewFinderScaleBox->Slot))
		{
			ViewFinderScaleBoxSlot->SetHorizontalAlignment(HAlign_Fill);
			ViewFinderScaleBoxSlot->SetVerticalAlignment(VAlign_Fill);				
		}
	}
	
	if (ViewFinderImage->GetParent() != ViewFinderScaleBox)
	{
		ViewFinderScaleBox->AddChild(ViewFinderImage);
	}	

	if (IsDesignTime())
	{	
		if (CalibrationTexture && Image->GetBrush().GetResourceObject() != CalibrationTexture)
		{
			Image->SetBrushFromTexture(CalibrationTexture);
		}
		
		return Overlay->TakeWidget();
	}

	if (MediaTexture == nullptr && BaseMaterial)
	{
		MediaTexture = NewObject<UMediaTexture>(this);
		MediaTexture->AddressX = TA_Clamp;
		MediaTexture->AddressY = TA_Clamp;
		MediaTexture->AutoClear = true;
		MediaTexture->ClearColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);
		MediaTexture->UpdateResource();

		TArray<FMediaCaptureDeviceInfo> AvailableDevices;
		MediaCaptureSupport::EnumerateVideoCaptureDevices(AvailableDevices);
		
		for (const FMediaCaptureDeviceInfo& Device : AvailableDevices)
		{
			if (
				(!CameraOverride.IsEmpty() && CameraOverride == Device.DisplayName.ToString()) ||
				(CameraOverride.IsEmpty() && Device.Type == EMediaCaptureDeviceType::WebcamRear)
			)
			{
				if (DeviceUrl != Device.Url || DeviceDisplayName != Device.DisplayName.ToString())
				{
					UE_LOG(LogTemp, Log, TEXT("Device Name: %s, URL: %s"), *Device.DisplayName.ToString(), *Device.Url);
					DeviceUrl = Device.Url;
					DeviceDisplayName = Device.DisplayName.ToString();

					MediaPlayer = NewObject<UMediaPlayer>(this);
					MediaPlayer->OpenUrl(DeviceUrl);
					MediaPlayer->PlayOnOpen = true;
					MediaPlayer->OnMediaOpened.AddDynamic(this, &UCameraBarcodeReader::CatchMediaOpened);
					MediaPlayer->OnMediaOpenFailed.AddDynamic(this, &UCameraBarcodeReader::CatchMediaOpenFailed);
					MediaPlayer->OnEndReached.AddDynamic(this, &UCameraBarcodeReader::CatchEndReached);
					MediaPlayer->OnPlaybackSuspended.AddDynamic(this, &UCameraBarcodeReader::CatchPlaybackSuspended);
				}
				break;
			}
		}
	}

	return Overlay->TakeWidget();
}

void UCameraBarcodeReader::CatchMediaOpened(FString OpenedUrl)
{
	UE_LOG(LogTemp, Warning, TEXT("MediaPlayer opened: %s"), *OpenedUrl);
	MediaTexture->SetMediaPlayer(MediaPlayer);
	MediaTexture->UpdateResource();

	InitializeDynamicMaterial();

	if (!MediaPlayer->IsPlaying())
	{
		MediaPlayer->Play();		
	}
}

void UCameraBarcodeReader::ProcessFrameInBackground()
{
	auto Frame = GetFrameFromMaterial();
	
	const FTexture2DMipMap* MipMap = &Frame->GetPlatformData()->Mips[0];
	const FByteBulkData* RawImageData = &MipMap->BulkData;
	
	EPixelFormat Format = Frame->GetPixelFormat();
	int32_t Width = Frame->GetSizeX();
	int32_t Height = Frame->GetSizeY();
	const uint8_t* Buffer = static_cast<const uint8*>(RawImageData->LockReadOnly());
	
	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, Format, Width, Height, Buffer, RawImageData]()
	{
		auto Result = UZXingBlueprintFunctionLibrary::ReadBarcodes(this, Format, Width, Height, Buffer);
		AsyncTask(ENamedThreads::GameThread, [this, Result, RawImageData]()
		{
			RawImageData->Unlock();
			if (Result.Num() > 0)
			{
				OnBarcodeRead.Broadcast(Result);
			}
		});
	});
}

void UCameraBarcodeReader::CatchMediaOpenFailed(FString FailedUrl)
{
	UE_LOG(LogTemp, Error, TEXT("MediaPlayer failed to open: %s"), *FailedUrl);
}

void UCameraBarcodeReader::CatchEndReached()
{
	UE_LOG(LogTemp, Warning, TEXT("MediaPlayer end reached"));
}

void UCameraBarcodeReader::CatchPlaybackSuspended()
{
	UE_LOG(LogTemp, Warning, TEXT("MediaPlayer playback suspended"));
}

UCameraBarcodeReader::~UCameraBarcodeReader()
{
	if (MediaPlayer != nullptr)
	{
		MediaPlayer->Close();
	}
}

UTexture2D* UCameraBarcodeReader::GetFrameFromMaterial()
{
	RenderTarget->InitAutoFormat(MediaTexture->GetSurfaceWidth(), MediaTexture->GetSurfaceHeight());
	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	FCanvas Canvas(RenderTargetResource, nullptr, GetWorld(), ERHIFeatureLevel::SM6);
	FCanvasTileItem TileItem(FVector2D(0, 0), DynamicMaterial->GetRenderProxy(),
	                         FVector2D(MediaTexture->GetSurfaceWidth(), MediaTexture->GetSurfaceHeight()));
	TileItem.Size = FVector2D(RenderTarget->SizeX, RenderTarget->SizeY);
	Canvas.DrawItem(TileItem);
	Canvas.Flush_GameThread();

	TArray<FColor> OutData;
	RenderTargetResource->ReadPixels(OutData);

	UTexture2D* NewTexture = UTexture2D::CreateTransient(MediaTexture->GetSurfaceWidth(),
	                                                     MediaTexture->GetSurfaceHeight());
	NewTexture->NeverStream = true;
	FTexture2DMipMap& Mip = NewTexture->GetPlatformData()->Mips[0];

	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, OutData.GetData(), OutData.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();

	NewTexture->UpdateResource();
	return NewTexture;
}

void UCameraBarcodeReader::Tick(float DeltaTime)
{
	if (TickCount >= 100)
	{
		if (MediaPlayer && MediaPlayer->IsPlaying() && MediaTexture && MediaTexture->GetResource())
		{
			ProcessFrameInBackground();
		}
		TickCount = 0;
	}

	TickCount++;
}

bool UCameraBarcodeReader::IsTickable() const
{
	return true;
}

bool UCameraBarcodeReader::IsTickableWhenPaused() const
{
	return false;
}

bool UCameraBarcodeReader::IsTickableInEditor() const
{
	return false;
}

UWorld* UCameraBarcodeReader::GetTickableGameObjectWorld() const
{
	return GetWorld();
}

TStatId UCameraBarcodeReader::GetStatId() const
{
	return UObject::GetStatID();
}
