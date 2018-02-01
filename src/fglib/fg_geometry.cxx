/**
 * @file fg_geometry.cxx
 */                                                                           
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, U$
//

//////////////////////////////////////////////////////////////////////
//
//  Server for FlightGear, geometry functions
//  taken from simgear and other sources
//
//////////////////////////////////////////////////////////////////////
#ifdef HAVE_CONFIG_H
#include "config.h" // for MSVC, always first
#endif

#include <math.h>
#ifndef _MSC_VER
	#include <strings.h>
#endif // _MSC_VER
#include <assert.h>
#include "fg_geometry.hxx"

namespace
{
/**
 * High-precision versions of the above produced with an arbitrary
 * precision calculator (the compiler might lose a few bits in the FPU
 * operations).  These are specified to 81 bits of mantissa, which is
 * higher than any FPU known to me:
 */
constexpr double SQUASH			{ 0.9966471893352525192801545 };
constexpr double SG_NM_TO_METER		{ 1852.0000 };
constexpr double SG_METER_TO_FEET	{ 3.28083989501312335958 };
constexpr double E2 			{ fabs ( 1 - SQUASH * SQUASH ) };
constexpr double E4			{ E2 * E2 };
constexpr double EQURAD			{ 6378137.0 };
constexpr double RA2			{  1 / (EQURAD * EQURAD) };
constexpr double SG_180			{ 180.0 };
constexpr double SG_PI			{ 3.1415926535 };
constexpr double SG_RADIANS_TO_DEGREES	{ SG_180 / SG_PI };

} // namespace

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

point3d::point3d
()
{
	clear ();
}

//////////////////////////////////////////////////////////////////////

point3d::point3d
(
	const point3d_t& x,
	const point3d_t& y,
	const point3d_t& z
)
{
	m_x = x;
	m_y = y;
	m_z = z;
}

//////////////////////////////////////////////////////////////////////

void point3d::set
(
	const point3d_t& x,
	const point3d_t& y,
        const point3d_t& z
)
{
	m_x = x;
	m_y = y;
	m_z = z;
}

//////////////////////////////////////////////////////////////////////

point3d&
point3d::operator +=
(
	const point3d& p
)
{
	m_x += p.m_x;
	m_y += p.m_y;
	m_z += p.m_z;
	return *this;
}

//////////////////////////////////////////////////////////////////////

point3d&
point3d::operator -=
(
	const point3d& p
)
{
	m_x -= p.m_x;
	m_y -= p.m_y;
	m_z -= p.m_z;
	return *this;
}

//////////////////////////////////////////////////////////////////////

point3d&
point3d::operator *=
(
	const point3d& p
)
{
	m_x *= p.m_x;
	m_y *= p.m_y;
	m_z *= p.m_z;
	return *this;
}

//////////////////////////////////////////////////////////////////////

point3d&
point3d::operator /=
(
	const point3d& p
)
{
	m_x /= p.m_x;
	m_y /= p.m_y;
	m_z /= p.m_z;
	return *this;
}

//////////////////////////////////////////////////////////////////////

point3d&
point3d::operator ^=
(
	const point3d& p
)
{
	m_x = (m_y * p.m_z) - (m_z * p.m_y);
	m_y = (m_z * p.m_x) - (m_z * p.m_z);
	m_z = (m_x * p.m_y) - (m_z * p.m_x);
	return *this;
}

//////////////////////////////////////////////////////////////////////

point3d&
point3d::operator *=
(
	point3d_t v
)
{
	m_x *= v;
	m_y *= v;
	m_z *= v;
	return *this;
}

//////////////////////////////////////////////////////////////////////

point3d&
point3d::operator /=
(
	point3d_t v
)
{
	m_x /= v;
	m_y /= v;
	m_z /= v;
	return *this;
}

//////////////////////////////////////////////////////////////////////

bool
point3d::operator ==
(
	const point3d& p
)
{
	if ((m_x == p.m_x) && (m_y == p.m_y) && (m_z == p.m_z))
		return (true);
	return (false);
}

//////////////////////////////////////////////////////////////////////

bool
point3d::operator !=
(
	const point3d& p
)
{
	if ((m_x == p.m_x) && (m_y == p.m_y) && (m_z == p.m_z))
		return (false);
	return (true);
}

//////////////////////////////////////////////////////////////////////

point3d_t
point3d::operator []
(
	const int index
) const
{
	assert (! (index < X || index > Z));
	switch (index)
	{
		case X: return m_x;
		case Y: return m_y;
		case Z: return m_z;
	}
	return (0.0); // never reached
}

//////////////////////////////////////////////////////////////////////

point3d_t&
point3d::operator []
(
	const int index
)
{
	assert (! (index < X || index > Z));
	switch (index)
	{
		case X: return m_x;
		case Y: return m_y;
		case Z: return m_z;
	}
	return (m_x);
}

//////////////////////////////////////////////////////////////////////

point3d_t
point3d::length
() const
{
	return sqrt ((m_x * m_x) + (m_y * m_y) + (m_z * m_z));
}

//////////////////////////////////////////////////////////////////////

void
point3d::normalize
()
{
	point3d_t len;

	len = length();
	m_x /= len;
	m_y /= len;
	m_z /= len;
}

//////////////////////////////////////////////////////////////////////

point3d_t
point3d::sqr
()
{
	return (m_x * m_x) + (m_y * m_y) + (m_z * m_z);
}

//////////////////////////////////////////////////////////////////////

void
point3d::invert
()
{
	m_x = -m_x;
	m_y = -m_y;
	m_z = -m_z;
}

//////////////////////////////////////////////////////////////////////

void
point3d::clear
()
{
	m_x = 0.0;
	m_y = 0.0;
	m_z = 0.0;
}

//////////////////////////////////////////////////////////////////////

point3d
invert
(
	const point3d& p
)
{
	return (point3d (
	    -p.m_x,
	    -p.m_y,
	    -p.m_z
	));
}

//////////////////////////////////////////////////////////////////////

point3d_t
sqr
(
	const point3d& p
)
{
	return (
	   (p.m_x * p.m_x) +
	   (p.m_y * p.m_y) +
	   (p.m_z * p.m_z)
	);
}

//////////////////////////////////////////////////////////////////////

/**
 * @brief Normalize P
 */
point3d
normalize
(
	const point3d& p
)
{
	point3d_t len;

	len = p.length();
	return (point3d (
	    (p.m_x / len),
	    (p.m_y / len),
	    (p.m_z / len)
	));
} // normalize ( const point3d& P )

//////////////////////////////////////////////////////////////////////

/**
 * @brief Return the length of P
 */
point3d_t
length
(
	const point3d& p
)
{
	return (sqrt (
	    (p.m_x * p.m_x) +
	    (p.m_y * p.m_y) +
	    (p.m_z * p.m_z)
	));
} // length ( const point3d& P )

//////////////////////////////////////////////////////////////////////

/**
 * @brief Calculate distance of clients
 */
float
distance
(
	const point3d & p1,
	const point3d & p2
)
{
	point3d p;

	p = p1 - p2;
	return (float)(p.length() / SG_NM_TO_METER);
} // distance ( const point3d & P1, const point3d & P2 )

//////////////////////////////////////////////////////////////////////

/**
 * @brief Convert a cartexian XYZ coordinate to a geodetic lat/lon/alt.
 *        This function is a copy of what's in SimGear,
 *        simgear/math/SGGeodesy.cxx
 */
void
cart_to_geod
(
	const point3d& cart_point,
	point3d& geod_point
)
{
	// according to
	// H. Vermeille,
	// Direct transformation from geocentric to geodetic cordinates,
	// Journal of Geodesy (2002) 76:451-454
	double x = cart_point[point3d::X];
	double y = cart_point[point3d::Y];
	double z = cart_point[point3d::Z];
	double XXpYY = x * x + y * y;
	double sqrtXXpYY = sqrt ( XXpYY );
	double p = XXpYY * RA2;
	double q = z * z * ( 1 - E2 ) * RA2;
	double r = 1 / 6.0 * ( p + q - E4);
	double s = E4 * p * q / ( 4 * r * r * r );
	double t = pow ( 1 + s + sqrt ( s * (2 + s ) ), 1 /3.0 );
	double u = r * ( 1 + t + 1 / t);
	double v = sqrt ( u * u + E4 * q );
	double w = E2 * ( u + v - q ) / ( 2 * v );
	double k = sqrt ( u + v + w * w ) - w;
	double D = k * sqrtXXpYY / ( k + E2 );
	geod_point[point3d::LON] =
	  ( 2 * atan2 ( y, x + sqrtXXpYY ) ) * SG_RADIANS_TO_DEGREES;
	double sqrtDDpZZ = sqrt ( D * D + z * z );
	geod_point[point3d::LAT] =
	  ( 2 * atan2 ( z, D + sqrtDDpZZ ) ) * SG_RADIANS_TO_DEGREES;
	geod_point[point3d::ALT] = 
	  ( ( k + E2 - 1 ) * sqrtDDpZZ / k ) * SG_METER_TO_FEET;
} // sgCartToGeod()

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

