#pragma once
#include "DLLCommon.h"

class RENDERER_API R2D_BH
{
public:
	static unsigned int CharArrToUInt(char* _arr, unsigned int _len)
	{
		unsigned int out = 0;

		for (unsigned int i = 0; i < _len; i++)
		{
			int shift = (_len - i - 1) * 8;
			out |= ((unsigned char)_arr[i] << shift);
		}

		return out;
	}

	static bool CompCharArrToStr(const char* _arr, const char* _str, unsigned int _len)
	{
		for (unsigned int i = 0; i < _len; i++)
		{
			if (_arr[i] != _str[i])
				return false;
		}

		return true;
	}

	static unsigned int ReadBytesIntoUInt(std::ifstream& _reader, unsigned int _byteCount)
	{
		char* num = new char[_byteCount];
		_reader.read(num, _byteCount);

		return CharArrToUInt(num, _byteCount);
	}

	static char* ReadBytesIntoStr(std::ifstream& _reader, unsigned int _byteCount)
	{
		char* str = new char[_byteCount];
		_reader.read(str, _byteCount);

		return str;
	}

	static bool IsLittleEndian()
	{
		unsigned int x = 1;
		char* c = (char*)&x;

		return (*c);
	}

	static std::uint32_t ToBigEndian(std::uint32_t _value)
	{
		if (IsLittleEndian())
		{
			return ((_value >> 24) & 0x000000FF) |
				((_value >> 8) & 0x0000FF00) |
				((_value << 8) & 0x00FF0000) |
				((_value << 24) & 0xFF000000);
		}
		
		return _value;
	}
};