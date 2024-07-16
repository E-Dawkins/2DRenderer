#pragma once
#include "DLLCommon.h"

struct RENDERER_API Color
{
public:
	Color()
	{
		r = 1.f;
		g = 1.f;
		b = 1.f;
		a = 1.f;
	}

	Color(float _r, float _g, float _b, float _a, float _channelMax = 1.f)
	{
		r = _r / _channelMax;
		g = _g / _channelMax;
		b = _b / _channelMax;
		a = _a / _channelMax;
	}

public:
	float r, g, b, a;
};