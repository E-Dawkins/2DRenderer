#pragma once

struct Color
{
public:
	Color()
	{
		r = 1.f;
		g = 1.f;
		b = 1.f;
		a = 1.f;
	}

	Color(float _r, float _g, float _b, float _a)
	{
		r = _r;
		g = _g;
		b = _b;
		a = _a;
	}

public:
	float r, g, b, a;
};