#pragma once
#include "DLLCommon.h"
#include "Vec2.h"

class RENDERER_API RenderObject
{
public:
	RenderObject();

	// Getters
	const Vec2& GetPosition() const				{ return mPosition; }
	const Vec2& GetSize() const					{ return mSize; }
	const Vec2& GetOrigin() const				{ return mOrigin; }

	const float& GetScale() const				{ return mScale; }
	const float& GetRotation() const			{ return mRotation; }

	const unsigned int& GetZOrder() const		{ return mZOrder; }

	const bool& GetVisible() const				{ return mVisible; }

	// Setters
	void SetPosition(const Vec2& _position)			{ mPosition = _position; }
	void SetSize(const Vec2& _size)					{ mSize = _size; }
	void SetOrigin(const Vec2& _origin)				{ mOrigin = _origin; }

	void SetScale(const float& _scale)				{ mScale = _scale; }
	void SetRotation(const float& _rotation)		{ mRotation = _rotation; }

	void SetZOrder(const unsigned int& _zOrder)		{ mZOrder = _zOrder; }

	void SetVisible(const bool& _visible)			{ mVisible = _visible; }

protected:
	// Anchor mAnchor; TODO

	Vec2 mPosition;
	Vec2 mSize;
	Vec2 mOrigin;

	float mScale;
	float mRotation;

	unsigned int mZOrder;

	bool mVisible;
};

