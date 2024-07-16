#pragma once
#include "DLLCommon.h"
#include "Color.h"

#pragma warning(disable : 4251)
#include <vector>

class RENDERER_API PNGProperties
{
public:
	PNGProperties();

	void LoadPNG(const char* _filePath);

protected:
	bool CheckSignature(std::ifstream& _reader);
	void Chunk_IHDR(std::ifstream& _reader);
	void Chunk_PLTE(std::ifstream& _reader, unsigned int _chunkLength);
	void Chunk_IDAT(std::ifstream& _reader, unsigned int _chunkLength);
	void Chunk_Ancillary(std::ifstream& _reader, unsigned int _chunkLength);

	// Assumes _chunkType is exactly 4-bytes.
	bool IsAncillaryChunk(const char* _chunkType);

	unsigned char Previous(unsigned int _byteIndex, unsigned int _scanlineLength, unsigned char* _pixelBuffer);
	unsigned char Prior(unsigned int _byteIndex, unsigned int _scanlineLength, unsigned char* _pixelBuffer);
	unsigned char PrevPrior(unsigned int _byteIndex, unsigned int _scanlineLength, unsigned char* _pixelBuffer);
	unsigned char PaethPredictor(char _left, char _up, char _upLeft);

public:
	unsigned int width, height;

	char bitDepth, colourType, compressionMethod, 
		filterMethod, interlaceMethod;

	std::vector<Color> palette;
	Color* pixels;
};