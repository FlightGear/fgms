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

#if !defined FG_GEOMETRY_H
#define FG_GEOMETRY_H

#include <simgear/math/SGMath.hxx>

#define SG_180 180.0
#define SG_PI 3.1415926535
#define SG_RADIANS_TO_DEGREES (SG_180/SG_PI)
#define SG_DEGREES_TO_RADIANS (SG_PI/SG_180)
#define SG_FEET_TO_METER    0.3048

typedef double t_Point3D;

enum { X, Y, Z };
enum { Lat, Lon, Alt };

class Point3D
{
	public:
	Point3D();
	Point3D ( const Point3D& P );
	Point3D ( const t_Point3D& X, const t_Point3D& Y, const t_Point3D& Z );
	t_Point3D GetX () { return m_X; };
	t_Point3D GetY () { return m_Y; };
	t_Point3D GetZ () { return m_Z; };
	void SetX ( const t_Point3D& nV ) { m_X = nV; };
	void SetY ( const t_Point3D& nV ) { m_Y = nV; };
	void SetZ ( const t_Point3D& nV ) { m_Z = nV; };
	void Set ( const t_Point3D& X, const t_Point3D& Y, const t_Point3D& Z );
	void CartToPolar();
	void PolarToCart();
	//////////////////////////////////////////////////
	//  operators
	//////////////////////////////////////////////////
	void operator =  ( const Point3D& P );
	void operator =  ( const sgdVec3& P );
	void operator += ( const Point3D& P );
	void operator -= ( const Point3D& P );
	void operator *= ( const Point3D& P );
	void operator /= ( const Point3D& P );
	void operator ^= ( const Point3D& P );
	void operator *= ( const t_Point3D& nV );
	void operator /= ( const t_Point3D& nV );
	bool operator == ( const Point3D& P );
	bool operator != ( const Point3D& P );
	t_Point3D operator[] ( const int Index ) const;
	t_Point3D& operator[] ( const int Index );
	friend Point3D operator + ( const Point3D& P1, const Point3D& P2 );
	friend Point3D operator - ( const Point3D& P1, const Point3D& P2 );
	friend Point3D operator * ( const Point3D& P1, const Point3D& P2 );
	friend Point3D operator / ( const Point3D& P1, const Point3D& P2 );
	friend Point3D operator ^ ( const Point3D& P1, const Point3D& P2 );
	friend Point3D operator * ( const t_Point3D& nV, const Point3D& P1 );
	friend Point3D operator / ( const t_Point3D& nV, const Point3D& P1 );
	//////////////////////////////////////////////////
	//  others
	//////////////////////////////////////////////////
	t_Point3D length () const;
	friend t_Point3D length ( const Point3D& P );
	void normalize ();
	friend Point3D normalize ( const Point3D& P );
	t_Point3D sqr ();
	friend t_Point3D sqr ( const Point3D& P );
	void invert ();
	friend Point3D invert (const Point3D& P );
	void clear ();

	private:
	t_Point3D m_X;
	t_Point3D m_Y;
	t_Point3D m_Z;
}; // class Point3D

void CopyPos (  const Point3D& src, Point3D &dst );
void Mat4ToCoord ( const sgMat4& src,  Point3D & dst );
float Distance ( const Point3D & P1, const Point3D & P2 );
float HeightAboveSea ( const Point3D & P );
void sgCartToPolar3d(const Point3D& cp, Point3D& Polar );
void CartToLatLon ( const Point3D& CartPoint , Point3D& LatLonAlt );
double
calc_gc_dist ( const Point3D& start, const Point3D& dest );

bool IsWithinRMiles ( double lat1, double lon1, double lat2, double lon2, 
    double R );

void sgCartToGeod ( const Point3D& CartPoint , Point3D& GeodPoint );
void sgGeodToCart(double lat, double lon, double alt, double* xyz);

#endif

