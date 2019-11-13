#pragma once

struct IntVec2;

//-----------------------------------------------------------------------------------------------
struct Vec2
{
public:
	// Construction/Destruction
	Vec2() {}												// default constructor: do nothing (for speed)
	Vec2(const Vec2& copyFrom);							// copy constructor (from another Vec2)
	Vec2(const IntVec2& copyFrom);						// copy constructor (from an IntVec2)
	explicit Vec2(float initialX, float initialY);		// explicit constructor (from x, y)

	//Static Vectors
	const static Vec2	ZERO;
	const static Vec2	ONE;
	const static Vec2	NEGATIVE_ONE;

	const static Vec2	ALIGN_CENTERED;
	const static Vec2	ALIGN_LEFT_CENTERED;
	const static Vec2	ALIGN_LEFT_BOTTOM;
	const static Vec2	ALIGN_LEFT_TOP;
	const static Vec2	ALIGN_RIGHT_CENTERED;
	const static Vec2	ALIGN_RIGHT_BOTTOM;
	const static Vec2	ALIGN_RIGHT_TOP;
	const static Vec2	ALIGN_TOP_CENTERED;
	const static Vec2	ALIGN_BOTTOM_CENTERED;

	//Accessor methods
	float				GetLength() const;
	float				GetLengthSquared() const;
	float				GetAngleDegrees() const;
	float				GetAngleRadians() const;
	const Vec2			GetRotated90Degrees() const;
	const Vec2			GetRotatedMinus90Degrees() const;
	const Vec2			GetRotatedDegrees(float degreesToRotate) const;
	const Vec2			GetRotatedRadians(float radiansToRotate) const;
	const Vec2			GetClamped(float maxLenth) const;
	const Vec2			GetNormalized() const;

	static const Vec2	MakeFromPolarDegrees(const float polarDegrees, float r = 1.0f);
	static const Vec2	MakeFromPolarRadians(const float polarRadians, float r = 1.0f);

	//Mutator methods
	const Vec2			ClampVector(Vec2& toClamp, const Vec2& minBound, const Vec2& maxBound);
	void				ClampLength(float maxLength);
	void				SetLength(float setLength);
	void				SetAngleDegrees(float setAngleDeg);
	void				SetAngleRadians(float setAngleRad);
	void				SetPolarDegrees(float setPolarDeg, float r = 1.0f);
	void				SetPolarRadians(float setPolarRad, float r = 1.0f);
	void				RotateRadians(float radiansToRotate);
	void				RotateDegrees(float degreesToRotate);
	void				Rotate90Degrees();
	void				RotateMinus90Degrees();
	void				Normalize();
	float				NormalizeAndGetOldLength();

	//Debug
	const float			GetX();
	const float			GetY();

	// Operators
	const Vec2			operator+(const Vec2& vecToAdd) const;				// vec2 + vec2
	const Vec2			operator-(const Vec2& vecToSubtract) const;			// vec2 - vec2
	const Vec2			operator*(float uniformScale) const;					// vec2 * float
	const Vec2			operator*(const Vec2& vecToMultiply) const;			// vec2 * vec2
	const Vec2			operator/(float inverseScale) const;					// vec2 / float
	void				operator+=(const Vec2& vecToAdd);						// vec2 += vec2
	void				operator-=(const Vec2& vecToSubtract);				// vec2 -= vec2
	void				operator*=(const float uniformScale);					// vec2 *= float
	void				operator/=(const float uniformDivisor);				// vec2 /= float
	void				operator=(const Vec2& copyFrom);						// vec2 = vec2
	bool				operator==(const Vec2& compare) const;				// vec2 == vec2
	bool				operator!=(const Vec2& compare) const;				// vec2 != vec2
	bool				operator<(const Vec2& compare) const;					// vec2 < vec2
	bool				operator>(const Vec2& compare) const;					// vec2 > vec2

	Vec2				Min(const Vec2& compare);
	Vec2				Max(const Vec2& compare);

	friend const Vec2 operator*(float uniformScale, const Vec2& vecToScale);	// float * vec2

public:
	float x;
	float y;
};