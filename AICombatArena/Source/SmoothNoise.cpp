//-----------------------------------------------------------------------------------------------
// SmoothNoise.cpp
//
#include "SmoothNoise.hpp"
#include "RawNoise.hpp"			// for raw bit-noise base functions (SquirrelNoise4)
#include "MathUtils.hpp"	// for SmoothStep3(); see "SmoothStep" on Wikipedia
#include "Vec2.hpp"			// for Vec2( float x,y ) class/struct

/////////////////////////////////////////////////////////////////////////////////////////////////
// For all fractal (and Perlin) noise functions, the following internal naming conventions
//	are used, primarily to help me visualize 3D and 4D constructs clearly.  They need not
//	have any actual bearing on / relationship to actual external coordinate systems.
//
// 1D noise: only X (+east / -west)
// 2D noise: also Y (+north / -south)
// 3D noise: also Z (+above / -below)
// 4D noise: also T (+after / -before)
/////////////////////////////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------------------------
float Compute1dFractalNoise( float position, float scale, unsigned int numOctaves, float octavePersistence, float octaveScale, bool renormalize, unsigned int seed )
{
	const float OCTAVE_OFFSET = 0.636764989593174f; // Translation/bias to add to each octave

	float totalNoise = 0.f;
	float totalAmplitude = 0.f;
	float currentAmplitude = 1.f;
	float currentPosition = position * (1.f / scale);

	for( unsigned int octaveNum = 0; octaveNum < numOctaves; ++ octaveNum )
	{
		// Determine noise values at nearby integer "grid point" positions
		float positionFloor = floorf( currentPosition );
		int indexWest = (int) positionFloor;
		int indexEast = indexWest + 1;
		float valueWest = Get1dNoiseZeroToOne( indexWest, seed );
		float valueEast = Get1dNoiseZeroToOne( indexEast, seed );

		// Do a smoothed (nonlinear) weighted average of nearby grid point values
		float distanceFromWest = currentPosition - positionFloor;
		float weightEast = SmoothStep3( distanceFromWest ); // Gives rounder (nonlinear) results
		float weightWest = 1.f - weightEast;
		float noiseZeroToOne = (valueWest * weightWest) + (valueEast * weightEast);
		float noiseThisOctave = 2.f * (noiseZeroToOne - 0.5f); // Map from [0,1] to [-1,1]

		// Accumulate results and prepare for next octave (if any)
		totalNoise += noiseThisOctave * currentAmplitude;
		totalAmplitude += currentAmplitude;
		currentAmplitude *= octavePersistence;
		currentPosition *= octaveScale;
		currentPosition += OCTAVE_OFFSET; // Add "irrational" offset to de-align octave grids
		++ seed; // Eliminates octaves "echoing" each other (since each octave is uniquely seeded)
	}

	// Re-normalize total noise to within [-1,1] and fix octaves pulling us far away from limits
	if( renormalize && totalAmplitude > 0.f )
	{
		totalNoise /= totalAmplitude;				// Amplitude exceeds 1.0 if octaves are used!
		totalNoise = (totalNoise * 0.5f) + 0.5f;	// Map to [0,1]
		totalNoise = SmoothStep3( totalNoise );		// Push towards extents (octaves pull us away)
		totalNoise = (totalNoise * 2.0f) - 1.f;		// Map back to [-1,1]
	}

	return totalNoise;
}


//-----------------------------------------------------------------------------------------------
float Compute2dFractalNoise( float posX, float posY, float scale, unsigned int numOctaves, float octavePersistence, float octaveScale, bool renormalize, unsigned int seed )
{
	const float OCTAVE_OFFSET = 0.636764989593174f; // Translation/bias to add to each octave

	float totalNoise = 0.f;
	float totalAmplitude = 0.f;
	float currentAmplitude = 1.f;
	float invScale = (1.f / scale);
	Vec2 currentPos( posX * invScale, posY * invScale );

	for( unsigned int octaveNum = 0; octaveNum < numOctaves; ++ octaveNum )
	{
		// Determine noise values at nearby integer "grid point" positions
		Vec2 cellMins( floorf( currentPos.x ), floorf( currentPos.y ) );
		int indexWestX = (int) cellMins.x;
		int indexSouthY = (int) cellMins.y;
		int indexEastX = indexWestX + 1;
		int indexNorthY = indexSouthY + 1;
		float valueSouthWest = Get2dNoiseZeroToOne( indexWestX, indexSouthY, seed );
		float valueSouthEast = Get2dNoiseZeroToOne( indexEastX, indexSouthY, seed );
		float valueNorthWest = Get2dNoiseZeroToOne( indexWestX, indexNorthY, seed );
		float valueNorthEast = Get2dNoiseZeroToOne( indexEastX, indexNorthY, seed );

		// Do a smoothed (nonlinear) weighted average of nearby grid point values
		Vec2 displacementFromMins = currentPos - cellMins;
		float weightEast  = SmoothStep3( displacementFromMins.x );
		float weightNorth = SmoothStep3( displacementFromMins.y );
		float weightWest  = 1.f - weightEast;
		float weightSouth = 1.f - weightNorth;

		float blendSouth = (weightEast * valueSouthEast) + (weightWest * valueSouthWest);
		float blendNorth = (weightEast * valueNorthEast) + (weightWest * valueNorthWest);
		float blendTotal = (weightSouth * blendSouth) + (weightNorth * blendNorth);
		float noiseThisOctave = 2.f * (blendTotal - 0.5f); // Map from [0,1] to [-1,1]

		// Accumulate results and prepare for next octave (if any)
		totalNoise += noiseThisOctave * currentAmplitude;
		totalAmplitude += currentAmplitude;
		currentAmplitude *= octavePersistence;
		currentPos *= octaveScale;
		currentPos.x += OCTAVE_OFFSET; // Add "irrational" offsets to noise position components
		currentPos.y += OCTAVE_OFFSET; //	at each octave to break up their grid alignment
		++ seed; // Eliminates octaves "echoing" each other (since each octave is uniquely seeded)
	}

	// Re-normalize total noise to within [-1,1] and fix octaves pulling us far away from limits
	if( renormalize && totalAmplitude > 0.f )
	{
		totalNoise /= totalAmplitude;				// Amplitude exceeds 1.0 if octaves are used
		totalNoise = (totalNoise * 0.5f) + 0.5f;	// Map to [0,1]
		totalNoise = SmoothStep3( totalNoise );		// Push towards extents (octaves pull us away)
		totalNoise = (totalNoise * 2.0f) - 1.f;		// Map back to [-1,1]
	}

	return totalNoise;
}

//-----------------------------------------------------------------------------------------------
// Perlin noise is fractal noise with "gradient vector smoothing" applied.
//
// In 1D, the gradients are trivial: -1.0 or 1.0, so resulting noise is boring at one octave.
//
float Compute1dPerlinNoise( float position, float scale, unsigned int numOctaves, float octavePersistence, float octaveScale, bool renormalize, unsigned int seed )
{
	const float OCTAVE_OFFSET = 0.636764989593174f; // Translation/bias to add to each octave
	const float gradients[2] = { -1.f, 1.f }; // 1D unit "gradient" vectors; one back, one forward

	float totalNoise = 0.f;
	float totalAmplitude = 0.f;
	float currentAmplitude = 1.f;
	float currentPosition = position * (1.f / scale);

	for( unsigned int octaveNum = 0; octaveNum < numOctaves; ++ octaveNum )
	{
		// Determine random "gradient vectors" (just +1 or -1 for 1D Perlin) for surrounding corners
		float positionFloor = (float) floorf( currentPosition );
		int indexWest = (int) positionFloor;
		int indexEast = indexWest + 1;
		float gradientWest = gradients[ Get1dNoiseUint( indexWest, seed ) & 0x00000001 ];
		float gradientEast = gradients[ Get1dNoiseUint( indexEast, seed ) & 0x00000001 ];

		// Dot each point's gradient with displacement from point to position
		float displacementFromWest = currentPosition - positionFloor; // always positive
		float displacementFromEast = displacementFromWest - 1.f; // always negative
		float dotWest = gradientWest * displacementFromWest; // 1D "dot product" is... multiply
		float dotEast = gradientEast * displacementFromEast;

		// Do a smoothed (nonlinear) weighted average of dot results
		float weightEast = SmoothStep3( displacementFromWest );
		float weightWest = 1.f - weightEast;
		float blendTotal = (weightWest * dotWest) + (weightEast * dotEast);
		float noiseThisOctave = 2.f * blendTotal; // 1D Perlin is in [-.5,.5]; map to [-1,1]

		// Accumulate results and prepare for next octave (if any)
		totalNoise += noiseThisOctave * currentAmplitude;
		totalAmplitude += currentAmplitude;
		currentAmplitude *= octavePersistence;
		currentPosition *= octaveScale;
		currentPosition += OCTAVE_OFFSET; // Add "irrational" offset to de-align octave grids
		++ seed; // Eliminates octaves "echoing" each other (since each octave is uniquely seeded)
	}

	// Re-normalize total noise to within [-1,1] and fix octaves pulling us far away from limits
	if( renormalize && totalAmplitude > 0.f )
	{
		totalNoise /= totalAmplitude;				// Amplitude exceeds 1.0 if octaves are used
		totalNoise = (totalNoise * 0.5f) + 0.5f;	// Map to [0,1]
		totalNoise = SmoothStep3( totalNoise );		// Push towards extents (octaves pull us away)
		totalNoise = (totalNoise * 2.0f) - 1.f;		// Map back to [-1,1]
	}

	return totalNoise;
}


//-----------------------------------------------------------------------------------------------
// Perlin noise is fractal noise with "gradient vector smoothing" applied.
//
// In 2D, gradients are unit-length vectors in various directions with even angular distribution.
//
float Compute2dPerlinNoise( float posX, float posY, float scale, unsigned int numOctaves, float octavePersistence, float octaveScale, bool renormalize, unsigned int seed )
{
	const float OCTAVE_OFFSET = 0.636764989593174f; // Translation/bias to add to each octave
	const Vec2 gradients[ 8 ] = // Normalized unit vectors in 8 quarter-cardinal directions
	{
		Vec2( +0.923879533f, +0.382683432f ), //  22.5 degrees (ENE)
		Vec2( +0.382683432f, +0.923879533f ), //  67.5 degrees (NNE)
		Vec2( -0.382683432f, +0.923879533f ), // 112.5 degrees (NNW)
		Vec2( -0.923879533f, +0.382683432f ), // 157.5 degrees (WNW)
		Vec2( -0.923879533f, -0.382683432f ), // 202.5 degrees (WSW)
		Vec2( -0.382683432f, -0.923879533f ), // 247.5 degrees (SSW)
		Vec2( +0.382683432f, -0.923879533f ), // 292.5 degrees (SSE)
		Vec2( +0.923879533f, -0.382683432f )	 // 337.5 degrees (ESE)
	};

	float totalNoise = 0.f;
	float totalAmplitude = 0.f;
	float currentAmplitude = 1.f;
	float invScale = (1.f / scale);
	Vec2 currentPos( posX * invScale, posY * invScale );

	for( unsigned int octaveNum = 0; octaveNum < numOctaves; ++ octaveNum )
	{
		// Determine random unit "gradient vectors" for surrounding corners
		Vec2 cellMins( floorf( currentPos.x ), floorf( currentPos.y ) );
		Vec2 cellMaxs( cellMins.x + 1.f, cellMins.y + 1.f );
		int indexWestX  = (int) cellMins.x;
		int indexSouthY = (int) cellMins.y;
		int indexEastX  = indexWestX  + 1;
		int indexNorthY = indexSouthY + 1;

		unsigned int noiseSW = Get2dNoiseUint( indexWestX, indexSouthY, seed );
		unsigned int noiseSE = Get2dNoiseUint( indexEastX, indexSouthY, seed );
		unsigned int noiseNW = Get2dNoiseUint( indexWestX, indexNorthY, seed );
		unsigned int noiseNE = Get2dNoiseUint( indexEastX, indexNorthY, seed );

		const Vec2& gradientSW = gradients[ noiseSW & 0x00000007 ];
		const Vec2& gradientSE = gradients[ noiseSE & 0x00000007 ];
		const Vec2& gradientNW = gradients[ noiseNW & 0x00000007 ];
		const Vec2& gradientNE = gradients[ noiseNE & 0x00000007 ];

		// Dot each corner's gradient with displacement from corner to position
		Vec2 displacementFromSW( currentPos.x - cellMins.x, currentPos.y - cellMins.y );
		Vec2 displacementFromSE( currentPos.x - cellMaxs.x, currentPos.y - cellMins.y );
		Vec2 displacementFromNW( currentPos.x - cellMins.x, currentPos.y - cellMaxs.y );
		Vec2 displacementFromNE( currentPos.x - cellMaxs.x, currentPos.y - cellMaxs.y );

		float dotSouthWest = GetDotProduct( gradientSW, displacementFromSW );
		float dotSouthEast = GetDotProduct( gradientSE, displacementFromSE );
		float dotNorthWest = GetDotProduct( gradientNW, displacementFromNW );
		float dotNorthEast = GetDotProduct( gradientNE, displacementFromNE );

		// Do a smoothed (nonlinear) weighted average of dot results
		float weightEast = SmoothStep3( displacementFromSW.x );
		float weightNorth = SmoothStep3( displacementFromSW.y );
		float weightWest = 1.f - weightEast;
		float weightSouth = 1.f - weightNorth;

		float blendSouth = (weightEast * dotSouthEast) + (weightWest * dotSouthWest);
		float blendNorth = (weightEast * dotNorthEast) + (weightWest * dotNorthWest);
		float blendTotal = (weightSouth * blendSouth) + (weightNorth * blendNorth);
		float noiseThisOctave = blendTotal * (1.f / 0.662578106f); // 2D Perlin is in [-.662578106,.662578106]; map to ~[-1,1]
		
		// Accumulate results and prepare for next octave (if any)
		totalNoise += noiseThisOctave * currentAmplitude;
		totalAmplitude += currentAmplitude;
		currentAmplitude *= octavePersistence;
		currentPos *= octaveScale;
		currentPos.x += OCTAVE_OFFSET; // Add "irrational" offset to de-align octave grids
		currentPos.y += OCTAVE_OFFSET; // Add "irrational" offset to de-align octave grids
		++ seed; // Eliminates octaves "echoing" each other (since each octave is uniquely seeded)
	}

	// Re-normalize total noise to within [-1,1] and fix octaves pulling us far away from limits
	if( renormalize && totalAmplitude > 0.f )
	{
		totalNoise /= totalAmplitude;				// Amplitude exceeds 1.0 if octaves are used
		totalNoise = (totalNoise * 0.5f) + 0.5f;	// Map to [0,1]
		totalNoise = SmoothStep3( totalNoise );		// Push towards extents (octaves pull us away)
		totalNoise = (totalNoise * 2.0f) - 1.f;		// Map back to [-1,1]
	}

	return totalNoise;
}

