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
		$sql="select flights.id,callsign, count(*) as cnt from flights join waypoints on waypoints.flight_id=flights.id where status='OPEN' AND server='".$clients[$this->uuid]['server_ident'] ."' group by flights.id, callsign";
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
		{
			$this->open_flight_array[pg_result($res,$i,"callsign")]['id']=pg_result($res,$i,"id");			
			$this->open_flight_array[pg_result($res,$i,"callsign")]['waypoints']=pg_result($res,$i,"cnt");			
		}


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
				}else if(intval($msg_array['alt'])<-9000)
				{
					$message="$err_prefix Invalid altitude (".$msg_array['alt'].")";
					$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);	
				}else if(intval($msg_array['alt'])==0 and intval($msg_array['lat'])==0 and intval($msg_array['lon'])==0)
				{
					$message="$err_prefix Invalid positin (".$msg_array['lat'].", ".$msg_array['lon'].", ".$msg_array['alt'].")";
					$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);	
				}else
				{
					$timestamp=$msg_array['date']." ".$msg_array['time']." Z";
					/*TODO: Use UPSERT once PostgreSQL 9.5 is available*/
					$sql_parm=Array($this->open_flight_array[$msg_array['callsign']]['id'],$timestamp,$msg_array['lat'],$msg_array['lon'],$msg_array['alt']);
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
					$this->open_flight_array[$msg_array['callsign']]['waypoints']++;
					/* Splited flight detection
					A newly started flight will be checked when the second waypoints received.
					In case the following situation occurs, flight merging will automatically be done:
					1. Same model as previous flight
					2. Start_time and previous end_time 0<=time<=120 seconds 
					3. First waypoints and last waypoints distance <49000/time difference
					4. First two waypoints have the calculated speed <80km/h
					*/
					if($this->open_flight_array[$msg_array['callsign']]['waypoints']==2)
					{
						$message="Splited flight detection on callsign ".$msg_array['callsign'];
						$fgt_error_report->fgt_set_error_report("CORE",$message,E_ALL);
						$sql_parm=Array($msg_array['callsign'],$this->open_flight_array[$msg_array['callsign']]['id']);
						$sql="select * from flights where callsign=$1 and (select start_time from flights where id=$2)-end_time <120 and (select start_time from flights where id=$2)-end_time > 0 and model=(select model from flights where id=$2) order by end_time desc limit 1";						
					}
				}
			break;
			case "CONNECT":
				$err_prefix="Could not CONNECT for callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident'].".";
				if(isset($this->open_flight_array[$msg_array['callsign']]))
				{
					/*Close previous flight*/
					$sql="select time from waypoints where flight_id=".$this->open_flight_array[$msg_array['callsign']]['id']." ORDER BY time DESC LIMIT 1";
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
					
					$sql_parm=Array($close_time,$this->open_flight_array[$msg_array['callsign']]['id']);
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
				$this->open_flight_array[$msg_array['callsign']]['id']=pg_result($res,0,"lastinsertid");
				$this->open_flight_array[$msg_array['callsign']]['waypoints']=0;
				pg_free_result($res);
				
				$message="Welcome callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident']." with flight id ".$this->open_flight_array[$msg_array['callsign']]['id'];
				$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
			break;
			case "DISCONNECT":
				$err_prefix="Could not DISCONNECT for callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident'].".";
				$timestamp=$msg_array['date']." ".$msg_array['time']." Z";
				$sql_parm=Array($timestamp,$this->open_flight_array[$msg_array['callsign']]['id']);
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
				
				$message="Callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident']." with flight id ".$this->open_flight_array[$msg_array['callsign']]['id']." left";
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