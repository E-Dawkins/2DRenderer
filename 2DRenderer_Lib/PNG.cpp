#include "PNG.h"

#include "ByteHelpers.h"
#include <gzguts.h> // TODO - replace with own decompressor?
#include "BitReader.h"

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
	pixels = {};
	backgroundColor = Color(0, 0, 0, 0);
	trnsColor = Color(-1, -1, -1, -1, 1, false);
	gamma = 1.f;
}

void PNGProperties::LoadPNG(const char* _filePath)
{
	std::ifstream reader = std::ifstream();
	reader.open(_filePath, std::ios::in | std::ios::binary);

	if (!reader.is_open())
		return;

	if (!CheckSignature(reader))
	{
		reader.close();
		return;
	}

	bool reading = true;
	std::vector<unsigned char> rawIDATData;

	while (reading)
	{
		unsigned int chunkLength = R2D_BH::ReadBytesIntoUInt(reader, 4);
		char* chunkType = R2D_BH::ReadBytesIntoStr(reader, 4);

		// We've read all IDAT chunks, now we need to decode the data
		if (rawIDATData.size() != 0 && !R2D_BH::CompCharArrToStr(chunkType, "IDAT", 4))
		{
			DecodeIDATData(rawIDATData);
		}

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
			Chunk_IDAT(reader, chunkLength, rawIDATData);
		}
		else if (R2D_BH::CompCharArrToStr(chunkType, "IEND", 4))
		{
			reading = false;
		}
		else if (IsAncillaryChunk(chunkType))
		{
			Chunk_Ancillary(reader, chunkLength, chunkType);
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
		unsigned char r = 0, g = 0, b = 0;

		_reader.read((char*)&r, 1);
		_reader.read((char*)&g, 1);
		_reader.read((char*)&b, 1);

		palette.push_back(Color(r, g, b, 255, 255));
	}
}

void PNGProperties::Chunk_IDAT(std::ifstream& _reader, unsigned int _chunkLength, std::vector<unsigned char>& _data)
{
	unsigned char* data = new unsigned char[_chunkLength];
	_reader.read((char*)data, _chunkLength);

	_data.insert(_data.end(), data, data + _chunkLength);
}

void PNGProperties::Chunk_Ancillary(std::ifstream& _reader, unsigned int _chunkLength, char* _chunkType)
{
	if (R2D_BH::CompCharArrToStr(_chunkType, "sRGB", 4))
	{
		unsigned char colourSpace = '\0';
		_reader.read((char*)&colourSpace, 1);
	}
	else if (R2D_BH::CompCharArrToStr(_chunkType, "gAMA", 4))
	{
		std::uint32_t gammaRaw = 0;
		_reader.read((char*)&gammaRaw, 4);

		gamma = (float)R2D_BH::ToBigEndian(gammaRaw) / 100000.f;
	}
	else if (R2D_BH::CompCharArrToStr(_chunkType, "pHYs", 4))
	{
		std::uint32_t ppuX = 0, ppuY = 0;
		unsigned char unitSpecifier = '\0';

		_reader.read((char*)&ppuX, 4);
		_reader.read((char*)&ppuY, 4);
		_reader.read((char*)&unitSpecifier, 1);
	}
	else if (R2D_BH::CompCharArrToStr(_chunkType, "bKGD", 4))
	{
		float sampleMax = powf(2.f, (float)bitDepth) - 1.f;

		switch (colourType)
		{
			case 3: // indexed
			{
				unsigned char paletteIndex;
				_reader.read((char*)&paletteIndex, 1);

				backgroundColor = palette[paletteIndex];

				break;
			}

			case 0: // grayscale, with or without alpha
			case 4:
			{
				unsigned short g;
				_reader.read((char*)&g, 2);

				backgroundColor = Color(g, g, g, sampleMax, sampleMax);

				break;
			}

			case 2: // rgb, with or without alpha
			case 6:
			{
				unsigned short r, g, b;
				_reader.read((char*)&r, 2);
				_reader.read((char*)&g, 2);
				_reader.read((char*)&b, 2);

				backgroundColor = Color(r, g, b, sampleMax, sampleMax);

				break;
			}
		}
	}
	else if (R2D_BH::CompCharArrToStr(_chunkType, "tRNS", 4))
	{
		float sampleMax = powf(2.f, (float)bitDepth) - 1.f;

		switch (colourType)
		{
			case 0: // grayscale
			{
				unsigned short g;
				_reader.read((char*)&g, 2);

				trnsColor = Color(g, g, g, -sampleMax, sampleMax);

				break;
			}

			case 2: // rgb
			{
				unsigned short r, g, b;
				_reader.read((char*)&r, 2);
				_reader.read((char*)&g, 2);
				_reader.read((char*)&b, 2);

				trnsColor = Color(r, g, b, -sampleMax, sampleMax);

				break;
			}

			case 3: // indexed
			{
				for (unsigned int i = 0; i < _chunkLength; i++)
				{
					unsigned char alpha;
					_reader.read((char*)&alpha, 1);

					palette[i].a = (float)alpha / 255.f;
				}

				break;
			}

			case 4: // these colour types already contain full alpha channels
			case 6:
			{
				perror("tRNS chunk in PNG file is prohibited for current colour type!");

				break;
			}
		}
	}
	else
	{
		_reader.seekg(_reader.tellg() + (std::streampos)_chunkLength);
	}
}

bool PNGProperties::IsAncillaryChunk(const char* _chunkType)
{
	return (_chunkType[0] & 0b100000) != 0;
}

void PNGProperties::DecodeIDATData(std::vector<unsigned char>& _rawIDATData)
{
	std::vector<unsigned char> decodedIDATData;

	DecompressIDATData(_rawIDATData, decodedIDATData);
	UnfilterIDATData(decodedIDATData);
	ReadIDATData(decodedIDATData);
}

void PNGProperties::DecompressIDATData(std::vector<unsigned char>& _compressedData, std::vector<unsigned char>& _decompressedData)
{
	const size_t CHUNK_SIZE = 4096;
	unsigned char out[CHUNK_SIZE] = {};

	z_stream strm = {};
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = (unsigned int)_compressedData.size();
	strm.next_in = (Bytef*)_compressedData.data();

	if (inflateInit(&strm) != Z_OK) {
		throw std::runtime_error("Failed to initialize inflate");
	}

	int result;
	do 
	{
		strm.avail_out = CHUNK_SIZE;
		strm.next_out = (Bytef*)out;
		result = inflate(&strm, Z_NO_FLUSH);

		if (result == Z_STREAM_ERROR) 
		{
			inflateEnd(&strm);
			perror("ZLib stream error during inflation of IDAT PNG data!");
			return;
		}

		size_t have = CHUNK_SIZE - strm.avail_out;
		_decompressedData.insert(_decompressedData.end(), out, out + have);

	} while (result != Z_STREAM_END);

	inflateEnd(&strm);

	if (result != Z_STREAM_END)
	{
		perror("Failed to decompress all IDAT PNG data!");
		return;
	}
}

void PNGProperties::UnfilterIDATData(std::vector<unsigned char>& _decompressedData)
{
	constexpr char channels[7] = { 1, 0, 3, 1, 2, 0, 4 };

	double bytesPerPixel = (double)(bitDepth * channels[colourType]) / 8.0;
	double stride = (double)width * bytesPerPixel;

	unsigned int offsetBytesPerPixel = (unsigned int)std::fmax(bytesPerPixel, 1); // minimum 1 byte offset

	for (unsigned int _scanlineNum = 0, i = 0; _scanlineNum < height; _scanlineNum++)
	{
		char filterMethod = _decompressedData[i];
		_decompressedData.erase(_decompressedData.begin() + i); // remove filter byte from data, not needed after this point

		for (unsigned int _posInScanline = 0; _posInScanline < stride; _posInScanline++)
		{
			unsigned short byte = (unsigned short)_decompressedData[i];

			switch (filterMethod)
			{
				case 0: break;
				case 1: byte += Previous(_scanlineNum, (unsigned int)stride, _posInScanline, offsetBytesPerPixel, _decompressedData.data()); break;
				case 2: byte += Prior(_scanlineNum, (unsigned int)stride, _posInScanline, _decompressedData.data()); break;
				case 3: byte += Average(_scanlineNum, (unsigned int)stride, _posInScanline, offsetBytesPerPixel, _decompressedData.data()); break;
				case 4: byte += Paeth(_scanlineNum, (unsigned int)stride, _posInScanline, offsetBytesPerPixel, _decompressedData.data()); break;
			}

			_decompressedData[i++] = byte % 256; // keep in byte format
		}
	}
}

void PNGProperties::ReadIDATData(std::vector<unsigned char>& _unfiltered)
{
	BitReader br = BitReader(_unfiltered.data());
	pixels.resize((size_t)(width * height));
	float sampleMax = powf(2.f, (float)bitDepth) - 1.f;

	for (unsigned int i = 0; i < width * height; i++) // fill the pixel data
	{
		if (i != 0 && i % width == 0) // this is a failsafe if a scanline doesn't end on a byte boundary, i.e. 5 pixels, 2 bit-depth
		{
			br.NextByte();
		}

		switch (colourType)
		{
			case 0: // grayscale
			{
				float g = (float)br.ReadBits(bitDepth);

				pixels[i] = Color(g, g, g, sampleMax, sampleMax);
				break;
			}

			case 2: // RGB
			{
				float r = (float)br.ReadBits(bitDepth);
				float g = (float)br.ReadBits(bitDepth);
				float b = (float)br.ReadBits(bitDepth);

				pixels[i] = Color(r, g, b, sampleMax, sampleMax);
				break;
			}

			case 3: // indexed
			{
				unsigned int paletteIndex = (unsigned int)br.ReadBits(bitDepth);

				pixels[i] = palette[paletteIndex];
				break;
			}

			case 4: // grayscale + alpha
			{
				float g = (float)br.ReadBits(bitDepth);
				float a = (float)br.ReadBits(bitDepth);

				pixels[i] = Color(g, g, g, a, sampleMax);
				break;
			}

			case 6: // RGB + alpha
			{
				float r = (float)br.ReadBits(bitDepth);
				float g = (float)br.ReadBits(bitDepth);
				float b = (float)br.ReadBits(bitDepth);
				float a = (float)br.ReadBits(bitDepth);

				pixels[i] = Color(r, g, b, a, sampleMax);
				break;
			}
		}

		CheckTRNS(pixels[i]);
	}
}

unsigned char PNGProperties::Previous(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned int _bytesPerPixel, unsigned char* _byteBuffer)
{
	if (_posInScanline >= _bytesPerPixel)
	{
		unsigned int index = _scanlineNum * _stride + _posInScanline - _bytesPerPixel;
		return _byteBuffer[index];
	}

	return 0;
}

unsigned char PNGProperties::Prior(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned char* _byteBuffer)
{
	if (_scanlineNum > 0)
	{
		unsigned int index = (_scanlineNum - 1) * _stride + _posInScanline;
		return _byteBuffer[index];
	}

	return 0;
}

unsigned char PNGProperties::Average(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned int _bytesPerPixel, unsigned char* _byteBuffer)
{
	unsigned char left = Previous(_scanlineNum, _stride, _posInScanline, _bytesPerPixel, _byteBuffer);
	unsigned char up = Prior(_scanlineNum, _stride, _posInScanline, _byteBuffer);

	return (unsigned char)floor((left + up) / 2.0);
}

unsigned char PNGProperties::PrevPrior(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned int _bytesPerPixel, unsigned char* _byteBuffer)
{
	if (_scanlineNum > 0 && _posInScanline >= _bytesPerPixel)
	{
		unsigned int index = (_scanlineNum - 1) * _stride + _posInScanline - _bytesPerPixel;
		return _byteBuffer[index];
	}

	return 0;
}

unsigned char PNGProperties::Paeth(unsigned int _scanlineNum, unsigned int _stride, unsigned int _posInScanline, unsigned int _bytesPerPixel, unsigned char* _byteBuffer)
{
	unsigned char left = Previous(_scanlineNum, _stride, _posInScanline, _bytesPerPixel, _byteBuffer);
	unsigned char up = Prior(_scanlineNum, _stride, _posInScanline, _byteBuffer);
	unsigned char upLeft = PrevPrior(_scanlineNum, _stride, _posInScanline, _bytesPerPixel, _byteBuffer);

	return PaethPredictor(left, up, upLeft);
}

unsigned char PNGProperties::PaethPredictor(unsigned char _left, unsigned char _up, unsigned char _upLeft)
{
	unsigned short p = (unsigned short)std::fmax(_left + _up - _upLeft, 0);		// initial estimate
	unsigned short pa = abs(p - _left);				// distances to a, b, c
	unsigned short pb = abs(p - _up);
	unsigned short pc = abs(p - _upLeft);

	// Return minimum distance
	if (pa <= pb && pa <= pc) return _left;
	else if (pb <= pc) return _up;
	return _upLeft;
}

void PNGProperties::CheckTRNS(Color& _color)
{
	switch (colourType)
	{
		case 0: // grayscale
		{
			if (_color.r == trnsColor.r)
			{
				_color.a = 0;
			}

			break;
		}

		case 2: // rgb
		{
			if (Color::EqualRGB(_color, trnsColor))
			{
				_color.a = 0;
			}

			break;
		}
	}
}
