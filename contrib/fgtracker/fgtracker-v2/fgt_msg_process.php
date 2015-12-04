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
			$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ERROR);
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
		$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_NOTICE);			
	}
	
	function computeDistance($latA,$lonA,$latB,$lonB)
	{	/* Compute the distance between two waypoints (meters)*/
		$ERAD=6378138.12; 
		$D2R = M_PI / 180;
		$R2D = 180 / M_PI;

		$latA*=$D2R;
		$lonA*=$D2R;
		$latB*=$D2R;
		$lonB*=$D2R;
		
		$distance=$ERAD*2*asin(sqrt(pow(sin(($latA-$latB)/2),2) + cos($latA)*cos($latB)*pow(sin(($lonA-$lonB)/2),2)));

		return $distance;
	}
	
	function fgt_pg_query_params($sql,$sql_parm)
	{
		global $fgt_error_report,$clients,$fgt_sql;
		$res=pg_query_params($fgt_sql->conn,$sql,$sql_parm);
		if ($res===false or $res==NULL)
		{
			$phpErr=error_get_last();
			$message="Internal DB Error - ".pg_last_error ($fgt_sql->conn);
			$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ERROR);
			$message="SQL command of last error: ".$sql;
			$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ERROR);
			$message="PHP feedback of last error: ".$phpErr['message'];
			$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ERROR);
			$stat = pg_connection_status($fgt_sql->conn);
			if ($stat !== PGSQL_CONNECTION_OK) 
				$fgt_sql->connected=false;
			$clients[$this->uuid]['connected']=false;
			return false;
		}return $res;
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
					$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_WARNING);	
					break;
				}
				if(intval($msg_array['alt'])<-9000)
				{
					$message="$err_prefix Invalid altitude (".$msg_array['alt'].")";
					$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_NOTICE);
					break;
				}
				if(intval($msg_array['alt'])==0 and intval($msg_array['lat'])==0 and intval($msg_array['lon'])==0)
				{
					$message="$err_prefix Invalid position (".$msg_array['lat'].", ".$msg_array['lon'].", ".$msg_array['alt'].")";
					$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_NOTICE);
					break;
				}
				$timestamp=$msg_array['date']." ".$msg_array['time']." Z";
				/*TODO: Use UPSERT once PostgreSQL 9.5 is available*/
				$sql_parm=Array($this->open_flight_array[$msg_array['callsign']]['id'],$timestamp,$msg_array['lat'],$msg_array['lon'],$msg_array['alt']);
				$sql="INSERT INTO waypoints(flight_id,time,latitude,longitude,altitude)VALUES ($1,$2,$3,$4,$5);";
				
				$res=$this->fgt_pg_query_params($sql,$sql_parm);
				if ($res===false or $res==NULL)
					return;
				$this->open_flight_array[$msg_array['callsign']]['waypoints']++;
				
				/* Splited flight detection
				A newly started flight will be checked when the second waypoints received.
				In case the following situation occurs, flight merging will automatically be done:
				1. Same model as previous flight
				2. Start_time and previous end_time 0<=time<=120 seconds 
				3. First waypoints and last waypoints distance <408m/s
				4. First two waypoints of current flight have the calculated speed >60km/h
				5. Last two waypoints of previous flight have the calculated speed >60km/h
				*/
				if($this->open_flight_array[$msg_array['callsign']]['waypoints']!=2)
					break;
				$merge_speed_threshold=60; /*in KM/h*/
				$message="Splited flight detection on callsign ".$msg_array['callsign'];
				$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ALL);
				$sql_parm=Array($msg_array['callsign'],$this->open_flight_array[$msg_array['callsign']]['id']);
				$sql="select * from flights where callsign=$1 and (select extract(epoch from start_time) from flights where id=$2)-extract(epoch from end_time) <120 and (select extract(epoch from start_time) from flights where id=$2)- extract(epoch from end_time) > 0 and model=(select model from flights where id=$2) order by end_time desc limit 1";
				$res=$this->fgt_pg_query_params($sql,$sql_parm);
				if ($res===false or $res==NULL)
					return;
				$nr=pg_num_rows($res);
				if($nr!=1)
					break;

				$p_flightid=pg_result($res,0,"id");
				$message="First phase pass on callsign ".$msg_array['callsign']." (Previous flight id: $p_flightid). Conduct second phase check";
				$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ALL);
				$sql_parm=Array($this->open_flight_array[$msg_array['callsign']]['id'],$p_flightid);
				$sql='(select flight_id, extract(epoch from "time") AS time, latitude, longitude from waypoints where flight_id=$1 order by time desc) UNION all (select flight_id, extract(epoch from "time") AS time, latitude, longitude from waypoints where flight_id=$2 order by time desc limit 2)';						
				$res=$this->fgt_pg_query_params($sql,$sql_parm);
				if ($res===false or $res==NULL)
					return;
				$nr=pg_num_rows($res);
				if ($nr!=4)
				{
					$message="Splited flight second phase check failed on callsign ".$msg_array['callsign']." - No. of wpts not equal to 4 (got $nr)";
					$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_WARNING);
					break;
				}
				for($i=0;$i<$nr;$i++)/*waypoints in time descending order*/
					$waypointsArr[]=Array('time'=>pg_result($res,$i,"time"),'lat'=>pg_result($res,$i,"latitude"),'lon'=>pg_result($res,$i,"longitude"));
				pg_free_result($res);
				
				/*check time validity*/
				if($waypointsArr[0]['time']==$waypointsArr[1]['time'] or $waypointsArr[2]['time']==$waypointsArr[3]['time'])
				{
					$message="Splited flight second phase check failed on callsign ".$msg_array['callsign']." - Invalid time detected";
					$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_WARNING);
					break;					
				}
				/*check if wpt 0&1 over 60km/h*/
				$distanceM=$this->computeDistance($waypointsArr[0]['lat'],$waypointsArr[0]['lon'],$waypointsArr[1]['lat'],$waypointsArr[1]['lon']);
				$speedKmh=abs($distanceM/($waypointsArr[0]['time']-$waypointsArr[1]['time'])*3600/1000); 
				if($speedKmh<$merge_speed_threshold)
				{
					$message="Splited flight second phase check failed on callsign ".$msg_array['callsign']." - Current flight is slower than threshold (got $speedKmh)";
					$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ALL);
					break;
				}
				/*check if wpt 2&3 over 60km/h*/
				$distanceM=$this->computeDistance($waypointsArr[2]['lat'],$waypointsArr[2]['lon'],$waypointsArr[3]['lat'],$waypointsArr[3]['lon']);
				$speedKmh=abs($distanceM/($waypointsArr[2]['time']-$waypointsArr[3]['time'])*3600/1000); 
				if($speedKmh<$merge_speed_threshold)
				{
					$message="Splited flight second phase check failed on callsign ".$msg_array['callsign']." - Previous flight is slower than threshold (got $speedKmh)";
					$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ALL);
					break;
				}
				/*check if First waypoints and last waypoints distance > 408m/s*/
				$distanceM=$this->computeDistance($waypointsArr[1]['lat'],$waypointsArr[1]['lon'],$waypointsArr[2]['lat'],$waypointsArr[2]['lon']);
				$distanceThreshold=408*($waypointsArr[1]['time']-$waypointsArr[2]['time']);
				if($distanceM>$distanceThreshold)
				{
					$message="Splited flight second phase check failed on callsign ".$msg_array['callsign']." - Distance between departure and arriaval greater than threshold (Should smaller than $distanceThreshold but got $distanceM)";
					$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ALL);
					break;
				}
				$message="Splited flight second phase check pass on callsign ".$msg_array['callsign']." - Perform merging";
				$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_NOTICE);
			break;
			case "CONNECT":
				$err_prefix="Could not CONNECT for callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident'].".";
				if(isset($this->open_flight_array[$msg_array['callsign']]))
				{
					/*Close previous flight*/
					$sql="select time from waypoints where flight_id=".$this->open_flight_array[$msg_array['callsign']]['id']." ORDER BY time DESC LIMIT 1";
					$res=$this->fgt_pg_query_params($sql,Array());
					if ($res===false or $res==NULL)
						return;
					
					$close_time=pg_result($res,0,"time");
					pg_free_result($res);
					
					$sql_parm=Array($close_time,$this->open_flight_array[$msg_array['callsign']]['id']);
					$sql="UPDATE flights SET status='CLOSED',end_time=$1 WHERE id=$2 AND status='OPEN';";
					$res=$this->fgt_pg_query_params($sql,$sql_parm);
					if ($res===false or $res==NULL)
						return;
				}

				/*Insert flight*/
				$timestamp=$msg_array['date']." ".$msg_array['time']." Z";
				$sql_parm=Array($msg_array['callsign'],$msg_array['model'],$timestamp,$clients[$this->uuid]['server_ident']);
				$sql="INSERT INTO flights (callsign,status,model,start_time,server) VALUES ($1,'OPEN',$2,$3,$4);";
				$res=$this->fgt_pg_query_params($sql,$sql_parm);
				if ($res===false or $res==NULL)
					return;
				
				
				$res=pg_query($fgt_sql->conn,"SELECT currval('flights_id_seq') AS lastinsertid;");
				$this->open_flight_array[$msg_array['callsign']]['id']=pg_result($res,0,"lastinsertid");
				$this->open_flight_array[$msg_array['callsign']]['waypoints']=0;
				pg_free_result($res);
				
				$message="Welcome callsign \"".$msg_array['callsign']."\" with flight id ".$this->open_flight_array[$msg_array['callsign']]['id'];
				$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_ERROR);
			break;
			case "DISCONNECT":
				$err_prefix="Could not DISCONNECT for callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident'].".";
				$timestamp=$msg_array['date']." ".$msg_array['time']." Z";
				$sql_parm=Array($timestamp,$this->open_flight_array[$msg_array['callsign']]['id']);
				$sql="UPDATE flights SET status='CLOSED',end_time=$1 WHERE id=$2 AND status='OPEN';";
				$res=$this->fgt_pg_query_params($sql,$sql_parm);
					if ($res===false or $res==NULL)
						return;
				
				$message="Callsign \"".$msg_array['callsign']."\" from ".$clients[$this->uuid]['server_ident']." with flight id ".$this->open_flight_array[$msg_array['callsign']]['id']." left";
				$fgt_error_report->fgt_set_error_report($clients[$this->uuid]['server_ident'],$message,E_NOTICE);
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