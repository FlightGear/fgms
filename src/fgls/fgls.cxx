//
// Copyright (C) Oliver Schroeder <fgms@postrobot.de>
//
// This file is part of fgms, the flightgear multiplayer server
//
// fgms is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// fgms is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with fgms.  If not, see <http://www.gnu.org/licenses/>
//

///
/// @file fgls.cxx
/// flightgear list server
///
/// @author     Oliver Schroeder <fgms@o-schroeder.de>
/// @date       2017
/// @copyright  GPLv2
///
/// This is the list server for the flightgear multiplayer
/// network. All servers register at this server. All clients ask FGLS
/// which server they should use.
///

#include <fglib/fg_log.hxx>
#include "fgls.hxx"

const FG_VERSION FGLS::m_version ( 1, 0, 0, "-dev" );

//////////////////////////////////////////////////////////////////////

server::server
()
{
	id		= ( size_t ) -1;
	type		= fgms::eCLIENTTYPES::UNSET;
	//name	
	//location
	//admin
	//version
	last_seen	= 0;
	registered_at	= 0;

} // server::server()

//////////////////////////////////////////////////////////////////////

std::ostream&
operator <<
(
	std::ostream& o,
	const server& s
)
{
	o << s.name
	  << " " << s.location
	  << " " << s.addr
	  << " (ID " << s.id << ")";
	return o;
}; // operator << ( server )

//////////////////////////////////////////////////////////////////////

FGLS::FGLS
()
{
	m_BindAddress	= "";
	// the main data channel
	m_DataPort	= 5004;
	m_ReinitData	= true;
	m_DataChannel	= 0;
	// the query channel
	m_QueryPort	= m_DataPort + 1;
	m_ReinitQuery	= true;
	m_QueryChannel	= 0;
	// the admin channel
	m_AdminPort	= m_DataPort + 2;
	m_ReinitAdmin	= true;
	m_AdminChannel	= 0;
	m_AdminCLI	= true;
	// logging
	m_LogFileName	= "";
	m_ReinitLog	= true;
	m_DebugLevel	= log::MEDIUM;
	// general
	m_IsParent	= false;
} // FGLS::FGLS()

//////////////////////////////////////////////////////////////////////
/**
 * @brief Basic initialisation
 * @return true OK
 * @return false on error
 */
bool
FGLS::Init
()
{
	//
	// enable logging
	//
	if ( m_LogFileName != "" )
	{
		OpenLogfile ();
	}
	logger.priority ( log::MEDIUM );
	logger.flags ( log::WITH_DATE );
	//
	// enable data channel
	//
	if ( ! InitDataChannel () )
		return false;
	//
	// enable query channel
	//
	if ( ! InitQueryChannel () )
		return false;
	//
	// enable admin channel
	//
	if ( ! InitAdminChannel () )
		return false;
	return 0;
} // FGLS::Init ()

//////////////////////////////////////////////////////////////////////

void
FGLS::Loop
()
{
	m_IsParent = true;
} // FGLS::Loop ()

//////////////////////////////////////////////////////////////////////
/** Initialise the data channel
 * @return true OK
 * @return false something went wrong
 */
bool
FGLS::InitDataChannel
()
{
	if ( ! m_ReinitData )
		return true;
	if ( m_DataChannel )
	{
		delete m_DataChannel;
		m_DataChannel = 0;
	}
	m_DataChannel = new NetSocket ();
	if ( m_DataChannel->Open ( NetSocket::UDP ) == 0 )
	{
		LOG ( log::URGENT, "FGMS::Init() - "
		  << "failed to create data socket" );
		return false;
	}
	m_DataChannel->SetBlocking ( false );
	m_DataChannel->SetSocketOption ( SO_REUSEADDR, true );
	if ( ! m_DataChannel->Bind ( m_BindAddress, m_DataPort ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to bind on " << m_BindAddress
		  << ":" << m_DataPort );
		LOG ( log::URGENT, "already in use?" );
		return false;
	}
	m_ReinitData = false;
	return true;
} // FGLS::InitDataChannel()

//////////////////////////////////////////////////////////////////////
/** Initialise the query channel
 * @return true OK
 * @return false something went wrong
 */
bool
FGLS::InitQueryChannel
()
{
	if ( ! m_ReinitQuery )
		return true;
	if ( m_QueryChannel )
	{
		delete m_QueryChannel;
	}
	m_QueryChannel = 0;
	if ( m_QueryPort == 0 )
	{
		m_ReinitQuery = false;
		return true; // query channel disabled
	}
	m_QueryChannel = new NetSocket ();
	if ( m_QueryChannel->Open ( NetSocket::TCP ) == 0 )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to create Query socket" );
		return false;
	}
	m_QueryChannel->SetBlocking ( false );
	m_QueryChannel->SetSocketOption ( SO_REUSEADDR, true );
	if ( ! m_QueryChannel->Bind ( m_BindAddress, m_QueryPort ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to bind on " << m_BindAddress
		  << ":" << m_QueryPort );
		LOG ( log::URGENT, "already in use?" );
		return false;
	}
	if ( ! m_QueryChannel->Listen ( MAX_TELNETS ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to listen to query port" );
		return false;
	}
	m_ReinitQuery = false;
	return true;
} // FGLS::InitQueryChannel()

//////////////////////////////////////////////////////////////////////
/** Initialise the admin channel
 * @return true OK
 * @return false something went wrong
 */
bool
FGLS::InitAdminChannel
()
{
	if ( ! m_ReinitAdmin )
		return true;
	if ( m_AdminChannel )
	{
		delete m_AdminChannel;
	}
	m_AdminChannel = 0;
	if ( m_AdminCLI == false )
		return true;
	if ( m_AdminPort == 0 )
	{
		m_ReinitAdmin = false;
		return true; // query channel disabled
	}
	m_AdminChannel = new NetSocket ();
	if ( m_AdminChannel->Open ( NetSocket::TCP ) == 0 )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to create Admin socket" );
		return false;
	}
	m_AdminChannel->SetBlocking ( false );
	m_AdminChannel->SetSocketOption ( SO_REUSEADDR, true );
	if ( ! m_AdminChannel->Bind ( m_BindAddress, m_AdminPort ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to bind on " << m_BindAddress
		  << ":" << m_AdminPort );
		LOG ( log::URGENT, "already in use?" );
		return false;
	}
	if ( ! m_AdminChannel->Listen ( MAX_TELNETS ) )
	{
		LOG ( log::URGENT, "FGLS::Init() - "
		  << "failed to listen to query port" );
		return false;
	}
	m_ReinitAdmin = false;
	return true;
} // FGLS::InitAdminChannel()

//////////////////////////////////////////////////////////////////////

void
FGLS::OpenLogfile
()
{
	if ( m_ReinitLog == false )
		return;	// nothing to do
	if ( m_LogFileName == "" )
	{
		return;
	}
	LOG ( log::CONSOLE, "# using logfile " << m_LogFileName );
	if ( ! logger.open ( m_LogFileName ) )
	{
		LOG ( log::URGENT,
		  "FGLS::OpenLogfile(): "
		  << "Failed to open log file " << m_LogFileName );
	}
	m_ReinitLog = false;
} // FGLS::OpenLogfile ()

//////////////////////////////////////////////////////////////////////

void
FGLS::ShutDown
()
{
	if ( ! m_IsParent )
		return;
	LOG ( log::URGENT, "FGLS::Shutdown() - exiting" );
	logger.close ();
	if ( m_DataChannel )
	{
		m_DataChannel->Close ();
		delete m_DataChannel;
		m_DataChannel = 0;
	}
	if ( m_QueryChannel )
	{
		m_QueryChannel->Close ();
		delete m_QueryChannel;
		m_QueryChannel = 0;
	}
	if ( m_AdminChannel )
	{
		m_AdminChannel->Close ();
		delete m_AdminChannel;
		m_AdminChannel = 0;
	}
} // FGLS::ShutDown ()

//////////////////////////////////////////////////////////////////////

int
FGLS::ParseParams
(
	int argc,
	char* argv[]
)
{
	return 0;
} // FGLS::ParseParams()

//////////////////////////////////////////////////////////////////////

int
main
(
	int   argc,
	char* argv[]
)
{
	LOG(log::MEDIUM | log::CONSOLE, "# This is me" );
	return 0;
}
