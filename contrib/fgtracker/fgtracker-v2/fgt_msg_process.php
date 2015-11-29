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
		$sql="select id,callsign from flights where status='OPEN' AND server='".$clients[$this->uuid]['server_ident'] ."'";
		$res=pg_query($fgt_sql->conn,$sql);
		if ($res===false or $res==NULL)
		{
			$message="Message processor for ".$clients[$this->uuid]['server_ident'] ." could not be initialized due to DB problem - ".pg_last_error ($fgt_sql->conn);
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
			$clients[$uuid]['connected']=false;
			return;
		}
		$nr=pg_num_rows($res);
		for ($i=0;$i<$nr;$i++)
			$this->open_flight_array[pg_result($res,$i,"callsign")]=pg_result($res,$i,"id");

		pg_free_result($res);
		$message="Message processor for ".$clients[$this->uuid]['server_ident'] ." initalized. $nr flight(s) remain open";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);			
	}
	
	function msg_process($msg_array,$uuid)
	{
		global $fgt_error_report,$clients,$var,$fgt_sql;
		switch ($msg_array['nature'])
		{
			case "POSITION":
				$err_prefix="Could not insert POSITION for callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident'].".";
				if(!isset($this->open_flight_array[$msg_array['callsign']]))
				{
					$message="$err_prefix No open flight available";
					$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);	
				}else
				{
					$timestamp=$msg_array['date']." ".$msg_array['time']." Z";
					/*TODO: Use UPSERT once PostgreSQL 9.5 is available*/
					$sql_parm=Array($this->open_flight_array[$msg_array['callsign']],$timestamp,$msg_array['lat'],$msg_array['lon'],$msg_array['alt']);
					$sql="INSERT INTO waypoints(flight_id,time,latitude,longitude,altitude)VALUES ($1,$2,$3,$4,$5);";
					
					$res=pg_query_params($fgt_sql->conn,$sql,$sql_parm);
					if ($res===false or $res==NULL)
					{
						$message="$err_prefix Internal DB Error - ".pg_last_error ($fgt_sql->conn);
						$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
						$clients[$uuid]['connected']=false;
						$fgt_sql->connected=false;
						return;
					}
				}
			break;
			case "CONNECT":
				$err_prefix="Could not CONNECT for callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident'].".";
				if(isset($this->open_flight_array[$msg_array['callsign']]))
				{
					/*Close previous flight*/
					$sql="select time from waypoints where flight_id=".$this->open_flight_array[$msg_array['callsign']]." ORDER BY time DESC LIMIT 1";
					$res=pg_query($fgt_sql->conn,$sql);
					if ($res===false or $res==NULL)
					{
						$message="$err_prefix Internal DB Error - ".pg_last_error ($fgt_sql->conn);
						$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
						$clients[$uuid]['connected']=false;
						$fgt_sql->connected=false;
						return;
					}
					$close_time=pg_result($res,0,"time");
					pg_free_result($res);
					
					$sql_parm=Array($close_time,$this->open_flight_array[$msg_array['callsign']]);
					$sql="UPDATE flights SET status='CLOSED',end_time=$1 WHERE id=$2 AND status='OPEN';";
					$res=pg_query_params($fgt_sql->conn,$sql,$sql_parm);
					if ($res===false or $res==NULL)
					{
						$message="$err_prefix Internal DB Error - ".pg_last_error ($fgt_sql->conn);
						$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
						$clients[$uuid]['connected']=false;
						$fgt_sql->connected=false;
						return;
					}
				}
				/*Insert flight*/
				$timestamp=$msg_array['date']." ".$msg_array['time']." Z";
				$sql_parm=Array($msg_array['callsign'],$msg_array['model'],$timestamp,$clients[$this->uuid]['server_ident']);
				$sql="INSERT INTO flights (callsign,status,model,start_time,server) VALUES ($1,'OPEN',$2,$3,$4);";
				$res=pg_query_params($fgt_sql->conn,$sql,$sql_parm);
				if ($res===false or $res==NULL)
				{
					$message="$err_prefix Internal DB Error - ".pg_last_error ($fgt_sql->conn);
					$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
					$clients[$uuid]['connected']=false;
					$fgt_sql->connected=false;
					return;
				}
				
				
				$res=pg_query($fgt_sql->conn,"SELECT currval('flights_id_seq') AS lastinsertid;");
				$this->open_flight_array[$msg_array['callsign']]=pg_result($res,0,"lastinsertid");
				pg_free_result($res);
				
				$message="Welcome callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident']." with flight id ".$this->open_flight_array[$msg_array['callsign']];
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
			break;
			case "DISCONNECT":
				$err_prefix="Could not DISCONNECT for callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident'].".";
				$timestamp=$msg_array['date']." ".$msg_array['time']." Z";
				$sql_parm=Array($timestamp,$this->open_flight_array[$msg_array['callsign']]);
				$sql="UPDATE flights SET status='CLOSED',end_time=$1 WHERE id=$2 AND status='OPEN';";
				$res=pg_query_params($fgt_sql->conn,$sql,$sql_parm);
				if ($res===false or $res==NULL)
				{
					$message="$err_prefix Internal DB Error - ".pg_last_error ($fgt_sql->conn);
					$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
					$clients[$uuid]['connected']=false;
					$fgt_sql->connected=false;
					return;
				}
				
				$message="Callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident']." with flight id ".$this->open_flight_array[$msg_array['callsign']]." left";
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);
				unset($this->open_flight_array[$msg_array['callsign']]);
			break;
			default:
			/*fgt_read_XX should already handled the unrecognized messages*/
			return;
		}
		$clients[$uuid]['write_buffer'].="OK\0";
		
	}
}
?>