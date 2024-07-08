#include "RenderObject.h"

RenderObject::RenderObject()
{
	mPosition	= Vec2(0, 0);
	mSize		= Vec2(100, 100);
	mOrigin		= Vec2(0.5f, 0.5f);

	mScale		= 1.f;
	mRotation	= 0.f;

	mZOrder		= 0;

	mVisible	= true;
}
