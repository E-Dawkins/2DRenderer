#pragma once
#include "DLLCommon.h"

struct RENDERER_API Color
{
public:
	Color()
	{
		r = 0.f;
		g = 0.f;
		b = 0.f;
		a = 0.f;
	}

	// Color channels are first divided by the _channelMax, then clamped between 0-1.
	Color(float _r, float _g, float _b, float _a, float _channelMax = 1.f, bool _clamped = true)
	{
		r = _r / _channelMax;
		g = _g / _channelMax;
		b = _b / _channelMax;
		a = _a / _channelMax;

		if (_clamped)
		{
			r = std::fmax(std::fmin(r, 1.f), 0.f);
			g = std::fmax(std::fmin(g, 1.f), 0.f);
			b = std::fmax(std::fmin(b, 1.f), 0.f);
			a = std::fmax(std::fmin(a, 1.f), 0.f);
		}
	}

	bool operator == (Color& _other)
	{
		return (
			(r == _other.r) &&
			(g == _other.g) &&
			(b == _other.b) &&
			(a == _other.a)
		);
	}

	static bool EqualRGB(Color& _color1, Color& _color2)
	{
		return (
			(_color1.r == _color2.r) && 
			(_color1.g == _color2.g) &&
			(_color1.b == _color2.b)
		);
	}

public:
	float r, g, b, a;
};