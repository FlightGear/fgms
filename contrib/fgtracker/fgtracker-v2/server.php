<?php
/*
FGTracker Version 2.0INCOMPLETE

Author								: Hazuki Amamiya <FlightGear forum nick Hazuki>
License								: GPL
OS requirement 						: Linux 
DB requirement						: PostgreSQL v8 or above
PHP requirement						: PHP 5.1 or above (With php-cli module installed)
Developed and tested under this env	: Debian 8.2/php 5.6.14+dfsg-0+deb8u1/PostgreSQL 9.4.5-0+deb8u1

Current version (Version 2.0INCOMPLETE) does NOT support any version of fgms. When it is done, it 
will support the following fgms:
v0.10 : v0.10.23 and above
v0.11 :	v0.11.6 and above

NOTICE to Windows user
This program should be able to run in Windows environemt. However, the exit routine is not 
implemented because of lack of signal handling (SIGINT). Unless at the time of quit the sockets are
idle, otherwise data discrepancy may occur.
*/

/*variable setup*/
$var['port'] = 8000; /*Port to bind*/
$var['error_reporting_level'] = E_ALL; /*Set Error reporting level (E_ERROR, E_WARNING, E_NOTICE, E_ALL). Default E_ALL*/

/*Postgresql information*/
$var['postgre_conn']['host'] = ""; /*(Linux only: empty sting for using unix socket*/
$var['postgre_conn']['port'] = 5432; /*(Linux only: lgnored if using unix socket*/
$var['postgre_conn']['desc'] = "AC-VSERVER";
$var['postgre_conn']['uname'] = "fgtracker";
$var['postgre_conn']['pass'] = "fgtracker";
$var['postgre_conn']['db'] = "fgtracker";

/*Do not amend below unless in development*/

if(!defined('MSG_DONTWAIT')) define('MSG_DONTWAIT', 0x40);
set_time_limit(0);

require("fgt_error_report.php");
$fgt_error_report=new fgt_error_report();

$var['os'] = strtoupper(PHP_OS);
$var['fgt_ver']="2.0INCOMPLETE";
$var['min_php_ver']='5.1';
$var['exitflag']=false;
$var['ping_interval']=60;/*check timeout interval. Default(=60)*/

$message="FGTracker Version ".$var['fgt_ver']." in ".$var['os']." with PHP ".PHP_VERSION;
$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);

if (version_compare(PHP_VERSION, $var['min_php_ver'], '<')) {
	$message="PHP is not new enough to support FGTracker. FGTracker is now exiting";
	$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
	return;
}
	
if(substr($var['os'],0,3) != "WIN")
{
	declare(ticks = 1); /*required by signal handler*/
	define('IS_WINDOWS', false);
	require("signal.php");
}else
	define('IS_WINDOWS', true);

require("fgt_read_NOWAIT.php");
require("fgt_ident.php");
require("fgt_postgres.php");
require("fgt_msg_process.php");
require("fgt_connection_mgr.php");

$fgt_ident=new fgt_ident();
$fgt_conn=NULL; /*to be called by $fgt_sql->connectmaster*/
$fgt_sql=new fgt_postgres();


$clients=Array();
while (1)
{
	/*$clients[$uuid]=Array(
	[res socket],
	[bool connected],
	[bool identified],
	[str protocal_version],
	[str server_ident],
	[str read_buffer],
	[class read_class],
	[class msg_process_class],
	[int last_reception],
	[int timeout_stage],
	[str write_buffer])
	*/
	
	// accept incoming connections
	$fgt_conn->accept_connection();
	
	foreach($clients as $uuid=>$client)
	{
		/*Check the connection*/
		if ($fgt_sql->connectmaster()===true)
			break;
		
		if( $fgt_conn->close_connection($uuid)===true)
			continue;
		
		/*Read client input*/
		if( $fgt_conn->read_connection($uuid)===false)
			continue;
		
		/*Process the read buffer (if needed)*/
		//print strlen ($clients[$uuid]['read_buffer'])."-";
		if(strlen ($clients[$uuid]['read_buffer'])>2)
		{
			$clients[$uuid]['last_reception']=time();
			$clients[$uuid]['timeout_stage']=0;
			if($client['identified']===false)
				$fgt_ident->check_ident($uuid);
			else $clients[$uuid]['read_class']->read_buffer();
		}
		
		/*check timeout*/
		$fgt_conn->check_timeout($uuid);
		
		/*Process the write buffer*/
		$fgt_conn->write_connection($uuid);
		
	}
	if($var['exitflag']===true)
	{
		$message="Exiting";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
		break;
	}
	usleep(200000);
}
// close sockets
$fgt_conn->close_all_connections();
$fgt_error_report->terminate();
?>

