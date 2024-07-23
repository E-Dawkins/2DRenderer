#pragma once
#include "DLLCommon.h"

class RENDERER_API BitReader
{
public:
	BitReader(unsigned char* _data)
	{
		mData = _data;
		mPos = 0;
		mLastByte = mData[0];
		mBitCount = 0;
	}

	unsigned short ReadBits(unsigned int _count)
	{
		unsigned short out = '\0';

		for (unsigned int i = 0; i < _count; i++)
		{
			if (mBitCount >= 8) // fully read last byte, move to next byte in data
			{
				mPos++;
				mLastByte = mData[mPos];
				mBitCount = 0;
			}

			unsigned char bit = (mLastByte >> (7 - mBitCount)) & 1; // shift out a single bit

			out |= bit << (_count - i - 1);

			mBitCount++;
		}

		return out;
	}

	// Finish reading current byte and skip to next.
	void NextByte()
	{
		ReadBits(8 - mBitCount);
	}

protected:
	unsigned char* mData;
	unsigned int mPos;

	unsigned char mLastByte;
	unsigned int mBitCount;
};