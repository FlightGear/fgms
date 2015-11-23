<?php
class fgt_read_NOWAIT
{
	var $uuid;
	function fgt_read_NOWAIT($uuid)
	{
		global $fgt_error_report,$var,$clients;
		
		$this->uuid=$uuid;
		$message="Subroutine \"NOWAIT\" for $uuid initialized";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);		
	}
	
	function read_buffer()
	{
		global $fgt_error_report,$var,$clients;
		
		while(1)
		{
			/*check if whole message received*/
			$slash_n_pos = strpos($clients[$this->uuid]['read_buffer'], "\0");
			if($slash_n_pos===false)
				return;
		
			/*obtain one line*/
			$lines=explode("\0", $clients[$this->uuid]['read_buffer'],2);
			$line=$lines[0];
			$clients[$this->uuid]['read_buffer']=$lines[1];
			print "NOWAIT: $line\n";
			$data=explode(" ", $line);
			if($data[0]=="POSITION")
				$msg_array=Array('nature'=>$data[0],'callsign'=>$data[1],'lat'=>$data[3],'lon'=>$data[4],'alt'=>$data[5],'date'=>$data[6],'time'=>$data[7]);
			else if($data[0]=="CONNECT" or $data[0]=="DISCONNECT")
				$msg_array=Array('nature'=>$data[0],'callsign'=>$data[1],'type'=>$data[3],'date'=>$data[4],'time'=>$data[5]);
			$clients[$this->uuid]['write_buffer'].="OK\0";
		}

	}
}
?>