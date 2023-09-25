#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MediaAssets/Public/MediaTexture.h"

THIRD_PARTY_INCLUDES_START
#include "ThirdParty/ZXing/zxing-cpp/core/src/ReadBarcode.h"
THIRD_PARTY_INCLUDES_END

#include "ZXingUnreal.generated.h"

UENUM(BlueprintType)
enum class EZXingBarcodeFormat
{
	BF_None = 0 UMETA(DisplayName = "Used as a return value if no valid barcode has been detected"),
	BF_Aztec = (1 << 0) UMETA(DisplayName = "Aztec"),
	BF_Codabar = (1 << 1) UMETA(DisplayName = "Codabar"),
	BF_Code39 = (1 << 2) UMETA(DisplayName = "Code39"),
	BF_Code93 = (1 << 3) UMETA(DisplayName = "Code93"),
	BF_Code128 = (1 << 4) UMETA(DisplayName = "Code128"),
	BF_DataBar = (1 << 5) UMETA(DisplayName = "GS1 DataBar formerly known as RSS 14"),
	BF_DataBarExpanded = (1 << 6) UMETA(DisplayName = "GS1 DataBar Expanded formerly known as RSS EXPANDED"),
	BF_DataMatrix = (1 << 7) UMETA(DisplayName = "DataMatrix"),
	BF_EAN8 = (1 << 8) UMETA(DisplayName = "EAN-8"),
	BF_EAN13 = (1 << 9) UMETA(DisplayName = "EAN-13"),
	BF_ITF = (1 << 10) UMETA(DisplayName = "ITF (Interleaved Two of Five)"),
	BF_MaxiCode = (1 << 11) UMETA(DisplayName = "MaxiCode"),
	BF_PDF417 = (1 << 12) UMETA(DisplayName = "PDF417 or"),
	BF_QRCode = (1 << 13) UMETA(DisplayName = "QR Code"),
	BF_UPCA = (1 << 14) UMETA(DisplayName = "UPC-A"),
	BF_UPCE = (1 << 15) UMETA(DisplayName = "UPC-E"),
	BF_MicroQRCode = (1 << 16) UMETA(DisplayName = "Micro QR Code"),

	BF_LinearCodes = BF_Codabar | BF_Code39 | BF_Code93 | BF_Code128 | BF_EAN8 | BF_EAN13 | BF_ITF | BF_DataBar |
	BF_DataBarExpanded | BF_UPCA | BF_UPCE UMETA(DisplayName = "Linear Codes"),
	BF_MatrixCodes = BF_Aztec | BF_DataMatrix | BF_MaxiCode | BF_PDF417 | BF_QRCode | BF_MicroQRCode UMETA(
		DisplayName = "Matrix Codes"),
};

UENUM(BlueprintType)
enum class EZXingContentType
{
	Text UMETA(DisplayName = "Text"),
	Binary UMETA(DisplayName = "Binary"),
	Mixed UMETA(DisplayName = "Mixed"),
	GS1 UMETA(DisplayName = "GS1"),
	ISO15434 UMETA(DisplayName = "ISO15434"),
	UnknownECI UMETA(DisplaName = "Unknown ECI")
};

namespace ZXingUnreal
{
	class Position : public ZXing::Quadrilateral<FVector2D>
	{
		using Base = Quadrilateral;

	public:
		using Base::Base;
	};

	class Result : ZXing::Result
	{
	public:
		Result() = default;

		explicit Result(ZXing::Result&& r);

		using ZXing::Result::isValid;

		EZXingBarcodeFormat format() const;
		EZXingContentType contentType() const;
		FString formatName() const;
		const FString& text() const;
		const TArray<uint8_t>& bytes() const;
		const Position& position() const;

		// For debugging/development
		int runTime = 0;

	private:
		FString _text;
		TArray<uint8> _bytes;
		Position _position;
	};

	auto ImgFmtFromEPixelFormat(const EPixelFormat PixelFormat);
	auto ImgFmtFromUTexture2D(const UTexture2D* TextureIn);
	ZXing::ImageView ImageViewFromBuffer(EPixelFormat InPixelFormat, int32_t InWidth, int32_t InHeight, const uint8_t* InBuffer);
	ZXing::ImageView ImageViewFromTexture2D(const UTexture2D* Texture);
	ZXing::ImageView ImageViewFromTexture(const UTexture* Texture);
}

USTRUCT(BlueprintType)
struct FZXingQuadrilateral
{
	GENERATED_USTRUCT_BODY()

	FZXingQuadrilateral();

	FZXingQuadrilateral(
		const FVector2D TopLeftIn,
		const FVector2D TopRightIn,
		const FVector2D BottomRightIn,
		const FVector2D BottomLeftIn
	);

	FZXingQuadrilateral(const ZXingUnreal::Position& Position);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertices")
	FVector2D TopLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertices")
	FVector2D TopRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertices")
	FVector2D BottomRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vertices")
	FVector2D BottomLeft;
};

UCLASS()
class ZXINGUNREAL_API UZXingResult : public UObject
{
	GENERATED_BODY()

public:
	UZXingResult();

	void Initialize(ZXingUnreal::Result&& ResultIn);
	void Initialize(ZXing::Result&& ResultIn);

	UFUNCTION(BlueprintPure, Category = "Barcode")
	EZXingBarcodeFormat Format() const;

	UFUNCTION(BlueprintPure, Category = "Barcode")
	EZXingContentType ContentType() const;

	UFUNCTION(BlueprintPure, Category = "Barcode")
	FString FormatName() const;

	UFUNCTION(BlueprintPure, Category = "Barcode")
	FString Text() const;

	UFUNCTION(BlueprintPure, Category = "Barcode")
	FZXingQuadrilateral Position() const;

private:
	ZXingUnreal::Result Result;
};

UCLASS()
class ZXINGUNREAL_API UZXingBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static TArray<UZXingResult*> ReadBarcodes(UObject* Owner, const EPixelFormat Format, int32_t Width, int32_t Height,  const uint8_t* Buffer);
	
	static TArray<UZXingResult*> ReadBarcodes(UObject* Owner, const UTexture2D* Image);

	UFUNCTION(BlueprintCallable, Category="ZXing")
	static TArray<UZXingResult*> ReadBarcodes(UObject* Owner, const UTexture* Image);

private:
	static TArray<UZXingResult*> ResultsToArray(UObject* Owner, ZXing::Results&& ResultsIn);
};
