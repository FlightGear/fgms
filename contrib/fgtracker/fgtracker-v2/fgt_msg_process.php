<?php
class fgt_msg_process
{
	var $uuid;
	var $open_flight_array;
	
	function __construct ($uuid)
	{
		global $fgt_error_report,$clients,$var,$fgt_sql;
		$this->uuid=$uuid;
		
		/*Get opening flights*/
		$sql="select id,callsign from flights where server='".$clients[$this->uuid]['server_ident'] ."'";
		$res=pg_query($fgt_sql->conn,$sql);
		if ($res===false or $res==NULL)
		{
			$message="Message processor for ".$clients[$this->uuid]['server_ident'] ." could not be initialized due to DB problem";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
			$clients[$uuid]['connected']=false;
			return;
		}
		$nr=pg_num_rows($res);
		for ($i=0;$i<$nr;$i++)
		{
			//$open_flight_array['']=
		}

			pg_free_result($res);
		$message="Message processor for ".$clients[$this->uuid]['server_ident'] ." initalized. $nr flight(s) remain open";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);			
	}
	
	function msg_process($msg_array,$uuid)
	{
		global $fgt_error_report,$clients,$var,$fgt_sql;
		$clients[$uuid]['write_buffer'].="OK\0";
		
	}
}
?>