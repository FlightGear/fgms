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
#include <limits>
#include "fg_geometry.hxx"
#include "fg_util.hxx"

namespace fgmp
{

//////////////////////////////////////////////////////////////////////

point3d::point3d
(): m_x { 0.0 }, m_y { 0.0 }, m_z { 0.0 }
{
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
        assert (! (index < 0 || index > 2));
        switch (index)
        {
                case 0: return m_x;
                case 1: return m_y;
                case 2: return m_z;
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
        assert (! (index < 0 || index > 2));
        switch (index)
        {
                case 0: return m_x;
                case 1: return m_y;
                case 2: return m_z;
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

namespace
{
constexpr double SG_NM_TO_METER         { 1852.0000 };
constexpr double SG_METER_TO_FEET       { 3.28083989501312335958 };
constexpr double SQUASH                 { 0.9966471893352525192801545 };

#ifdef _MSC_VER
        #define E2 fabs ( 1.0 - SQUASH * SQUASH )
        #define E4 ( E2 * E2 )
#else
constexpr double E2                     { fabs ( 1 - SQUASH * SQUASH ) };
constexpr double E4                     { E2 * E2 };
#endif
constexpr double EQURAD                 { 6378137.0 };
constexpr double RA2                    { 1 / (EQURAD * EQURAD) };
constexpr double SG_180                 { 180.0 };
constexpr double SG_PI                  { 3.1415926535 };
constexpr double SG_RADIANS_TO_DEGREES  { SG_180 / SG_PI };
constexpr double SG_DEGREES_TO_RADIANS  { SG_PI / SG_180 };
constexpr float  SGMISC_PI              { 3.1415926535 };
 //{ 3.1415926535897932384626433832795029L };
} // namespace

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
        double x = cart_point.x();
        double y = cart_point.y();
        double z = cart_point.z();
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
        geod_point.lon() =
          ( 2 * atan2 ( y, x + sqrtXXpYY ) ) * SG_RADIANS_TO_DEGREES;
        double sqrtDDpZZ = sqrt ( D * D + z * z );
        geod_point.lat() =
          ( 2 * atan2 ( z, D + sqrtDDpZZ ) ) * SG_RADIANS_TO_DEGREES;
        geod_point.alt() = 
          ( ( k + E2 - 1 ) * sqrtDDpZZ / k ) * SG_METER_TO_FEET;
} // sgCartToGeod()

//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////
// The code below is basically taken from
// simgears SGEuler.hxx and SGEuler.cxx but
// more c++-tified
//////////////////////////////////////////////////

namespace
{

using quat_t = double;

class quat
{
public:
        quat () {};
        quat ( quat_t x, quat_t y, quat_t z, quat_t w )
        {
                data[0] = x;
                data[1] = y;
                data[2] = z;
                data[3] = w;
        }
        inline quat_t x() const { return data[0]; };
        inline quat_t y() const { return data[1]; };
        inline quat_t z() const { return data[2]; };
        inline quat_t w() const { return data[3]; };
        inline quat_t & x() { return data[0]; };
        inline quat_t & y() { return data[1]; };
        inline quat_t & z() { return data[2]; };
        inline quat_t & w() { return data[3]; };
private:
        quat_t data[4];
};

//////////////////////////////////////////////////////////////////////

quat
operator *
(
        const quat & v1, const quat & v2
)
{
        quat v;
        v.x() = v1.w()*v2.x() + v1.x()*v2.w() + v1.y()*v2.z() - v1.z()*v2.y();
        v.y() = v1.w()*v2.y() - v1.x()*v2.z() + v1.y()*v2.w() + v1.z()*v2.x();
        v.z() = v1.w()*v2.z() + v1.x()*v2.y() - v1.y()*v2.x() + v1.z()*v2.w();
        v.w() = v1.w()*v2.w() - v1.x()*v2.x() - v1.y()*v2.y() - v1.z()*v2.z();
        return v;
} // operator * (quat, quat)

//////////////////////////////////////////////////////////////////////

quat
from_real_imag
(
        quat_t r,
        const point3d & i
)
{
        quat q;
        q.w() = r;
        q.x() = i.x();
        q.y() = i.y();
        q.z() = i.z();
        return q;
} // from_real_imag ()

//////////////////////////////////////////////////////////////////////

quat
unit
()
{
        return from_real_imag (1, point3d());
} // unit()

quat
conj
(
        const quat& v
)
{
        return { -v.x(), -v.y(), -v.z(), v.w() };
} // conj ()

//////////////////////////////////////////////////////////////////////

quat
from_lon_lat_rad
(
        quat_t lon,
        quat_t lat
)
{
        quat_t zd2 { 0.5f * lon };
        quat_t yd2 { -0.25f * SGMISC_PI - 0.5f * lat };
        quat_t Szd2 { sin(zd2) };
        quat_t Syd2 { sin(yd2) };
        quat_t Czd2 { cos(zd2) };
        quat_t Cyd2 { cos(yd2) };
        return { -Szd2 * Syd2, Czd2 * Syd2, Szd2 * Cyd2, Czd2 * Cyd2 };
} // from_lon_lat_rad ()

//////////////////////////////////////////////////////////////////////

quat
from_angle_axis
(
        const point3d & axis
)
{
        quat_t n_axis { length(axis) };
        if ( n_axis <= std::numeric_limits<quat_t>::min() )
        {
                return unit ();
        }
        quat_t angle { 0.5f * n_axis };
        return from_real_imag ( cos(angle), axis * (sin(angle) / n_axis));
} // from_angle_axis ()

//////////////////////////////////////////////////////////////////////

point3d
get_euler_rad
(
        const quat & v
)
{
        quat_t sqrQW { v.w() * v.w() };
        quat_t sqrQX { v.x() * v.x() };
        quat_t sqrQY { v.y() * v.y() };
        quat_t sqrQZ { v.z() * v.z() };
        quat_t num   { 2 * (v.y() * v.z() + v.w() * v.x()) };
        quat_t den   { sqrQW - sqrQX - sqrQY + sqrQZ };
        point3d r;
        if (fgmp::absolute(den) <= std::numeric_limits<float>::min()
        &&  fgmp::absolute(num) <= std::numeric_limits<float>::min())
        {
                r.x() = 0;
        }
        else
        {
                r.x() = atan2 (num, den);
        }
        quat_t tmp = 2 * (v.x() * v.z() - v.w() * v.y());
        if ( tmp <= -1 )
        {
                r.y() = 0.5 * SGMISC_PI;
        }
        else if ( 1 <= tmp )
        {
                r.y() = 0.5 * SGMISC_PI;
        }
        else
        {
                r.y() = -asin ( tmp );
        }
        num = 2 * (v.x() * v.y() + v.w() * v.z());
        den = sqrQW + sqrQX - sqrQY - sqrQZ;
        if (fgmp::absolute(den) <= std::numeric_limits<float>::min()
        &&  fgmp::absolute(num) <= std::numeric_limits<float>::min())
        {
                r.z() = 0;
        }
        else
        {
                quat_t psi { atan2(num, den) };
                if ( psi < 0 )
                {
                        psi += 2 * SGMISC_PI;
                }
                r.z() = psi;
        }
        return r;
} // get_euler_deg ()

//////////////////////////////////////////////////////////////////////

point3d
get_euler_deg
(
        const quat & v
)
{
        point3d r { get_euler_rad (v) };
        r.x() *= SG_RADIANS_TO_DEGREES;
        r.y() *= SG_RADIANS_TO_DEGREES;
        r.z() *= SG_RADIANS_TO_DEGREES;
        return r;
} // get_euler_deg ()

//////////////////////////////////////////////////////////////////////

} // namespace

//////////////////////////////////////////////////////////////////////

point3d
euler_get
(
        const point3d & pos_geod, const point3d & orientation
)
{
        point3d angle_axis { orientation };
        quat  ec_orient { from_angle_axis (angle_axis) };
        quat_t lat_rad { pos_geod.lat() * SG_DEGREES_TO_RADIANS };
        quat_t lon_rad { pos_geod.lon() * SG_DEGREES_TO_RADIANS };
        quat  qEc2Hl { from_lon_lat_rad ( lon_rad, lat_rad ) };
        quat  hlOr { conj(qEc2Hl) * ec_orient };
        return get_euler_deg ( hlOr );
} // euler_get ()

//////////////////////////////////////////////////////////////////////

} // namespace fgmp

