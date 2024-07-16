#pragma once
#include "DLLCommon.h"
#include "PNG.h"

#pragma warning(disable : 4251)
#include <string>

enum RENDERER_API FileFormat
{
	UNSUPPORTED = -1,
	PNG,
	JPG,

	TOTAL_SUPPORTED_FORMATS
};

class RENDERER_API Texture2D
{
public:
	Texture2D();
	Texture2D(std::string _filePath);
	Texture2D(const Texture2D& _tex);

protected:
	void SetFileName();
	void SetFormat();

	const bool IsValidFormat() const;

	void LoadPNG();
	void LoadJPG();

public:
	std::string mFilePath;
	std::string mFileName;

	FileFormat mFormat;
	PNGProperties mPNGProps;
};

