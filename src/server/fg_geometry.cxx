//                                                                              
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

#include <math.h>
#include <strings.h>
#include <assert.h>
#include "fg_geometry.hxx"

// High-precision versions of the above produced with an arbitrary
// precision calculator (the compiler might lose a few bits in the FPU
// operations).  These are specified to 81 bits of mantissa, which is
// higher than any FPU known to me:
static const double SQUASH  = 0.9966471893352525192801545;
static const double STRETCH = 1.0033640898209764189003079;
static const double POLRAD  = 6356752.3142451794975639668;

static const double SG_RAD_TO_NM  = 3437.7467707849392526;
static const double SG_NM_TO_METER  = 1852.0000;
static const double SG_METER_TO_FEET  = 3.28083989501312335958;
static const double SGD_PI_2    = 1.57079632679489661923;

Point3D::Point3D()
{
  clear ();
}

Point3D::Point3D ( const Point3D& P )
{
  m_X = P.m_X;
  m_Y = P.m_Y;
  m_Z = P.m_Z;
}

Point3D::Point3D ( const t_Point3D& X, const t_Point3D& Y, const t_Point3D& Z )
{
  m_X = X;
  m_Y = Y;
  m_Z = Z;
}

void Point3D::Set ( const t_Point3D& X, const t_Point3D& Y,
                    const t_Point3D& Z )
{
  m_X = X;
  m_Y = Y;
  m_Z = Z;
}

void
Point3D::CartToPolar()
{
  Point3D tmp (*this);
  t_Point3D sqr_X = m_X*m_X;
  t_Point3D sqr_Y = m_Y*m_Y;
  t_Point3D sqr_Z = m_Z*m_Z;

  m_X = atan2 (tmp[Y], tmp[X]);
  m_Y = SGD_PI_2 - atan2( sqrt(sqr_X + sqr_Y), tmp[Z]);
  m_Z = sqrt(sqr_X + sqr_Y + sqr_Z);
}

void
Point3D::PolarToCart()
{
  double dummy = cos (m_X) * m_Z;

  m_X = cos (m_Y) * dummy;
  m_Y = sin (m_Y) * dummy;
  m_Z = sin (m_X) * m_Z;
}

void Point3D::operator =  ( const Point3D& P )
{
  m_X = P.m_X;
  m_Y = P.m_Y;
  m_Z = P.m_Z;
}

void Point3D::operator =  ( const sgdVec3& P )
{
  m_X = P[0];
  m_Y = P[1];
  m_Z = P[2];
}

void Point3D::operator += ( const Point3D& P )
{
  m_X += P.m_X;
  m_Y += P.m_Y;
  m_Z += P.m_Z;
}

void Point3D::operator -= ( const Point3D& P )
{
  m_X -= P.m_X;
  m_Y -= P.m_Y;
  m_Z -= P.m_Z;
}
void Point3D::operator *= ( const Point3D& P )
{
  m_X *= P.m_X;
  m_Y *= P.m_Y;
  m_Z *= P.m_Z;
}
void Point3D::operator /= ( const Point3D& P )
{
  m_X /= P.m_X;
  m_Y /= P.m_Y;
  m_Z /= P.m_Z;
}
void Point3D::operator ^= ( const Point3D& P )
{
  m_X = (m_Y * P.m_Z) - (m_Z * P.m_Y);
  m_Y = (m_Z * P.m_X) - (m_Z * P.m_Z);
  m_Z = (m_X * P.m_Y) - (m_Z * P.m_X);

}
void Point3D::operator *= ( const t_Point3D& nV )
{
  m_X *= nV;
  m_Y *= nV;
  m_Z *= nV;
}
void Point3D::operator /= ( const t_Point3D& nV )
{
  m_X /= nV;
  m_Y /= nV;
  m_Z /= nV;
}
bool Point3D::operator == ( const Point3D& P )
{
  if ((m_X == P.m_X) && (m_Y == P.m_Y) && (m_Z == P.m_Z))
    return (true);
  return (false);
}
bool Point3D::operator != ( const Point3D& P )
{
  if ((m_X == P.m_X) && (m_Y == P.m_Y) && (m_Z == P.m_Z))
    return (false);
  return (true);
}
t_Point3D Point3D::operator[] ( const int Index ) const
{
  assert (! (Index < X || Index > Z));
  switch (Index)
  {
  case X:
    return m_X;
  case Y:
    return m_Y;
  case Z:
    return m_Z;
  }
  return (0.0); // never reached
}
t_Point3D& Point3D::operator[] ( const int Index )
{
  assert (! (Index < X || Index > Z));
  switch (Index)
  {
  case X:
    return m_X;
  case Y:
    return m_Y;
  case Z:
    return m_Z;
  }
  return (m_X);
}


t_Point3D Point3D::length () const
{
  return (sqrt ((m_X * m_X) + (m_Y * m_Y) + (m_Z * m_Z)));
}

void Point3D::normalize ()
{
  t_Point3D len;

  len = length();
  m_X /= len;
  m_Y /= len;
  m_Z /= len;
}

t_Point3D Point3D::sqr ()
{
  return ((m_X * m_X) + (m_Y * m_Y) + (m_Z * m_Z));
}
void Point3D::invert ()
{
  m_X = -m_X;
  m_Y = -m_Y;
  m_Z = -m_Z;
}
void Point3D::clear ()
{
  m_X = 0.0;
  m_Y = 0.0;
  m_Z = 0.0;
}

Point3D operator + ( const Point3D& P1, const Point3D& P2 )
{
  return (Point3D (
            P1.m_X + P2.m_X,
            P1.m_Y + P2.m_Y,
            P1.m_Z + P2.m_Z
          ));
}
Point3D operator - ( const Point3D& P1, const Point3D& P2 )
{
  return (Point3D (
            P1.m_X - P2.m_X,
            P1.m_Y - P2.m_Y,
            P1.m_Z - P2.m_Z
          ));
}
Point3D operator * ( const Point3D& P1, const Point3D& P2 )
{
  return (Point3D (
            P1.m_X * P2.m_X,
            P1.m_Y * P2.m_Y,
            P1.m_Z * P2.m_Z
          ));
}
Point3D operator / ( const Point3D& P1, const Point3D& P2 )
{
  return (Point3D (
            P1.m_X / P2.m_X,
            P1.m_Y / P2.m_Y,
            P1.m_Z / P2.m_Z
          ));
}
Point3D operator ^ ( const Point3D& P1, const Point3D& P2 )
{
  return (Point3D (
            (P1.m_Y * P2.m_Z) - (P1.m_Z * P2.m_Y),
            (P1.m_Z * P2.m_X) - (P1.m_Z * P2.m_Z),
            (P1.m_X * P2.m_Y) - (P1.m_Z * P2.m_X)
          ));

}
Point3D operator * ( const t_Point3D& nV, const Point3D& P1 )
{
  return (Point3D (
            P1.m_X * nV,
            P1.m_Y * nV,
            P1.m_Z * nV
          ));
}
Point3D operator / ( const t_Point3D& nV, const Point3D& P1 )
{
  return (Point3D (
            P1.m_X / nV,
            P1.m_Y / nV,
            P1.m_Z / nV
          ));
}
Point3D invert (const Point3D& P )
{
  return (Point3D (
            -P.m_X,
            -P.m_Y,
            -P.m_Z
          ));
}
t_Point3D sqr ( const Point3D& P )
{
  return (
           (P.m_X * P.m_X) +
           (P.m_Y * P.m_Y) +
           (P.m_Z * P.m_Z)
         );
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  normalize P
//
//////////////////////////////////////////////////////////////////////
Point3D
normalize ( const Point3D& P )
{
  t_Point3D len;

  len = P.length();
  return (Point3D (
            (P.m_X / len),
            (P.m_Y / len),
            (P.m_Z / len)
          ));
} // normalize ( const Point3D& P )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  return the length of P
//
//////////////////////////////////////////////////////////////////////
t_Point3D
length ( const Point3D& P )
{
  return (sqrt (
            (P.m_X * P.m_X) +
            (P.m_Y * P.m_Y) +
            (P.m_Z * P.m_Z)
          ));
} // length ( const Point3D& P )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  calculate distance of clients
//
//////////////////////////////////////////////////////////////////////
float
Distance ( const Point3D & P1, const Point3D & P2 )
{
  Point3D P;

  P = P1 - P2;
  return (P.length() / SG_NM_TO_METER);
} // Distance ( const Point3D & P1, const Point3D & P2 )
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
//  Returns a "local" geodetic latitude: an approximation that
//  will be correct only at zero altitude.
//
//////////////////////////////////////////////////////////////////////
#if 0   // not used
static double
localLat (double r, double z )
{
  // Squash to a spherical earth, compute a tangent vector to the
  // surface circle (in squashed space, the surface is a perfect
  // sphere) by swapping the components and negating one, stretch to
  // real coordinates, and take an inverse-tangent/perpedicular
  // vector to get a local geodetic "up" vector.  (Those steps all
  // cook down to just a few multiplies).  Then just turn it into an
  // angle.
  double upr = r * SQUASH;
  double upz = z * STRETCH;
  return atan2(upz, upr);
} // localLat ()
//////////////////////////////////////////////////////////////////////
#endif

//////////////////////////////////////////////////////////////////////
//
//  This is the inverse of the algorithm in localLat().  It
//  returns the (cylindrical) coordinates of a surface latitude
//  expressed as an "up" unit vector.
//
//////////////////////////////////////////////////////////////////////
static void
surfRZ (double upr, double upz, double* r, double* z)
{
  // We are
  // converting a (2D, cylindrical) "up" vector defined by the
  // geodetic latitude into unitless R and Z coordinates in
  // cartesian space.
  double R = upr * STRETCH;
  double Z = upz * SQUASH;
  // Now we need to turn R and Z into a surface point.  That is,
  // pick a coefficient C for them such that the point is on the
  // surface when converted to "squashed" space:
  //  (C*R*SQUASH)^2 + (C*Z)^2 = POLRAD^2
  //   C^2 = POLRAD^2 / ((R*SQUASH)^2 + Z^2)
  double sr = R * SQUASH;
  double c = POLRAD / sqrt(sr*sr + Z*Z);

  R *= c;
  Z *= c;
  *r = R;
  *z = Z;
} // surfRZ()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
// Convert a cartexian XYZ coordinate to a geodetic lat/lon/alt.
// This function is a copy of what's in SimGear,
//  simgear/math/SGGeodesy.cxx.
//
////////////////////////////////////////////////////////////////////////

#define _EQURAD     (6378137.0)
#define E2 fabs(1 - SQUASH*SQUASH)
static double ra2 = 1/(_EQURAD*_EQURAD);
static double e2 = E2;
static double e4 = E2*E2;

void
sgCartToGeod ( const Point3D& CartPoint , Point3D& GeodPoint )
{
  // according to
  // H. Vermeille,
  // Direct transformation from geocentric to geodetic ccordinates,
  // Journal of Geodesy (2002) 76:451-454
  double x = CartPoint[X];
  double y = CartPoint[Y];
  double z = CartPoint[Z];
  double XXpYY = x*x+y*y;
  double sqrtXXpYY = sqrt(XXpYY);
  double p = XXpYY*ra2;
  double q = z*z*(1-e2)*ra2;
  double r = 1/6.0*(p+q-e4);
  double s = e4*p*q/(4*r*r*r);
  double t = pow(1+s+sqrt(s*(2+s)), 1/3.0);
  double u = r*(1+t+1/t);
  double v = sqrt(u*u+e4*q);
  double w = e2*(u+v-q)/(2*v);
  double k = sqrt(u+v+w*w)-w;
  double D = k*sqrtXXpYY/(k+e2);
  GeodPoint[Lon] = (2*atan2(y, x+sqrtXXpYY)) * SG_RADIANS_TO_DEGREES;
  double sqrtDDpZZ = sqrt(D*D+z*z);
  GeodPoint[Lat] = (2*atan2(z, D+sqrtDDpZZ)) * SG_RADIANS_TO_DEGREES;
  GeodPoint[Alt] = ((k+e2-1)*sqrtDDpZZ/k) * SG_METER_TO_FEET;
} // sgCartToGeod()
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//
// opposite of sgCartToGeod
//
//////////////////////////////////////////////////////////////////////
void sgGeodToCart ( double lat, double lon, double alt, double* xyz )
{
  // This is the inverse of the algorithm in localLat().  We are
  // converting a (2D, cylindrical) "up" vector defined by the
  // geodetic latitude into unitless R and Z coordinates in
  // cartesian space.
  double upr = cos(lat);
  double upz = sin(lat);
  double r, z;
  surfRZ(upr, upz, &r, &z);

  // Add the altitude using the "up" unit vector we calculated
  // initially.
  r += upr * alt;
  z += upz * alt;

  // Finally, convert from cylindrical to cartesian
  xyz[0] = r * cos(lon);
  xyz[1] = r * sin(lon);
  xyz[2] = z;
} // sgGeodToCart()
//////////////////////////////////////////////////////////////////////

// vim: ts=2:sw=2:sts=0

