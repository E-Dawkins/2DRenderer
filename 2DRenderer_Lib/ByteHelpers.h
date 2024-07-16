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
};