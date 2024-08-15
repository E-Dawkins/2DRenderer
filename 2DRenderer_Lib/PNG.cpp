#include "PNG.h"

#include "ByteHelpers.h"

#include <gzguts.h> // TODO - replace with own decompressor?
#include <string>

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
	trnsColor = Color(-1, -1, -1, -1, 1, false);
	gamma = 1.f;
}

void PNGProperties::LoadPNG(const char* _filePath)
{
	std::ifstream reader = std::ifstream();
	reader.open(_filePath, std::ios::in | std::ios::binary);

	if (!reader.is_open())
	{
		throw std::runtime_error(std::string("Could not open file at: \"") + _filePath + std::string("\""));
	}

	CheckSignature(reader);

	bool reading = true;
	std::vector<unsigned char> rawIDATData;

	while (reading)
	{
		unsigned int chunkLength = R2D_BH::ReadBytesIntoUInt(reader, 4);

		// Compute the checksum for this chunk, including chunk type + data
		uLong crc = crc32(0L, Z_NULL, 0);
		crc = crc32(crc, (Bytef*)R2D_BH::ReadBytesIntoStr(reader, chunkLength + 4), chunkLength + 4);

		// Re-wind the readers' position to correctly read in the chunk type + data
		reader.seekg(-(int)(chunkLength + 4), std::ios_base::cur);
		char* chunkType = R2D_BH::ReadBytesIntoStr(reader, 4);

		// We've read all IDAT chunks, now we need to decode the data
		if (rawIDATData.size() != 0 && !R2D_BH::CompCharArrToStr(chunkType, "IDAT", 4))
		{
			std::vector<unsigned char> decodedIDATData;

			DecompressIDATData(rawIDATData, decodedIDATData);
			UnfilterIDATData(decodedIDATData);
			ReadIDATData(decodedIDATData);
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

			if (rawIDATData.size() == 0)
			{
				throw std::runtime_error("No IDAT chunk present in PNG file.");
			}
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
		// Check the stored checksum against the pre-computed one
		uLong storedCRC;
		reader.read((char*)&storedCRC, 4);

		if (crc != R2D_BH::ToBigEndian(storedCRC))
		{
			throw std::runtime_error("Stored checksum for " + std::string(chunkType, 4)+" chunk does not match pre-computed checksum.");
		}
	}

	reader.close();
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

	CheckIHDRData();
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
	if (R2D_BH::CompCharArrToStr(_chunkType, "gAMA", 4))
	{
		std::uint32_t gammaRaw = 0;
		_reader.read((char*)&gammaRaw, 4);

		gamma = (float)R2D_BH::ToBigEndian(gammaRaw) / 100000.f;
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
	const double bytesPerPixel = (double)(bitDepth * channels[colourType]) / 8.0;

	unsigned int offsetBytesPerPixel = (unsigned int)std::fmax(bytesPerPixel, 1); // minimum 1 byte offset

	std::vector<unsigned short> scanlineLengths, startingRows;
	unsigned int totalScanlines;

	GetScanlineVars(scanlineLengths, startingRows, totalScanlines);

	// Return previous byte in scanline
	auto previous = [&](unsigned int _byteIndex, unsigned int _posInScanline)
		{
			if (_posInScanline >= offsetBytesPerPixel)
				return _decompressedData[_byteIndex - offsetBytesPerPixel];

			return (unsigned char)0;
		};

	// Return corresponding byte of prior scanline
	auto prior = [&](unsigned int _byteIndex, unsigned int _scanlineNum, double _stride)
		{
			if (_scanlineNum > startingRows[_scanlineNum])
				return _decompressedData[_byteIndex - (unsigned int)std::fmax(_stride, 1.0)];

			return (unsigned char)0;
		};

	// Return corresponding previous byte of prior scanline
	auto prevPrior = [&](unsigned int _byteIndex, unsigned int _posInScanline, unsigned int _scanlineNum, double _stride)
		{
			if (_scanlineNum > 0 && _posInScanline >= offsetBytesPerPixel)
				return _decompressedData[_byteIndex - (unsigned int)std::fmax(_stride, 1.0) - offsetBytesPerPixel];

			return (unsigned char)0;
		};

	auto paethPredictor = [](unsigned char _left, unsigned char _up, unsigned char _upLeft)
		{
			unsigned short p = (unsigned short)std::fmax(_left + _up - _upLeft, 0);		// initial estimate
			unsigned short pa = abs(p - _left);				// distances to a, b, c
			unsigned short pb = abs(p - _up);
			unsigned short pc = abs(p - _upLeft);

			// Return minimum distance
			if (pa <= pb && pa <= pc) return _left;
			else if (pb <= pc) return _up;
			return _upLeft;
		};

	for (unsigned int scanlineNum = 0, i = 0; scanlineNum < totalScanlines; scanlineNum++)
	{
		char filterMethod = _decompressedData[i];
		_decompressedData.erase(_decompressedData.begin() + i); // remove filter byte from data, not needed after this point

		double stride = (double)((interlaceMethod == 0) ? width : scanlineLengths[scanlineNum]) * bytesPerPixel;

		for (unsigned int posInScanline = 0; posInScanline < stride; posInScanline++)
		{
			unsigned short byte = (unsigned short)_decompressedData[i];

			switch (filterMethod) // NONE, SUB, UP, AVG, PAETH
			{
				case 0: break;
				case 1: byte += previous(i, posInScanline); break;
				case 2: byte += prior(i, scanlineNum, stride); break;
				case 3:
				{
					unsigned char left = previous(i, posInScanline);
					unsigned char up = prior(i, scanlineNum, stride);

					byte += (unsigned char)floor((left + up) / 2.0);

					break;
				}
				case 4:
				{
					unsigned char left = previous(i, posInScanline);
					unsigned char up = prior(i, scanlineNum, stride);
					unsigned char upLeft = prevPrior(i, posInScanline, scanlineNum, stride);

					byte += paethPredictor(left, up, upLeft);

					break;
				}
			}

			_decompressedData[i++] = byte % 256; // keep in byte format
		}
	}
}

void PNGProperties::ReadIDATData(std::vector<unsigned char>& _unfiltered)
{
	BitReader br = BitReader(_unfiltered.data());
	pixels.resize((size_t)(width * height), Color(1, 0, 1, 1));

	if (interlaceMethod == 0)
	{
		for (unsigned int i = 0; i < width * height; i++)
		{
			if (i != 0 && i % width == 0) // if a scanline doesn't end on a byte boundary, skip to next byte
			{
				br.NextByte();
			}

			pixels[i] = GetNextPixel(br);
		}
	}
	else
	{
		unsigned char startingRows[7] = { 0, 0, 4, 0, 2, 0, 1 }, startingCols[7] = { 0, 4, 0, 2, 0, 1, 0 };
		unsigned char rowIncrement[7] = { 8, 8, 8, 4, 4, 2, 2 }, colIncrement[7] = { 8, 8, 4, 4, 2, 2, 1 };

		for (unsigned int pass = 0; pass < 7; pass++)
		{
			for (unsigned int row = startingRows[pass]; row < height; row += rowIncrement[pass])
			{
				for (unsigned int col = startingCols[pass]; col < width; col += colIncrement[pass])
				{
					unsigned int index = (row * width) + col;
					pixels[index] = GetNextPixel(br);
				}

				// if a scanline doesn't end on a byte boundary, skip to next byte
				br.NextByte();
			}
		}
	}
}

void PNGProperties::CheckSignature(std::ifstream& _reader)
{
	constexpr char pngSignature[8] = { -119, 80, 78, 71, 13, 10, 26, 10 };
	const char* pngSig = R2D_BH::ReadBytesIntoStr(_reader, 8);

	if (!R2D_BH::CompCharArrToStr(pngSig, pngSignature, 8))
	{
		std::string expected, actual;
		for (char c : pngSignature) expected += std::to_string((int)c) + ", ";
		for (unsigned int i = 0; i < 8; i++) actual += std::to_string((int)pngSig[i]) + ", ";
		throw std::runtime_error("PNG file signature does not match standard spec.\nExpect: " + expected + "\nActual: " + actual);
	}
}

void PNGProperties::CheckIHDRData()
{
	std::string errorType = "", errorData = "";

	const std::vector<unsigned char> allowedColourTypes = { 0, 2, 3, 4, 6 };
	if (std::find(allowedColourTypes.begin(), allowedColourTypes.end(), colourType) == allowedColourTypes.end())
	{
		errorType = "colour type", errorData = std::to_string((unsigned int)colourType);
	}

	constexpr unsigned short allowedBitDepths[7] = { 0b11111, 0b0, 0b11000, 0b1111, 0b11000, 0b0, 0b11000 };
	if (!(bitDepth & allowedBitDepths[colourType]))
	{
		errorType = "bit depth", errorData = std::to_string((unsigned int)bitDepth);
	}

	if (compressionMethod != 0)
	{
		errorType = "compression method", errorData = std::to_string((unsigned int)compressionMethod);
	}

	if (filterMethod != 0)
	{
		errorType = "filter method", errorData = std::to_string((unsigned int)filterMethod);
	}

	if (interlaceMethod != 0 && interlaceMethod != 1)
	{
		errorType = "interlace method", errorData = std::to_string((unsigned int)interlaceMethod);
	}

	if (errorType != "" && errorData != "")
	{
		throw std::runtime_error("IHDR chunk contains an invalid " + errorType + ". Read as " + errorType + ": " + errorData);
	}
}

bool PNGProperties::IsAncillaryChunk(const char* _chunkType)
{
	// Assuming _chunkType is exactly 4-bytes.
	return (_chunkType[0] & 0b100000) != 0;
}

void PNGProperties::GetScanlineVars(std::vector<unsigned short>& _scanlineLengths, std::vector<unsigned short>& _startingRows, unsigned int& _totalScanlines)
{
	if (interlaceMethod == 0)
	{
		_scanlineLengths = std::vector<unsigned short>(height, width);
		_startingRows = std::vector<unsigned short>(height, 0);
		_totalScanlines = height;

		return;
	}

	constexpr unsigned char offset[8] = { 0, 0, 4, 0, 2, 0, 1, 0 }, divisor[8] = { 8, 8, 8, 4, 4, 2, 2, 1 };
	double passes[7] = { 0, 0, 0, 0, 0, 0, 0 }, lengths[7] = { 0, 0, 0, 0, 0, 0, 0 };
	_totalScanlines = 0;

	for (unsigned int i = 0; i < 7; i++)
	{
		if (height > offset[i] && width > offset[i + 1])
		{
			passes[i] = ceil((double)(width - offset[i]) / divisor[i]);
			lengths[i] = ceil((double)(width - offset[i + 1]) / divisor[i + 1]);

			_totalScanlines += (unsigned int)passes[i];
		}
	}

	_scanlineLengths = std::vector<unsigned short>(_totalScanlines, 0);
	_startingRows = std::vector<unsigned short>(_totalScanlines, 0);

	for (unsigned int i = 0, index = 0, row = 0; i < 7; i++)
	{
		double scanlinesInPass = passes[i];

		for (unsigned int j = 0; j < scanlinesInPass; j++)
		{
			if (index < _startingRows.size())
			{
				_startingRows[index] = row;
				_scanlineLengths[index] = (unsigned short)ceil(lengths[i]);
				index++;
			}
		}

		row += (unsigned int)scanlinesInPass;
	}
}

Color PNGProperties::GetNextPixel(BitReader& _br)
{
	Color output = Color();

	float sampleMax = powf(2.f, (float)bitDepth) - 1.f;

	switch (colourType)
	{
		case 0: // grayscale
		{
			float g = (float)_br.ReadBits(bitDepth);

			output = Color(g, g, g, sampleMax, sampleMax);
			break;
		}

		case 2: // RGB
		{
			float r = (float)_br.ReadBits(bitDepth);
			float g = (float)_br.ReadBits(bitDepth);
			float b = (float)_br.ReadBits(bitDepth);

			output = Color(r, g, b, sampleMax, sampleMax);
			break;
		}

		case 3: // indexed
		{
			unsigned int paletteIndex = (unsigned int)_br.ReadBits(bitDepth);

			output = palette[paletteIndex];
			break;
		}

		case 4: // grayscale + alpha
		{
			float g = (float)_br.ReadBits(bitDepth);
			float a = (float)_br.ReadBits(bitDepth);

			output = Color(g, g, g, a, sampleMax);
			break;
		}

		case 6: // RGB + alpha
		{
			float r = (float)_br.ReadBits(bitDepth);
			float g = (float)_br.ReadBits(bitDepth);
			float b = (float)_br.ReadBits(bitDepth);
			float a = (float)_br.ReadBits(bitDepth);

			output = Color(r, g, b, a, sampleMax);
			break;
		}
	}

	ApplyGamma(output);
	CheckTRNS(output);

	return output;
}

void PNGProperties::ApplyGamma(Color& _color)
{
	float gammaPower = 1.f / gamma;

	_color.r = powf(_color.r, gammaPower);
	_color.g = powf(_color.g, gammaPower);
	_color.b = powf(_color.b, gammaPower);
}

void PNGProperties::CheckTRNS(Color& _color)
{
	// Any pixel that matches the TRNS color should be treated as fully transparent.
	if ((colourType == 0 && _color.r == trnsColor.r) ||
		(colourType == 2 && Color::EqualRGB(_color, trnsColor)))
	{
		_color.a = 0;
	}
}
