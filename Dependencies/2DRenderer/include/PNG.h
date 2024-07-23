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
	void Chunk_IDAT(std::ifstream& _reader, unsigned int _chunkLength, std::vector<unsigned char>& _data);
	void Chunk_Ancillary(std::ifstream& _reader, unsigned int _chunkLength, char* _chunkType);

	// Assumes _chunkType is exactly 4-bytes.
	bool IsAncillaryChunk(const char* _chunkType);

	void DecodeIDATData(std::vector<unsigned char>& _rawIDATData);

	// Decompresses concatenated IDAT data into an existing vector.
	void DecompressIDATData(std::vector<unsigned char>& _compressedData, std::vector<unsigned char>& _decompressedData);

	// Un-filters already decompressed IDAT data.
	void UnfilterIDATData(std::vector<unsigned char>& _decompressedData);

	// Reads decompressed, un-filtered IDAT data into desired pixel format.
	void ReadIDATData(std::vector<unsigned char>& _unfiltered);

	unsigned char Previous(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned int _bytesPerPixel, unsigned char* _byteBuffer);
	unsigned char Prior(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned char* _byteBuffer);
	unsigned char PrevPrior(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned int _bytesPerPixel, unsigned char* _byteBuffer);
	unsigned char Paeth(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned int _bytesPerPixel, unsigned char* _byteBuffer);
	unsigned char PaethPredictor(unsigned char _left, unsigned char _up, unsigned char _upLeft);

	// Any pixel that matches the TRNS color should be treated as fully transparent.
	void CheckTRNS(Color& _color);

public:
	unsigned int width, height;

	char bitDepth, colourType, compressionMethod, 
		filterMethod, interlaceMethod;

	std::vector<Color> palette;
	std::vector<Color> pixels;

	Color backgroundColor;
	Color trnsColor;
};