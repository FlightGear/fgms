<?php
/*variable setup*/

$var['port'] = 5001; /*Port to bind*/
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
declare(ticks = 1); /*required by signal handler*/
require("fgt_error_report.php");
$fgt_error_report=new fgt_error_report();
require("signal.php");
require("fgt_read_NOWAIT.php");
require("fgt_ident.php");
require("fgt_postgres.php");

$fgt_ident=new fgt_ident();

$var['exitflag']=false;
$fgt_postgres=new fgt_postgres();
while ($fgt_postgres->connected!==true)
{
	sleep(1);
	if($var['exitflag']===true)
	return;
}
	

/*check connection with Postgresql*/


// create socket
$socket=socket_create_listen($var['port']) or die("Could not Create socket\n");
$message="Started listening port ".$var['port'];
$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);
socket_set_nonblock($socket);

$clients=Array();
while (1)
{
	/*$socket_array[$uuid]=>[res $socket][bool $connected][bool $identified][str $ver][str $server_ident][str $readbuffer][class $subroutine][str $writebuffer]
								0			1					2				3			4				5					6							7
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
		
        $clients[$uuid] = Array($newc,TRUE,FALSE,NULL,NULL,NULL,NULL,NULL);
		socket_set_nonblock($clients[$uuid][0]);
    }
	$i=0;
	foreach($clients as $uuid=>$client)
	{
		/*Check the connected flag. If false then close the connection and unset*/
		if($client[1]===false)
		{
			
			socket_close($client[0]);
			$message="Stopped connection with $client[0] (UUID=$uuid)";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
			unset($clients[$uuid]);
			continue;
		}
		
		// read client input
		if(socket_recv ( $clients[$uuid][0] , $buft , 2048 , MSG_DONTWAIT )===false)
		{
			if(socket_last_error ($clients[$uuid][0])!=11)
				print socket_strerror(socket_last_error ($clients[$uuid][0]))."\n";
		}else
		{
			$clients[$uuid][5].=$buft;
		}
		
		/*Process the read buffer (if needed)*/
		if(strlen ($clients[$uuid][5])>2)
		{	
			if($client[2]===false)
				$fgt_ident->check_ident($uuid);
			else $clients[$uuid][6]->read_buffer();
		}
		
		/*Process the write buffer*/
		if(strlen ($clients[$uuid][7])!=0)
		{
			$bytes_written=socket_write($clients[$uuid][0], $clients[$uuid][7], strlen ($clients[$uuid][7]));
			if($bytes_written===false)
			{
				$message="Failed sending buffer to $client[0]: ".socket_strerror(socket_last_error ($clients[$uuid][0]));
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
				$clients[$uuid][1]=false;
			}else
				$clients[$uuid][7] = substr($clients[$uuid][7], $bytes_written);
		}
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
	socket_close($client[0]);
	$message="Stopped connection with $client[0] (UUID=$uuid)";
	$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
}

socket_close($socket);
$message="Stopped listening socket on port ".$var['port'];
$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);
$fgt_error_report->terminate();
?>

