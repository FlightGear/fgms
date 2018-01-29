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
 *  High-precision versions of the above produced with an arbitrary
 * precision calculator (the compiler might lose a few bits in the FPU
 * operations).  These are specified to 81 bits of mantissa, which is
 * higher than any FPU known to me:
 */
constexpr double SQUASH  = 0.9966471893352525192801545;
/** @brief Nautical Miles in a Meter */
constexpr double SG_NM_TO_METER  = 1852.0000;
/** @brief Meters to Feet */
constexpr double SG_METER_TO_FEET  = 3.28083989501312335958;
constexpr double _EQURAD = 6378137.0;
constexpr double E2 = fabs(1 - SQUASH*SQUASH);
constexpr double ra2 = 1/(_EQURAD*_EQURAD);
constexpr double e2 = E2;
constexpr double e4 = E2*E2;
} // namespace

Point3D::Point3D
()
{
	clear ();
}

Point3D::Point3D
(
	const t_Point3D& x,
	const t_Point3D& y,
	const t_Point3D& z
)
{
	m_x = x;
	m_y = y;
	m_z = z;
}

void Point3D::set
(
	const t_Point3D& x,
	const t_Point3D& y,
        const t_Point3D& z
)
{
	m_x = x;
	m_y = y;
	m_z = z;
}

Point3D&
Point3D::operator +=
(
	const Point3D& p
)
{
	m_x += p.m_x;
	m_y += p.m_y;
	m_z += p.m_z;
	return *this;
}

Point3D&
Point3D::operator -=
(
	const Point3D& p
)
{
	m_x -= p.m_x;
	m_y -= p.m_y;
	m_z -= p.m_z;
	return *this;
}

Point3D&
Point3D::operator *=
(
	const Point3D& p
)
{
	m_x *= p.m_x;
	m_y *= p.m_y;
	m_z *= p.m_z;
	return *this;
}

Point3D&
Point3D::operator /=
(
	const Point3D& p
)
{
	m_x /= p.m_x;
	m_y /= p.m_y;
	m_z /= p.m_z;
	return *this;
}

Point3D&
Point3D::operator ^=
(
	const Point3D& p
)
{
	m_x = (m_y * p.m_z) - (m_z * p.m_y);
	m_y = (m_z * p.m_x) - (m_z * p.m_z);
	m_z = (m_x * p.m_y) - (m_z * p.m_x);
	return *this;

}

Point3D&
Point3D::operator *=
(
	t_Point3D v
)
{
	m_x *= v;
	m_y *= v;
	m_z *= v;
	return *this;
}

Point3D&
Point3D::operator /=
(
	t_Point3D v
)
{
	m_x /= v;
	m_y /= v;
	m_z /= v;
	return *this;
}

bool
Point3D::operator ==
(
	const Point3D& p
)
{
	if ((m_x == p.m_x) && (m_y == p.m_y) && (m_z == p.m_z))
		return (true);
	return (false);
}

bool
Point3D::operator !=
(
	const Point3D& p
)
{
	if ((m_x == p.m_x) && (m_y == p.m_y) && (m_z == p.m_z))
		return (false);
	return (true);
}

t_Point3D
Point3D::operator []
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

t_Point3D&
Point3D::operator []
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

t_Point3D
Point3D::length
() const
{
	return (sqrt ((m_x * m_x) + (m_y * m_y) + (m_z * m_z)));
}

void
Point3D::normalize
()
{
	t_Point3D len;

	len = length();
	m_x /= len;
	m_y /= len;
	m_z /= len;
}

t_Point3D
Point3D::sqr
()
{
	return ((m_x * m_x) + (m_y * m_y) + (m_z * m_z));
}

void
Point3D::invert
()
{
	m_x = -m_x;
	m_y = -m_y;
	m_z = -m_z;
}

void
Point3D::clear
()
{
	m_x = 0.0;
	m_y = 0.0;
	m_z = 0.0;
}

Point3D
invert
(
	const Point3D& p
)
{
	return (Point3D (
	    -p.m_x,
	    -p.m_y,
	    -p.m_z
	));
}

t_Point3D
sqr
(
	const Point3D& p
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
Point3D
normalize
(
	const Point3D& p
)
{
	t_Point3D len;

	len = p.length();
	return (Point3D (
	    (p.m_x / len),
	    (p.m_y / len),
	    (p.m_z / len)
	));
} // normalize ( const Point3D& P )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Return the length of P
 */
t_Point3D
length
(
	const Point3D& p
)
{
	return (sqrt (
	    (p.m_x * p.m_x) +
	    (p.m_y * p.m_y) +
	    (p.m_z * p.m_z)
	));
} // length ( const Point3D& P )

//////////////////////////////////////////////////////////////////////
/**
 * @brief Calculate distance of clients
 */
float
distance
(
	const Point3D & p1,
	const Point3D & p2
)
{
	Point3D p;

	p = p1 - p2;
	return (float)(p.length() / SG_NM_TO_METER);
} // distance ( const Point3D & P1, const Point3D & P2 )

//////////////////////////////////////////////////////////////////////
//
// Convert a cartexian XYZ coordinate to a geodetic lat/lon/alt.
// This function is a copy of what's in SimGear,
//  simgear/math/SGGeodesy.cxx.
//
////////////////////////////////////////////////////////////////////////

/**
 * @brief Convert a cartexian XYZ coordinate to a geodetic lat/lon/alt.
 *        This function is a copy of what's in SimGear,
 *        simgear/math/SGGeodesy.cxx
 */
void
cart_to_geod
(
	const Point3D& cart_point,
	Point3D& geod_point
)
{
	// according to
	// H. Vermeille,
	// Direct transformation from geocentric to geodetic cordinates,
	// Journal of Geodesy (2002) 76:451-454
	double x = cart_point[Point3D::X];
	double y = cart_point[Point3D::Y];
	double z = cart_point[Point3D::Z];
	double XXpYY = x * x + y * y;
	double sqrtXXpYY = sqrt ( XXpYY );
	double p = XXpYY * ra2;
	double q = z * z * ( 1 - e2 ) * ra2;
	double r = 1 / 6.0 * ( p + q - e4);
	double s = e4 * p * q / ( 4 * r * r * r );
	double t = pow ( 1 + s + sqrt ( s * (2 + s ) ), 1 /3.0 );
	double u = r * ( 1 + t + 1 / t);
	double v = sqrt ( u * u + e4 * q );
	double w = e2 * ( u + v - q ) / ( 2 * v );
	double k = sqrt ( u + v + w * w ) - w;
	double D = k * sqrtXXpYY / ( k + e2 );
	geod_point[Point3D::LON] =
	  ( 2 * atan2 ( y, x + sqrtXXpYY ) ) * SG_RADIANS_TO_DEGREES;
	double sqrtDDpZZ = sqrt ( D * D + z * z );
	geod_point[Point3D::LAT] =
	  ( 2 * atan2 ( z, D + sqrtDDpZZ ) ) * SG_RADIANS_TO_DEGREES;
	geod_point[Point3D::ALT] = 
	  ( ( k + e2 - 1 ) * sqrtDDpZZ / k ) * SG_METER_TO_FEET;
} // sgCartToGeod()
//////////////////////////////////////////////////////////////////////
