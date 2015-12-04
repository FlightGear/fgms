<?php
class fgt_read_NOWAIT
{
	var $uuid;
	function fgt_read_NOWAIT($uuid)
	{
		global $fgt_error_report,$var,$clients;
		
		$this->uuid=$uuid;
		$message="Subroutine \"NOWAIT\" for ".$clients[$this->uuid]['server_ident'] ." initialized";
		$fgt_error_report->fgt_set_error_report("R_NOWAIT",$message,E_NOTICE);		
	}
	
	function read_buffer()
	{
		global $fgt_error_report,$var,$clients,$fgt_sql;
		$i=0;
		while(1)
		{
			/*check if stuck in the same server too long. if so return and receive another server's stream*/
			if($i==100)
			{
				$message="Buffer read stuck in ".$clients[$this->uuid]['server_ident'] ." too long, moving to another server";
				$fgt_error_report->fgt_set_error_report("R_NOWAIT",$message,E_NOTICE);
				break;
			}
			
			/*check if whole message received*/
			$slash_n_pos = strpos($clients[$this->uuid]['read_buffer'], "\0");
			if($slash_n_pos===false)
				break;
		
			/*obtain one line*/
			$lines=explode("\0", $clients[$this->uuid]['read_buffer'],2);
			$line=$lines[0];
			$clients[$this->uuid]['read_buffer']=$lines[1];
			print "NOWAIT: $line\n";
			/*Sometimes the fgms sends invalid packets. Below is a workaround to prevent unnessary forced exit. 
			Example of Invalid messgae are:
			POSITION * Bad Client *  0 0 . 2012-12-03 21:03:53
			DISCONNECT * Bad Client *  * unknown * 2012-12-03 20:56:32
			POSITION franck test  . . . 2012-12-08 14:28:42
			*/
			if(stripos ( $line , "* Bad Client *"  )!==false or stripos ( $line , ". . ."  )!==false)
			{
				$message="Unrecognized Message from ".$clients[$this->uuid]['server_ident'] ."($line). Message ignored";
				$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_WARNING);
				$clients[$this->uuid]['write_buffer'].="OK\0";
				continue;
			}
			
			$data=explode(" ", $line);
			if($data[0]=="PING")
			{
				$clients[$this->uuid]['write_buffer'].="PONG\0";
			}else if($data[0]=="POSITION")
			{
				$msg_array=Array('nature'=>$data[0],'callsign'=>$data[1],'lat'=>$data[3],'lon'=>$data[4],'alt'=>$data[5],'date'=>$data[6],'time'=>$data[7]);
				$clients[$this->uuid]['msg_process_class']->msg_process($msg_array,$this->uuid);
			}else if($data[0]=="CONNECT" or $data[0]=="DISCONNECT")
			{
				$msg_array=Array('nature'=>$data[0],'callsign'=>$data[1],'model'=>$data[3],'date'=>$data[4],'time'=>$data[5]);
				$clients[$this->uuid]['msg_process_class']->msg_process($msg_array,$this->uuid);
			}	else
			{
				$message="Unrecognized Message from ".$clients[$this->uuid]['server_ident'] ."($line). Setting close connection flag";
				$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ERROR);
				$clients[$this->uuid]['write_buffer'].="Failed : Message not recognized\0";
				$clients[$this->uuid]['connected']=false;
			}
			if($clients[$this->uuid]['connected']===false or $fgt_sql->connected===false)
				return false;
			$i++;
		}
	}
}
?>