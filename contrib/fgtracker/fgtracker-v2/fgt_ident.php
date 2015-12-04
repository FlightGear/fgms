<?php
class fgt_ident
{
	function fgt_ident()
	{
		global $fgt_error_report;
		$message="Server Ident Manager initalized";
		$fgt_error_report->fgt_set_error_report("IDENT",$message,E_NOTICE);		
	}
	
	function check_ident($uuid)
	{
		global $fgt_error_report,$var,$clients,$fgt_sql;
		//print $clients[$uuid]['read_buffer'];

		/*check if whole message received*/
		$slash_n_pos = strpos($clients[$uuid]['read_buffer'], "\0");
		if($slash_n_pos===false)
			return;
		/*read line*/
		$lines=explode("\0", $clients[$uuid]['read_buffer'],2);
		$line=$lines[0];
		$clients[$uuid]['read_buffer']=$lines[1];
		
		/*check if first message (first line from fgms will be ignored)*/
		if($clients[$uuid]['protocal_version']==null)
		{
			$clients[$uuid]['protocal_version']="Unknown";
			return;
		}
		socket_getpeername( $clients[$uuid]['socket'] ,$address);
		
		/*check the version*/
		$ver_test=strpos($line, "NOWAIT");
		if($ver_test!==false and $ver_test==0)
		{
			/*check the identity*/
			$protocal_version="NOWAIT";
			
			$sql="select name from fgt_servers where key='NOWAIT' and ip='$address'";
			$res=pg_query($fgt_sql->conn,$sql);
			if ($res===false or $res==NULL)
			{
				$message="Client from $address could not be identified due to DB problem";
				$fgt_error_report->fgt_set_error_report("IDENT",$message,E_ERROR);
				$clients[$uuid]['connected']=false;
				$fgt_sql->connected=false;
				return;
			}
			$nr=pg_num_rows($res);
			if($nr!=0)
				$clients[$uuid]['server_ident']=$serv_ident=pg_result($res,0,"name");
			else
			{
				$message="Client from $address could not be identified";
				$fgt_error_report->fgt_set_error_report("IDENT",$message,E_WARNING);
				$clients[$uuid]['connected']=false;
				return;
			}
			pg_free_result($res);
			
			$message="Client from $address identified as $serv_ident via protocal version $protocal_version";
			$fgt_error_report->fgt_set_error_report("IDENT",$message,E_NOTICE);
			$clients[$uuid]['identified']=true;
			$clients[$uuid]['protocal_version']=$protocal_version;
			$clients[$uuid]['read_class']=new fgt_read_NOWAIT($uuid);
			$clients[$uuid]['msg_process_class']=new fgt_msg_process($uuid);
			//$clients[$uuid]['write_buffer'].="OK\0";
			return;
		}
		/*$ver_test=strpos($line, "V20151118");
		
		if($ver_test!==false and $ver_test==0)
		{
			$protocal_version="V20151118";
			$message="Client from $address identified via protocal version $protocal_version";
			$fgt_error_report->fgt_set_error_report("IDENT",$message,E_NOTICE);
			$clients[$uuid]['identified']=true;
			$clients[$uuid]['protocal_version']=$protocal_version;
			return;
		}*/
		
		/*could not be identified*/
		$message="Client from $address could not be identified. Setting close connection flag";
		$fgt_error_report->fgt_set_error_report("IDENT",$message,E_NOTICE);
		$clients[$uuid]['socket']=false;
	}
}
?>