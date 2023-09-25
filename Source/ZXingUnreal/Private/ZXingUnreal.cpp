#include "ZXingUnreal.h"

ZXingUnreal::Result::Result(ZXing::Result&& r) : ZXing::Result(std::move(r))
{
	_text = FString(ZXing::Result::text().c_str());
	_bytes = TArray<uint8_t>(ZXing::Result::bytes().data(), ZXing::Result::bytes().size());
	auto& pos = ZXing::Result::position();
	auto qp = [&pos](int i) { return FVector2D(pos[i].x, pos[i].y); };
	_position = {qp(0), qp(1), qp(2), qp(3)};
}

EZXingBarcodeFormat ZXingUnreal::Result::format() const
{
	return static_cast<EZXingBarcodeFormat>(ZXing::Result::format());
}

EZXingContentType ZXingUnreal::Result::contentType() const
{
	return static_cast<EZXingContentType>(ZXing::Result::contentType());
}

FString ZXingUnreal::Result::formatName() const { return FString(ZXing::ToString(ZXing::Result::format()).c_str()); }
const FString& ZXingUnreal::Result::text() const { return _text; }
const TArray<uint8_t>& ZXingUnreal::Result::bytes() const { return _bytes; }
const ZXingUnreal::Position& ZXingUnreal::Result::position() const { return _position; }

auto ZXingUnreal::ImgFmtFromEPixelFormat(const EPixelFormat PixelFormat)
{
	using ZXing::ImageFormat;

	switch (PixelFormat)
	{
	// Grayscale / Luminance
	case PF_G8:
	case PF_G16:
		return ImageFormat::Lum;

	// RGB Formats
	case PF_FloatRGB:
	case PF_FloatR11G11B10:
	case PF_R5G6B5_UNORM:
	case PF_R32G32B32_UINT:
	case PF_R32G32B32_SINT:
	case PF_R32G32B32F:
		return ImageFormat::RGB;

	// RGBA Formats
	case PF_FloatRGBA:
	case PF_R16G16B16A16_UINT:
	case PF_R16G16B16A16_SINT:
	case PF_R32G32B32A32_UINT:
	case PF_R8G8B8A8:
	case PF_R8G8B8A8_UINT:
	case PF_R8G8B8A8_SNORM:
	case PF_R16G16B16A16_UNORM:
	case PF_R16G16B16A16_SNORM:
		return ImageFormat::RGBX;
	case PF_A32B32G32R32F:
		return ImageFormat::XRGB;
	case PF_B8G8R8A8:
	case PF_B5G5R5A1_UNORM:
		return ImageFormat::BGRX;
	case PF_A2B10G10R10:
	case PF_A16B16G16R16:
	case PF_A8R8G8B8:
		return ImageFormat::XBGR;

	// Could not easily infer the mapping for other formats, so we will default to none
	default:
		return ImageFormat::None;
	}
}

auto ZXingUnreal::ImgFmtFromUTexture2D(const UTexture2D* TextureIn)
{
	return ImgFmtFromEPixelFormat(TextureIn->GetPixelFormat());
};

ZXing::ImageView ZXingUnreal::ImageViewFromBuffer(EPixelFormat InPixelFormat, int32_t InWidth, int32_t InHeight, const uint8_t* InBuffer)
{
	using ZXing::ImageFormat;
	ImageFormat Format;
	verify((Format = ImgFmtFromEPixelFormat(InPixelFormat)) != ImageFormat::None);

	return {InBuffer, InWidth, InHeight, Format};
}

ZXing::ImageView ZXingUnreal::ImageViewFromTexture2D(const UTexture2D* Texture)
{
	using ZXing::ImageFormat;

	// obtain all the pixel information from the mipmaps
	const FTexture2DMipMap* MipMap = &Texture->GetPlatformData()->Mips[0];
	const FByteBulkData* RawImageData = &MipMap->BulkData;

	// store in fcolor array
	const uint8* Pixels = static_cast<const uint8*>(RawImageData->LockReadOnly());
	RawImageData->Unlock();
	ImageFormat Format;
	verify((Format = ImgFmtFromUTexture2D(Texture)) != ImageFormat::None);

	return {Pixels, Texture->GetSizeX(), Texture->GetSizeY(), Format};
}

inline ZXing::ImageView ZXingUnreal::ImageViewFromTexture(const UTexture* Texture)
{
	const UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
	check(Texture2D);
	return ImageViewFromTexture2D(Texture2D);
}

FZXingQuadrilateral::FZXingQuadrilateral()
	: TopLeft(0, 0)
	  , TopRight(0, 0)
	  , BottomRight(0, 0)
	  , BottomLeft(0, 0)
{
}

FZXingQuadrilateral::FZXingQuadrilateral(
	const FVector2D TopLeftIn,
	const FVector2D TopRightIn,
	const FVector2D BottomRightIn,
	const FVector2D BottomLeftIn
)
	: TopLeft(TopLeftIn)
	  , TopRight(TopRightIn)
	  , BottomRight(BottomRightIn)
	  , BottomLeft(BottomLeftIn)
{
}

FZXingQuadrilateral::FZXingQuadrilateral(const ZXingUnreal::Position& Position)
	: TopLeft(Position.topLeft())
	  , TopRight(Position.topRight())
	  , BottomRight(Position.bottomRight())
	  , BottomLeft(Position.bottomLeft())
{
}

UZXingResult::UZXingResult()
{
};

void UZXingResult::Initialize(ZXingUnreal::Result&& ResultIn)
{
	Result = std::move(ResultIn);
}

void UZXingResult::Initialize(ZXing::Result&& ResultIn)
{
	Result = ZXingUnreal::Result(std::move(ResultIn));
}

EZXingBarcodeFormat UZXingResult::Format() const { return Result.format(); }

EZXingContentType UZXingResult::ContentType() const { return Result.contentType(); }

FString UZXingResult::FormatName() const { return Result.formatName(); }

FString UZXingResult::Text() const { return Result.text(); }

FZXingQuadrilateral UZXingResult::Position() const { return Result.position(); }


TArray<UZXingResult*> UZXingBlueprintFunctionLibrary::ReadBarcodes(UObject* Owner, const EPixelFormat Format, int32_t Width, int32_t Height,  const uint8_t* Buffer)
{
	// TODO: Implement this in the actual function (would require a wrapped class)
	const ZXing::DecodeHints& hints = {};

	return ResultsToArray(Owner, ZXing::ReadBarcodes(ZXingUnreal::ImageViewFromBuffer(Format, Width, Height, Buffer), hints));
}

TArray<UZXingResult*> UZXingBlueprintFunctionLibrary::ReadBarcodes(UObject* Owner, const UTexture2D* Image)
{
	// TODO: Implement this in the actual function (would require a wrapped class)
	const ZXing::DecodeHints& hints = {};

	return ResultsToArray(Owner, ZXing::ReadBarcodes(ZXingUnreal::ImageViewFromTexture2D(Image), hints));
}

TArray<UZXingResult*> UZXingBlueprintFunctionLibrary::ReadBarcodes(UObject* Owner, const UTexture* Image)
{
	// TODO: Implement this in the actual function (would require a wrapped class)
	const ZXing::DecodeHints& hints = {};

	return ResultsToArray(Owner, ZXing::ReadBarcodes(ZXingUnreal::ImageViewFromTexture(Image), hints));
}

TArray<UZXingResult*> UZXingBlueprintFunctionLibrary::ResultsToArray(UObject* Owner, ZXing::Results&& ResultsIn)
{
	TArray<UZXingResult*> ResultsOut;
	for (auto&& Result : ResultsIn)
	{
		UZXingResult* NewResult = NewObject<UZXingResult>(Owner);
		NewResult->Initialize(std::move(Result));
		ResultsOut.Add(NewResult);
	}
	return ResultsOut;
}
