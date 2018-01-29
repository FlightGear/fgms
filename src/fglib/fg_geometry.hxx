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


class Point3D
{
public:
	enum { X, Y, Z };
	enum { LAT, LON, ALT };
	Point3D();
	Point3D ( const t_Point3D& x, const t_Point3D& y, const t_Point3D& z );
	t_Point3D get_x () { return m_x; };
	t_Point3D get_y () { return m_y; };
	t_Point3D get_z () { return m_z; };
	void set_x ( const t_Point3D& v ) { m_x = v; };
	void set_y ( const t_Point3D& v ) { m_y = v; };
	void set_z ( const t_Point3D& v ) { m_z = v; };
	void set ( const t_Point3D& x, const t_Point3D& y, const t_Point3D& z );
	//////////////////////////////////////////////////
	//  operators
	//////////////////////////////////////////////////
	Point3D& operator += ( const Point3D& p );
	Point3D& operator -= ( const Point3D& p );
	Point3D& operator *= ( const Point3D& p );
	Point3D& operator /= ( const Point3D& p );
	Point3D& operator ^= ( const Point3D& p );
	Point3D& operator *= ( t_Point3D v );
	Point3D& operator /= ( t_Point3D v );
	bool operator == ( const Point3D& p );
	bool operator != ( const Point3D& p );
	t_Point3D operator[] ( const int index ) const;
	t_Point3D& operator[] ( const int index );
	friend Point3D operator + ( Point3D p1, const Point3D& p2 )
	{
		return p1 += p2;
	}
	friend Point3D operator - ( Point3D p1, const Point3D& p2 )
	{
		return p1 -= p2;
	}
	friend Point3D operator * ( Point3D p1, const Point3D& p2 )
	{
		return p1 *= p2;
	}
	friend Point3D operator / ( Point3D p1, const Point3D& p2 )
	{
		return p1 /= p2;
	}
	friend Point3D operator ^ ( Point3D p1, const Point3D& p2 )
	{
		return p1 ^= p2;
	}
	friend Point3D operator * ( Point3D p, t_Point3D v )
	{
		return p *= v;
	}
	friend Point3D operator / ( Point3D p, t_Point3D v )
	{
		return p /= v;
	}
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
	t_Point3D m_x;
	t_Point3D m_y;
	t_Point3D m_z;
}; // class Point3D

float distance ( const Point3D & P1, const Point3D & P2 );
void cart_to_geod ( const Point3D& CartPoint , Point3D& GeodPoint );

#endif

