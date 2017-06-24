/**
 * @file fg_list.cxx
 * @author Oliver Schroeder
 */
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
// Copyright (C) 2005-2012  Oliver Schroeder
//

#ifdef HAVE_CONFIG_H
#include "config.h" // for MSVC, always first
#endif

#include <time.h>
#include "fg_list.hxx"

namespace fgmp
{

const size_t ListElement::NONE_EXISTANT = (size_t) -1;

//////////////////////////////////////////////////////////////////////
ListElement::ListElement( const string& Name )
{
	ID		= NONE_EXISTANT;	// flag a non existant Element
	Timeout		= 0;
	this->Name	= Name;
	JoinTime	= time (0);
	LastSeen	= JoinTime;
	LastSent	= JoinTime;
	PktsSent	= 0;
	PktsRcvd	= 0;
	BytesRcvd	= 0;
	BytesSent	= 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
ListElement::ListElement()
{
	ID		= NONE_EXISTANT;	// flag a non existant Element
	Timeout		= 0;
	Name		= "";
	JoinTime	= time (0);
	LastSeen	= JoinTime;
	LastSent	= JoinTime;
	PktsSent	= 0;
	PktsRcvd	= 0;
	BytesRcvd	= 0;
	BytesSent	= 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
ListElement::ListElement( const ListElement& P )
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
ListElement::~ListElement()
{
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
ListElement::operator =( const ListElement& P )
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
bool
ListElement::operator ==( const ListElement& P )
{
	// FIXME: compare the name, too?
	if (Address == P.Address)
		return true;
	return false;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
ListElement::assign( const ListElement& P )
{
	ID		= P.ID;
	Timeout		= P.Timeout;
        Name		= P.Name.c_str();
        Address 	= P.Address;
        JoinTime 	= P.JoinTime;
        LastSeen	= P.LastSeen;
        LastSent	= P.LastSent;
        PktsSent	= P.PktsSent;
	BytesSent	= P.BytesSent;
        PktsRcvd	= P.PktsRcvd;
	BytesRcvd	= P.BytesRcvd;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
ListElement::UpdateSent( size_t bytes )
{
	PktsSent++;
	BytesSent += bytes;
	LastSent = time(0);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
ListElement::UpdateRcvd
(
	size_t bytes
)
{
	PktsRcvd++;
	BytesRcvd += bytes;
	LastSeen = time(0);
}
//////////////////////////////////////////////////////////////////////

} // namespace fgmp
