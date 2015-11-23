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

$message="FGTrcker Version ".$var['fgt_ver']." in ".$var['os']." with PHP ".$var['php_ver'];
$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
	
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

$fgt_ident=new fgt_ident();

/*check connection with Postgresql*/
$fgt_sql=new fgt_postgres();
while ($fgt_sql->connected!==true)
{
	sleep(1);
	if($var['exitflag']===true)
	return;
}

// create socket
$socket=socket_create_listen($var['port']) or die("Could not Create socket\n");
$message="Started listening port ".$var['port'];
$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);
socket_set_nonblock($socket);

$clients=Array();
while (1)
{
	/*$socket_array[$uuid]=Array(
	[res socket],
	[bool connected],
	[bool identified],
	[str protocal_version],
	[str server_ident],
	[str read_buffer],
	[class read_class],
	[str write_buffer])
	*/
	// accept incoming connections
	// spawn another socket to handle communication
	$newc = socket_accept($socket);
	if($newc !== false and $newc !=null)
    {
        socket_getpeername( $newc ,$address);
		$uuid=uniqid();
		$message="Received connection with $newc from $address. UUID=$uuid";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
		
        $clients[$uuid] = Array("socket"=>$newc,'connected'=>TRUE,'identified'=>FALSE,'read_buffer'=>NULL,'write_buffer'=>NULL);
		socket_set_nonblock($clients[$uuid]['socket']);
    }
	$i=0;
	foreach($clients as $uuid=>$client)
	{
		/*Check the connected flag. If false then close the connection and unset*/
		if($client['connected']===false)
		{
			
			socket_close($client['socket']);
			$message="Stopped connection with".$client['socket']." (UUID=$uuid)";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
			unset($clients[$uuid]);
			continue;
		}
		
		// read client input
		if(socket_recv ( $clients[$uuid]['socket'] , $buft , 2048 , MSG_DONTWAIT )===false)
		{
			if(socket_last_error ($clients[$uuid]['socket'])!=11)
				print socket_strerror(socket_last_error ($clients[$uuid]['socket']))."\n";
		}else
		{
			$clients[$uuid]['read_buffer'].=$buft;
		}
		
		/*Process the read buffer (if needed)*/
		if(strlen ($clients[$uuid]['read_buffer'])>2)
		{	
			$clients[$uuid]['last_connection']=time();
			if($client['identified']===false)
				$fgt_ident->check_ident($uuid);
			else $clients[$uuid]['read_class']->read_buffer();
		}
		
		/*Process the write buffer*/
		if(strlen ($clients[$uuid]['write_buffer'])!=0)
		{
			$clients[$uuid]['last_connection']=time();
			$bytes_written=socket_write($clients[$uuid]['socket'], $clients[$uuid]['write_buffer'], strlen ($clients[$uuid]['write_buffer']));
			if($bytes_written===false)
			{
				$message="Failed sending buffer to ".$client['socket'].": ".socket_strerror(socket_last_error ($clients[$uuid]['socket']));
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
				$clients[$uuid]['connected']=false;
			}else
				$clients[$uuid]['write_buffer'] = substr($clients[$uuid]['write_buffer'], $bytes_written);
		}
		
		/*check timeout*/
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
foreach($clients as $uuid=>$client)
{
	socket_close($client['socket']);
	$message="Stopped connection with ".$client['socket']." (UUID=$uuid)";
	$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
}

socket_close($socket);
$message="Stopped listening socket on port ".$var['port'];
$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);
$fgt_error_report->terminate();
?>

