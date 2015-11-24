<?php
/*variable setup*/

$var['port'] = 5000; /*Port to bind*/
$var['error_reporting_level'] = E_ALL; /*Set Error reporting level (E_ERROR, E_WARNING, E_NOTICE, E_ALL). Default E_ALL*/

/*Postgresql information*/
$var['postgre_conn']['host'] = ""; /*(Linux only: leave blank for using unix socket*/
$var['postgre_conn']['port'] = 5432;
$var['postgre_conn']['desc'] = "AC-SERVER";
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
$var['php_ver']=phpversion ();
$var['exitflag']=false;
$var['ping_interval']=10;/*check timeout interval*/

$message="FGTracker Version ".$var['fgt_ver']." in ".$var['os']." with PHP ".$var['php_ver'];
$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
	
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
$fgt_sql=new fgt_postgres();
$fgt_conn=new fgt_connection_mgr();

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
		/*Check the connected flag.*/
		if( $fgt_conn->close_connection($uuid)===true)
			continue;
		
		// read client input
		if( $fgt_conn->read_connection($uuid)===false)
			continue;
		
		/*Process the read buffer (if needed)*/
		if(strlen ($clients[$uuid]['read_buffer'])>2)
		{	
			$clients[$uuid]['last_reception']=time();
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
$fgt_conn->close_all_connection();
$fgt_error_report->terminate();
?>

