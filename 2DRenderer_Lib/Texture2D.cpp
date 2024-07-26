#include "Texture2D.h"

#include <algorithm>
#include <fstream>
#include <iostream>

#pragma warning(disable : 6054)

Texture2D::Texture2D()
{
	mFilePath		= "";
	mFileName		= "";
	mFormat			= UNSUPPORTED;
	mPNGProps		= PNGProperties();
}

Texture2D::Texture2D(std::string _filePath)
{
	mFilePath = _filePath;
	mPNGProps = PNGProperties();

	SetFileName();
	SetFormat();

	try
	{
		if (IsValidFormat())
		{
			switch (mFormat)
			{
				case PNG:	LoadPNG();	break;
				case JPG:	LoadJPG();	break;
			}
		}
	}
	catch (const std::exception& _e)
	{
		std::cout << "!! Exception reading " << mFileName << " !!" << std::endl;
		std::cout << _e.what() << std::endl;
	}
}

Texture2D::Texture2D(const Texture2D& _tex)
{
	mFilePath		= _tex.mFilePath;
	mFileName		= _tex.mFileName;
	mFormat			= _tex.mFormat;
	mPNGProps		= _tex.mPNGProps;
}

void Texture2D::SetFileName()
{
	size_t nameStart = mFilePath.find_last_of('/');
	if (nameStart != mFilePath.npos && mFilePath.back() != '/')
	{
		mFileName = mFilePath.substr(nameStart + 1, mFilePath.length() - nameStart);
	}
	else mFileName = "";
}

void Texture2D::SetFormat()
{
	size_t extStart = mFileName.find_last_of('.');
	if (extStart == mFileName.npos || mFileName.back() == '.')
	{
		mFormat = UNSUPPORTED;
		return;
	}

	std::string extension = mFileName.substr(extStart + 1, mFileName.length() - extStart);
	std::transform(extension.begin(), extension.end(), extension.begin(),
		[](unsigned char c) { return std::tolower(c); });

	if (extension == "png")
	{
		mFormat = PNG;
	} 
	else if (extension == "jpg" || extension == "jpeg")
	{
		mFormat = JPG;
	} 
	else mFormat = UNSUPPORTED;
}

const bool Texture2D::IsValidFormat() const
{
	return !(mFormat == UNSUPPORTED || mFormat >= TOTAL_SUPPORTED_FORMATS);
}

void Texture2D::LoadPNG()
{
	mPNGProps.LoadPNG(mFilePath.c_str());
}

void Texture2D::LoadJPG()
{
	// TODO
}
