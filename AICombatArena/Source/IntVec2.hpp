#pragma once

struct Vec2;

//------------------------------------------------------------------------------------------------------------------------------
struct IntVec2
{
	int x;
	int y;

public:
	IntVec2();
	~IntVec2();
	IntVec2(const IntVec2& copyFrom);							// copy constructor (from another vec2)
	IntVec2(const Vec2& vec2);
	explicit IntVec2(int initialX, int initialY);		// explicit constructor (from x, y)

	void					SetIntVec2(IntVec2 inValue);
	IntVec2					GetIntVec2() const;

	bool					IsInBounds(const IntVec2& bounds) const;

	//Static Vectors
	const static IntVec2 ZERO;
	const static IntVec2 ONE;

	//Operators
	const IntVec2			operator+(const IntVec2& vecToAdd) const;
	const IntVec2			operator-(const IntVec2& vecToSubtract) const;
	const IntVec2			operator*(int uniformScale) const;
	const IntVec2			operator/(int inverseScale) const;
	void					operator+=(const IntVec2& vecToAdd);
	void					operator-=(const IntVec2& vecToSubtract);
	void					operator*=(const int uniformScale);
	void					operator/=(const int uniformDivisor);
	void					operator=(const IntVec2& copyFrom);
	bool					operator==(const IntVec2& compare) const;
	bool					operator!=(const IntVec2& compare) const;
	bool					operator<(const IntVec2& compare) const;

	friend const IntVec2	operator*(int uniformScale, const IntVec2& vecToScale);
};
