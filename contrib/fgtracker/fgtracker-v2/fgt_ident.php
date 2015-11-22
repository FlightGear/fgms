<?php
class fgt_ident
{
	function fgt_ident()
	{
		global $fgt_error_report,$var;
		$message="Server Ident Manager initalized";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);		
	}
	
	function check_ident($uuid)
	{
		global $fgt_error_report,$var,$clients;
		print $clients[$uuid][5];
		$protocal_version=null;
		/*check if whole message received*/
		$slash_n_pos = strpos($clients[$uuid][5], "\n");
		if($slash_n_pos===false)
			return;
		socket_getpeername( $clients[$uuid][0] ,$address);
		
		/*obtain the ident information*/
		$lines=explode("\n", $clients[$uuid][5],2);
		$line=$lines[0];
		$clients[$uuid][5]=$lines[1];
		
		/*check the version*/
		$ver_test=strpos($line, "NOWAIT");
		if($ver_test!==false and $ver_test==0)
		{
			/*check the identity*/
			
			$protocal_version="NOWAIT";
			$message="Client from $address identified via protocal version $protocal_version";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
			$clients[$uuid][2]=true;
			$clients[$uuid][3]=$protocal_version;
			$clients[$uuid][6]=new fgt_read_NOWAIT($uuid);
			$clients[$uuid][7].="OK\n";
			return;
		}
		$ver_test=strpos($line, "V20151118");
		
		if($ver_test!==false and $ver_test==0)
		{
			$protocal_version="V20151118";
			$message="Client from $address identified via protocal version $protocal_version";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
			$clients[$uuid][2]=true;
			$clients[$uuid][3]=$protocal_version;
			return;
		}
		/*could not be identified*/
		$message="Client from $address could not be identified. Setting close connection flag";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
		$clients[$uuid][1]=false;
	}
}
?>