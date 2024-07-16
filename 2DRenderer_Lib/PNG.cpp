#include "PNG.h"

#include "ByteHelpers.h"
#include "gzguts.h" // TODO - replace with own decompressor?

PNGProperties::PNGProperties()
{
	width = 0;
	height = 0;
	bitDepth = 0;
	colourType = 0;
	compressionMethod = 0;
	filterMethod = 0;
	interlaceMethod = 0;
	palette = {};
	pixels = nullptr;
}

void PNGProperties::LoadPNG(const char* _filePath)
{
	std::ifstream reader = std::ifstream();
	reader.open(_filePath, std::ios::in | std::ios::binary);
	bool reading = true;

	if (!reader.is_open())
		return;

	if (!CheckSignature(reader))
		reading = false;

	while (reading)
	{
		unsigned int chunkLength = R2D_BH::ReadBytesIntoUInt(reader, 4);

		char chunkType[4];
		reader.read(chunkType, 4);

		if (R2D_BH::CompCharArrToStr(chunkType, "IHDR", 4))
		{
			Chunk_IHDR(reader);
		}
		else if (R2D_BH::CompCharArrToStr(chunkType, "PLTE", 4))
		{
			Chunk_PLTE(reader, chunkLength);
		}
		else if (R2D_BH::CompCharArrToStr(chunkType, "IDAT", 4))
		{
			Chunk_IDAT(reader, chunkLength);
		}
		else if (R2D_BH::CompCharArrToStr(chunkType, "IEND", 4))
		{
			reading = false;
		}
		else if (IsAncillaryChunk(chunkType))
		{
			Chunk_Ancillary(reader, chunkLength);
		}
		else // found unknown critical chunk type, early exit reading
		{
			reading = false;
		}

		// CRC is present at the end of every chunk, even empty ones
		char crc[4];
		reader.read(crc, 4);
	}

	reader.close();
}

bool PNGProperties::CheckSignature(std::ifstream& _reader)
{
	constexpr char pngSignature[8] = { -119, 80, 78, 71, 13, 10, 26, 10 };
	const char* pngSig = R2D_BH::ReadBytesIntoStr(_reader, 8);

	if (!R2D_BH::CompCharArrToStr(pngSig, pngSignature, 8))
	{
		perror("PNG file signature does not match standard spec!");
		return false;
	}

	return true;
}

void PNGProperties::Chunk_IHDR(std::ifstream& _reader)
{
	width = R2D_BH::ReadBytesIntoUInt(_reader, 4);
	height = R2D_BH::ReadBytesIntoUInt(_reader, 4);

	_reader.read(&bitDepth, 1);
	_reader.read(&colourType, 1);
	_reader.read(&compressionMethod, 1);
	_reader.read(&filterMethod, 1);
	_reader.read(&interlaceMethod, 1);
}

void PNGProperties::Chunk_PLTE(std::ifstream& _reader, unsigned int _chunkLength)
{
	// Check for grayscale colour types
	if (colourType == 0 || colourType == 4)
	{
		perror("Can not use colour palette for grayscale PNG files!");
		return;
	}

	// Check that the palette is a valid byte-length (3-bytes divisible)
	if (_chunkLength % 3 != 0)
	{
		perror("Palette PNG chunk is not a valid byte-size!");
		return;
	}

	for (unsigned int i = 0; i < _chunkLength / 3; i++)
	{
		unsigned char r, g, b;

		_reader.read((char*)&r, 1);
		_reader.read((char*)&g, 1);
		_reader.read((char*)&b, 1);

		palette.push_back(Color(r / 255.f, g / 255.f, b / 255.f, 1));
	}
}
#include <iostream>
void PNGProperties::Chunk_IDAT(std::ifstream& _reader, unsigned int _chunkLength)
{
	char* data = new char[_chunkLength];
	_reader.read(data, _chunkLength);

	constexpr char channels[7] = { 1, 0, 3, 1, 2, 0, 4 };

	unsigned int bitsPerSample = bitDepth * channels[colourType];
	unsigned int decompBytes = (unsigned int)((width * height) / (8.0 / bitsPerSample)) + height;

	unsigned int scanlineLength = (decompBytes / width); // includes filter byte

	unsigned char* imageData = new unsigned char[decompBytes];

	z_stream infStream = {};
	infStream.zalloc = Z_NULL;
	infStream.zfree = Z_NULL;
	infStream.opaque = Z_NULL;
	inflateInit(&infStream);

	infStream.avail_in = _chunkLength;
	infStream.next_in = (Bytef*)data;

	infStream.avail_out = decompBytes;
	infStream.next_out = (Bytef*)imageData;

	int response = inflate(&infStream, Z_NO_FLUSH);
	inflateEnd(&infStream);

	for (unsigned int i = 0; i < height; i++)
	{
		unsigned int scanlineNum = i * scanlineLength;
		char filterMethod = imageData[scanlineNum];

		if (filterMethod == 0) continue; // no filter method, skip this scanline

		for (unsigned int j = 0; j < scanlineLength - 1; j++)
		{
			unsigned int byteIndex = scanlineNum + j + 1;

			if (byteIndex >= decompBytes) continue;

			unsigned char byte = imageData[byteIndex];

			switch (filterMethod) // SUB, UP, AVERAGE, PAETH
			{
				case 1: imageData[byteIndex] = byte + Previous(byteIndex, scanlineLength, imageData); break;
				case 2: imageData[byteIndex] = byte + Prior(byteIndex, scanlineLength, imageData); break;
				case 3: imageData[byteIndex] = byte + (unsigned char)floor((Previous(byteIndex, scanlineLength, imageData) + Prior(byteIndex, scanlineLength, imageData)) / 2); break;
				case 4: imageData[byteIndex] = byte + PaethPredictor(Previous(byteIndex, scanlineLength, imageData), Prior(byteIndex, scanlineLength, imageData), PrevPrior(byteIndex, scanlineLength, imageData)); break;
			}
		}
	}

	unsigned int samplesPerByte = 8 / bitDepth; // this will currently skip 16-bit encoded PNG's
	unsigned int bitMask = (1 << bitDepth) - 1;
	float sampleMax = powf(2.f, (float)bitDepth) - 1.f;

	pixels = new Color[width * height];
	unsigned int pixelIndex = 0;

	for (unsigned int i = 0; i < decompBytes - height; i++) // loop over all 'pixels'
	{
		unsigned int scanlineNum = i / (scanlineLength - 1);
		unsigned int byteIndex = i + scanlineNum + 1;

		if (byteIndex >= decompBytes) continue;

		unsigned char byte = imageData[byteIndex];

		for (unsigned int j = 0; j < samplesPerByte; j++, pixelIndex++) // loop over all samples within each byte
		{
			switch (colourType)
			{
				case 0: // grayscale
				{
					float grayscaleVal = (float)(byte & bitMask);
					pixels[pixelIndex] = Color(grayscaleVal, grayscaleVal, grayscaleVal, 1, sampleMax);

					break;
				}
				
				case 2: // RGB
				{
					float r = byte;
					float g = imageData[byteIndex + 1];
					float b = imageData[byteIndex + 2];

					pixels[pixelIndex] = Color(r, g, b, 1, sampleMax);
					i += 2;

					break;
				}

				case 3: // indexed
				{
					unsigned int paletteIndex = (byte >> j) & bitMask;
					pixels[pixelIndex] = palette[paletteIndex];

					break;
				}

				case 4: // grayscale + alpha
				{
					float gray = byte;
					float alpha = imageData[byteIndex + 1];

					pixels[pixelIndex] = Color(gray, gray, gray, alpha, sampleMax);
					i += 1;

					break;
				}

				case 6: // RGB + alpha
				{
					float r = byte;
					float g = imageData[byteIndex + 1];
					float b = imageData[byteIndex + 2];
					float a = imageData[byteIndex + 3];

					pixels[pixelIndex] = Color(r, g, b, a, sampleMax);
					i += 3;

					break;
				}
			}
		}
	}
}

void PNGProperties::Chunk_Ancillary(std::ifstream& _reader, unsigned int _chunkLength)
{
	_reader.seekg(_reader.tellg() + (std::streampos)_chunkLength);
}

bool PNGProperties::IsAncillaryChunk(const char* _chunkType)
{
	return (_chunkType[0] & 0b100000) != 0;
}

unsigned char PNGProperties::Previous(unsigned int _byteIndex, unsigned int _scanlineLength, unsigned char* _pixelBuffer)
{
	int prevIndex = _byteIndex - 1;
	return ((prevIndex % _scanlineLength == 0) ? 0 : _pixelBuffer[prevIndex]);
}

unsigned char PNGProperties::Prior(unsigned int _byteIndex, unsigned int _scanlineLength, unsigned char* _pixelBuffer)
{
	int priorIndex = _byteIndex - _scanlineLength;
	return (priorIndex < 0 ? 0 : _pixelBuffer[priorIndex]);
}

unsigned char PNGProperties::PrevPrior(unsigned int _byteIndex, unsigned int _scanlineLength, unsigned char* _pixelBuffer)
{
	int priorIndex = _byteIndex - _scanlineLength;
	if (priorIndex < 0) return 0;

	int prevIndex = priorIndex - 1;
	return ((prevIndex % _scanlineLength == 0) ? 0 : _pixelBuffer[prevIndex]);
}

unsigned char PNGProperties::PaethPredictor(char _left, char _up, char _upLeft)
{
	char p = _left + _up - _upLeft;		// initial estimate
	char pa = abs(p - _left);			// distances to a, b, c
	char pb = abs(p - _up);
	char pc = abs(p - _upLeft);

	// Return minimum distance
	if (pa <= pb && pa <= pc) return _left;
	else if (pb <= pc) return _up;
	return _upLeft;
}
