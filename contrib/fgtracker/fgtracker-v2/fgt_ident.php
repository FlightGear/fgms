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
		$message=" RECV: $line";
		$fgt_error_report->fgt_set_error_report("IDENT",$message,E_ALL);
		$data=explode(" ",$line);
		if ($data[0]=="NOWAIT") 
			$protocal_version="NOWAIT";
		else if ($data[0]=="V20151207")
			$protocal_version="V20151207";
			
		$identfailmsg="Client from $address could not be identified. ";
		$dbfailmessage="Client from $address could not be identified due to DB problem";
		if($protocal_version=="Unknown")
		{
			/*could not be identified*/
			$fgt_error_report->fgt_set_error_report("IDENT",$identfailmsg."Unknown/Unsupported protocal version",E_NOTICE);
			$clients[$uuid]['socket']=false;
			return;
		}
		/*check the identity*/
		$sql="select name from fgms_servers where key='$protocal_version' and ip='$address' and enabled = 'Y'";		
		$res=pg_query($fgt_sql->conn,$sql);
		if ($res===false or $res==NULL)
		{
			
			$fgt_error_report->fgt_set_error_report("IDENT",$dbfailmessage,E_ERROR);
			$clients[$uuid]['connected']=false;
			$fgt_sql->connected=false;
			return;
		}
		$nr=pg_num_rows($res);
		if($nr!=0)
		{
			$clients[$uuid]['server_ident']=$serv_ident=pg_result($res,0,"name");
			pg_free_result($res);
		}	
		else 
		{	
			pg_free_result($res);
			if($protocal_version=="NOWAIT")
			{
				$fgt_error_report->fgt_set_error_report("IDENT",$identfailmsg.$protocal_version."/".$address,E_WARNING);
				$clients[$uuid]['connected']=false;
				return;
			}
			/*V20151207 have one more chance, that is compare the list with self declared domain name. and compare the domain name with his address*/
			if(!filter_var($data[2], FILTER_VALIDATE_IP))
			{	/*FGTracker only accepts client self declare his domain name*/
				$clientdn=pg_escape_string ($fgt_sql->conn, $data[2]);
				$sql="select name from fms_servers where key='$protocal_version' and ip='$clientdn' and enabled = 'Y' ORDER BY ip";
				$res=pg_query($fgt_sql->conn,$sql);
				if ($res===false or $res==NULL)
				{
					$fgt_error_report->fgt_set_error_report("IDENT",$dbfailmessage,E_ERROR);
					$clients[$uuid]['connected']=false;
					$fgt_sql->connected=false;
					return;
				}
				$nr=pg_num_rows($res);
				if($nr!=0)
				{
					if ($address!=gethostbyname($data[2]."."))
					{
						$fgt_error_report->fgt_set_error_report("IDENT",$identfailmsg."Domain name and ip not match",E_WARNING);
						$clients[$uuid]['connected']=false;
						return;
					}
					$clients[$uuid]['server_ident']=$serv_ident=pg_result($res,0,"name");
					pg_free_result($res);
				}else			
				{
					$fgt_error_report->fgt_set_error_report("IDENT",$identfailmsg.$protocal_version."/".$clientdn,E_WARNING);
					$clients[$uuid]['connected']=false;
					return;
				}
			}else			
			{
				$fgt_error_report->fgt_set_error_report("IDENT",$identfailmsg."Invalid domain name supplied",E_WARNING);
				$clients[$uuid]['connected']=false;
				return;
			}
		}
		
		$message="Client from $address identified as $serv_ident via protocal version $protocal_version";
		$fgt_error_report->fgt_set_error_report("IDENT",$message,E_NOTICE);
		$clients[$uuid]['identified']=true;
		$clients[$uuid]['protocal_version']=$protocal_version;
		if ($protocal_version=="NOWAIT")
			$clients[$uuid]['read_class']=new fgt_read_NOWAIT($uuid);
		else $clients[$uuid]['read_class']=new fgt_read_V20151207($uuid);
		$clients[$uuid]['msg_process_class']=new fgt_msg_process($uuid);
		$clients[$uuid]['write_buffer'].="IDENTIFIED $serv_ident\0";
		return;
	}
}
?>