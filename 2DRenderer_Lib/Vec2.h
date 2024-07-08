#pragma once

struct Vec2
{
public:
	Vec2()
	{
		x = 0.f;
		y = 0.f;
	}

	Vec2(float _x, float _y)
	{
		x = _x;
		y = _y;
	}

	Vec2 operator + (Vec2& _v)
	{
		return Vec2(x + _v.x, y + _v.y);
	}

	Vec2 operator - (Vec2& _v)
	{
		return Vec2(x - _v.x, y - _v.y);
	}

	friend Vec2 operator * (float _s, Vec2 _v)
	{
		return Vec2(_s * _v.x, _s * _v.y);
	}

	friend Vec2 operator * (Vec2 _v, float _s)
	{
		return Vec2(_s * _v.x, _s * _v.y);
	}

	Vec2& Normalize()
	{
		float vecLen = (x * x) + (y * y);
		
		x /= vecLen;
		y /= vecLen;

		return *this;
	}

public:
	float x, y;
};