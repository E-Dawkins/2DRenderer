#pragma once
#include "DLLCommon.h"
#include "Color.h"
#include "BitReader.h"

#pragma warning(disable : 4251)
#include <vector>

class RENDERER_API PNGProperties
{
public:
	PNGProperties();

	void LoadPNG(const char* _filePath);

protected:
	// Chunk handlers

	void Chunk_IHDR(std::ifstream& _reader);
	void Chunk_PLTE(std::ifstream& _reader, unsigned int _chunkLength);
	void Chunk_IDAT(std::ifstream& _reader, unsigned int _chunkLength, std::vector<unsigned char>& _data);
	void Chunk_Ancillary(std::ifstream& _reader, unsigned int _chunkLength, char* _chunkType);

	// IDAT Flow

	void DecompressIDATData(std::vector<unsigned char>& _compressedData, std::vector<unsigned char>& _decompressedData);
	void UnfilterIDATData(std::vector<unsigned char>& _decompressedData);
	void ReadIDATData(std::vector<unsigned char>& _unfiltered);

	// Helpers

	void CheckSignature(std::ifstream& _reader);
	void CheckIHDRData();

	bool IsAncillaryChunk(const char* _chunkType);

	void GetScanlineVars(std::vector<unsigned short>& _scanlineLengths, std::vector<unsigned short>& _startingRows, unsigned int& _totalScanlines);

	Color GetNextPixel(BitReader& _br);

	void ApplyGamma(Color& _color);
	void CheckTRNS(Color& _color);

public:
	unsigned int width, height;

	char bitDepth, colourType, compressionMethod, 
		filterMethod, interlaceMethod;

	std::vector<Color> palette;
	std::vector<Color> pixels;

	Color trnsColor;
	float gamma;
};