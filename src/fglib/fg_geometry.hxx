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

namespace fgmp
{

using point3d_t = double;

class point3d
{
public:
	enum { X, Y, Z };
	enum { LAT, LON, ALT };
	point3d ();
	point3d ( const point3d_t& x, const point3d_t& y, const point3d_t& z );
	point3d_t get_x () { return m_x; };
	point3d_t get_y () { return m_y; };
	point3d_t get_z () { return m_z; };
	void set_x ( const point3d_t& v ) { m_x = v; };
	void set_y ( const point3d_t& v ) { m_y = v; };
	void set_z ( const point3d_t& v ) { m_z = v; };
	void set ( const point3d_t& x, const point3d_t& y, const point3d_t& z );
	//////////////////////////////////////////////////
	//  operators
	//////////////////////////////////////////////////
	point3d& operator += ( const point3d& p );
	point3d& operator -= ( const point3d& p );
	point3d& operator *= ( const point3d& p );
	point3d& operator /= ( const point3d& p );
	point3d& operator ^= ( const point3d& p );
	point3d& operator *= ( point3d_t v );
	point3d& operator /= ( point3d_t v );
	bool operator == ( const point3d& p );
	bool operator != ( const point3d& p );
	point3d_t operator[] ( const int index ) const;
	point3d_t& operator[] ( const int index );
	friend point3d operator + ( point3d p1, const point3d& p2 )
	{
		return p1 += p2;
	}
	friend point3d operator - ( point3d p1, const point3d& p2 )
	{
		return p1 -= p2;
	}
	friend point3d operator * ( point3d p1, const point3d& p2 )
	{
		return p1 *= p2;
	}
	friend point3d operator / ( point3d p1, const point3d& p2 )
	{
		return p1 /= p2;
	}
	friend point3d operator ^ ( point3d p1, const point3d& p2 )
	{
		return p1 ^= p2;
	}
	friend point3d operator * ( point3d p, point3d_t v )
	{
		return p *= v;
	}
	friend point3d operator / ( point3d p, point3d_t v )
	{
		return p /= v;
	}
	//////////////////////////////////////////////////
	//  others
	//////////////////////////////////////////////////
	point3d_t length () const;
	friend point3d_t length ( const point3d& P );
	void normalize ();
	friend point3d normalize ( const point3d& P );
	point3d_t sqr ();
	friend point3d_t sqr ( const point3d& P );
	void invert ();
	friend point3d invert (const point3d& P );
	void clear ();

private:
	point3d_t m_x;
	point3d_t m_y;
	point3d_t m_z;
}; // class point3d

float distance ( const point3d & P1, const point3d & P2 );
void cart_to_geod ( const point3d& CartPoint , point3d& GeodPoint );

} // namespace fgmp

#endif

