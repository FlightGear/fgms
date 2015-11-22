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
			$slash_n_pos = strpos($clients[$this->uuid][5], "\n");
			if($slash_n_pos===false)
				return;
		
			/*obtain the one line*/
			$lines=explode("\n", $clients[$this->uuid][5],2);
			$line=$lines[0];
			$clients[$this->uuid][5]=$lines[1];
			print "NOWAIT: $line\n";
			$clients[$this->uuid][7].="OK\n";
		}

	}
}
?>