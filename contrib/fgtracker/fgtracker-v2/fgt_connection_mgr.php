<?php
class fgt_connection_mgr
{	/*Connection manager
	Manage connections from fgms
	*/
	
	var $socket,$socket_seq;
	
	function __construct ()
	{
		/*This function will never exit until a socket is successfully created*/
		global $fgt_error_report,$var;
		$this->socket_seq=0;
		$message="Connection Manager initalized";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
		// create socket
		$socket=socket_create_listen($var['port']);
		if($socket===false)
		{
			$message="Could not create socket on port ".$var['port'].". Retry in 10 seconds";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);
			$last_failed=time();
			while (1)
			{
				sleep(1);
				if(time()-$last_failed>10)
				{
					$this->__construct();
					break;
				}
				if($var['exitflag']===true)
					exit;
			}
			return;
		}
		$message="Started listening port ".$var['port'];
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);
		socket_set_nonblock($socket);
		$this->socket=$socket;
	}
	
	function accept_connection()
	{
		// accept incoming connections
		// spawn another socket to handle communication
		global $fgt_error_report,$clients;
		$newc = socket_accept($this->socket);
		if($newc !== false and $newc !=null)
		{
			socket_getpeername( $newc ,$address);
			$uuid=$this->socket_seq; $this->socket_seq++;
			$message="Received connection with $newc from $address. UUID=$uuid";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
			
			$clients[$uuid] = Array("socket"=>$newc,'connected'=>TRUE,'identified'=>FALSE,'read_buffer'=>NULL,'write_buffer'=>NULL,'protocal_version'=>NULL,'server_ident'=>NULL,'last_reception'=>time(),'timeout_stage'=>0);
			socket_set_nonblock($clients[$uuid]['socket']);
		}
	}
	
	function check_timeout($uuid)
	{
		global $fgt_error_report,$clients,$var;
		
		if(time()-$clients[$uuid]['last_reception']-$var['ping_interval']*$clients[$uuid]['timeout_stage']>$var['ping_interval'])
		{
			$clients[$uuid]['timeout_stage']++;
			$timeout=($clients[$uuid]['timeout_stage'])*$var['ping_interval'];
			if($clients[$uuid]['timeout_stage']>3)
			{
				$clients[$uuid]['connected']=false;
				$message="Client ".$clients[$uuid]['server_ident']." timeout ($timeout seconds)";
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);
				return;
			}
			$message="PING Client ".$clients[$uuid]['server_ident']." (No input for $timeout seconds)";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
			if($clients[$uuid]['identified']===true) /*Only write ping when identified*/
				$clients[$uuid]['write_buffer'].="PING\0";
		}
	}
	
	function close_connection($uuid)
	{
		/*Check the connected flag. If false then close the connection and unset
		Return true if a connection is closed
		*/
		global $fgt_error_report,$clients;
		if($clients[$uuid]['connected']===false)
		{
			$clients[$uuid]['write_buffer'].="Error : FGTracker is closing your connection\0";
			$this->write_connection($uuid);
			socket_close($clients[$uuid]['socket']);
			$message="Stopped connection with ".$clients[$uuid]['server_ident']." (UUID=$uuid)";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
			unset($clients[$uuid]);
			return true;
		}
		return false;
	}
	
	function close_all_connections()
	{
		/*Close all connections*/
		global $fgt_error_report,$clients,$var;
		foreach($clients as $uuid=>$client)
		{
			$clients[$uuid]['connected']=false;
			$clients[$uuid]['write_buffer'].="Error : Fgtracker is closing your connection\0";
			$this->write_connection($uuid);
			socket_close($clients[$uuid]['socket']);
			$message="Stopped connection with ".$clients[$uuid]['server_ident']." (UUID=$uuid)";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
			unset($clients[$uuid]);
		}
		socket_close($this->socket);
		$message="Stopped listening socket on port ".$var['port'];
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);
	}
	
	function read_connection($uuid)
	{
		/*read connection to buffer
		Return true if success, false if failed
		*/
		global $fgt_error_report,$clients;
		if(socket_recv ( $clients[$uuid]['socket'] , $buft , 2048 , MSG_DONTWAIT )===false)
		{
			if(socket_last_error ($clients[$uuid]['socket'])!=11)
			{
				$message="Socket Error - ".socket_strerror(socket_last_error ($clients[$uuid]['socket']));
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
				$clients[$uuid]['connected']=false;
				return false;
			}
			return true;
		}else
		{
			$clients[$uuid]['read_buffer'].=$buft;
			return true;
		}
	}
	
	function write_connection($uuid)
	{
		/*write buffer to connection
		*/
		global $fgt_error_report,$clients;
		$i=0;
		while(strlen ($clients[$uuid]['write_buffer'])!=0)
		{
			if($i>3)
			{
				$message="Buffer write stuck in ".$clients[$uuid]['server_ident'] ." too long, moving out";
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
				break;
			}
			$i++;	
			/*check if stuck in write buffer too long. If so break*/
			$bytes_written=socket_write($clients[$uuid]['socket'], $clients[$uuid]['write_buffer'], strlen ($clients[$uuid]['write_buffer']));
			$message="Wrote $bytes_written bytes to ".$clients[$uuid]['server_ident'];
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_ALL);
			if($bytes_written===false)
			{
				$message="Failed sending buffer to ".$clients[$uuid]['server_ident'].": ".socket_strerror(socket_last_error ($clients[$uuid]['socket']));
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
				$clients[$uuid]['connected']=false;
				return;
			}else
				$clients[$uuid]['write_buffer'] = substr($clients[$uuid]['write_buffer'], $bytes_written);
		}
	}
}
?>