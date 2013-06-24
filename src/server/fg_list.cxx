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

#include <time.h>
#include <simgear/debug/logstream.hxx>
#include <fg_list.hxx>

//////////////////////////////////////////////////////////////////////
FG_ListElement::FG_ListElement
(
	const string& Name
)
{
	ID		= -1;	// flag a non existant Element
	Timeout		= 0;
	this->Name	= Name;
	JoinTime	= time (0);
	LastSeen	= JoinTime;
	PktsSentTo	= 0;
	PktsRcvdFrom	= 0;
	BytesRcvdFrom	= 0;
	BytesSentTo	= 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_ListElement::FG_ListElement
()
{
	ID		= -1;	// flag a non existant Element
	Timeout		= 0;
	Name		= "";
	JoinTime	= time (0);
	LastSeen	= JoinTime;
	PktsSentTo	= 0;
	PktsRcvdFrom	= 0;
	BytesRcvdFrom	= 0;
	BytesSentTo	= 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_ListElement::FG_ListElement
(
	const FG_ListElement& P
)
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_ListElement::~FG_ListElement
()
{
	//SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_ListElement::~FG_ListElement(" << pthread_self() << ")");
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_ListElement::operator =
(
	const FG_ListElement& P
)
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
bool
FG_ListElement::operator ==
(
	const FG_ListElement& P
)
{
	if (Address == P.Address)
		return true;
	return false;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_ListElement::assign
(
	const FG_ListElement& P
)
{
	ID		= P.ID;
	Timeout		= P.Timeout;
        Name		= P.Name.c_str();
        Address         = P.Address;
        JoinTime        = P.JoinTime;
        LastSeen        = P.LastSeen;
        PktsSentTo      = P.PktsSentTo;
	BytesSentTo	= P.BytesSentTo;
        PktsRcvdFrom	= P.PktsRcvdFrom;
	BytesRcvdFrom	= P.BytesRcvdFrom;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_ListElement::UpdateSent
(
	size_t bytes
)
{
	PktsSentTo++;
	BytesSentTo += bytes;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_ListElement::UpdateRcvd
(
	size_t bytes
)
{
	PktsRcvdFrom++;
	BytesRcvdFrom += bytes;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::FG_Player
()
{
        Passwd          = "";
        ModelName       = "";
        JoinTime        = 0;
        LastSeen        = 0;
        Error           = "";
        HasErrors       = false;
        ClientID        = 0;
        LastRelayedToInactive   = 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::FG_Player
(
	const FG_Player& P
)
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_Player::~FG_Player
()
{
	//SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_Player::~FG_Player(" << pthread_self() << ") - " << this->Name);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_Player::operator =
(
	const FG_Player& P
)
{
	this->assign (P);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
bool
FG_Player::operator ==
(
	const FG_Player& P
)
{
	if ((Address == P.Address) && (Name == P.Name))
		return true;
	return false;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_Player::assign
(
	const FG_Player& P
)
{
	//SG_LOG (SG_SYSTEMS, SG_ALERT, "FG_Player::assign(" << pthread_self() << ") - " << P.Name);
	//
	// using str.c_str() here to prevent copy-on-write in std::string!
	//
	FG_ListElement::assign (P);
        Origin          = P.Origin.c_str();
        Passwd          = P.Passwd.c_str();
        ModelName       = P.ModelName.c_str();
        JoinTime        = P.JoinTime;
        LastSeen        = P.LastSeen ;
        LastPos         = P.LastPos;
        IsLocal         = P.IsLocal;
        Error           = P.Error.c_str();
        HasErrors       = P.HasErrors;
        ClientID        = P.ClientID;
        LastOrientation         = P.LastOrientation;
        LastRelayedToInactive   = P.LastRelayedToInactive;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_List::FG_List
(
	const string& Name
)
{
	pthread_mutex_init ( &m_ListMutex, 0 );
	this->Name = Name;
	PktsSent	= 0;
	BytesSent	= 0;
	PktsRcvd	= 0;
	BytesRcvd	= 0;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_List::~FG_List
()
{
	Clear ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
size_t
FG_List::Size
()
{
	int size;
	pthread_mutex_lock ( & m_ListMutex );
	size = Elements.size ();
	pthread_mutex_unlock ( & m_ListMutex );
	return size;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
size_t
FG_List::Add
(
	FG_ListElement& Element,
	time_t TTL
)
{
	this->MaxID++;
	Element.ID = this->MaxID;
	Element.Timeout = TTL;
	pthread_mutex_lock ( & m_ListMutex );
	Elements.push_back ( Element );
	pthread_mutex_unlock ( & m_ListMutex );
	return this->MaxID++;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
ItList
FG_List::Delete
(
	const ItList& Element
)
{
	ItList E;
	pthread_mutex_lock   ( & m_ListMutex );
	E = Elements.erase   ( Element );
	pthread_mutex_unlock ( & m_ListMutex );
	return (E);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
ItList
FG_List::Find
(
	const netAddress& Address,
	const string& Name
)
{
	ItList Element;
	ItList RetElem;

	this->LastRun = time (0);
	RetElem = Elements.end();
	pthread_mutex_lock ( & m_ListMutex );
	Element = Elements.begin();
	while (Element != Elements.end())
	{
		if (Element->Address == Address)
		{
			if (Name != "") 
			{
				if (Element->Name == Name)
					RetElem = Element;
			}
			else
			{
				RetElem = Element;
			}
		}
		else
		{
			if (Element->Timeout == 0)
			{	// never times out
				Element++;
				continue;
			}
			if ( (this->LastRun - Element->LastSeen) > Element->Timeout )
			{
				SG_LOG ( SG_SYSTEMS, SG_INFO,
				  this->Name << ": TTL exceeded for "
				  << Element->Address.getHost() << " " << Element->Name);
				Element = Elements.erase (Element);
				continue;
			}
		}
		Element++;
	}
	pthread_mutex_unlock ( & m_ListMutex );
	return RetElem;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
ItList
FG_List::FindByID
(
	size_t ID
)
{
	ItList Element;
	ItList RetElem;
	this->LastRun = time (0);
	RetElem = Elements.end();
	pthread_mutex_lock ( & m_ListMutex );
	Element = Elements.begin();
	while (Element != Elements.end())
	{
		if (Element->ID == ID)
		{
			RetElem = Element;
			break;
		}
		Element++;
	}
	pthread_mutex_unlock ( & m_ListMutex );
	return RetElem;
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
ItList
FG_List::End
()
{
	return Elements.end ();
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
FG_ListElement
FG_List::operator []
(
	const size_t& Index
)
{
	FG_ListElement RetElem("");
	pthread_mutex_lock ( & m_ListMutex );
	if (Index < Elements.size ())
		RetElem = Elements[Index];
	pthread_mutex_unlock ( & m_ListMutex );
	return RetElem;
}
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
void
FG_List::UpdateSent
(
	ItList& Element,
	size_t bytes
)
{
	PktsSent++;
	BytesSent += bytes;
	Element->UpdateSent (bytes);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_List::UpdateRcvd
(
	ItList& Element,
	size_t bytes
)
{
	PktsRcvd++;
	BytesRcvd += bytes;
	Element->UpdateRcvd (bytes);
}
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void
FG_List::Clear
()
{
	Elements.clear ();
}
//////////////////////////////////////////////////////////////////////

