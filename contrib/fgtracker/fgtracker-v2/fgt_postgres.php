<?php
class fgt_postgres{
	
	var $conn;
	var $connected;
	
	public function __construct () 
	{
		global $var;
		$this->connected=false;
		$this->connectmaster();
	}
	
	public function connectmaster()
	{
		global $var,$fgt_error_report;
		/*connect to Server 1*/
		$message="Connecting to postgres server - ".$var['postgre_conn']['desc']."...";
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);	

		if ($var['postgre_conn']['host']=="")
		$conn1=pg_connect("dbname=".$var['postgre_conn']['db']." user=".$var['postgre_conn']['uname']." password=".$var['postgre_conn']['pass'] ." connect_timeout=10");
		else
		$conn1=pg_connect("host=".$var['postgre_conn']['host']." port=".$var['postgre_conn']['port']." dbname=".$var['postgre_conn']['db']." user=".$var['postgre_conn']['uname']." password=".$var['postgre_conn']['pass'] ." connect_timeout=10");
		if ($conn1 ===FALSE)
		{
			$message="Failed to connect to postgres server - ".$var['postgre_conn']['desc'].". Will retry in 1 minute";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);	
			pcntl_alarm(60);
			return false;
		}
		
		$message="Connected to postgres server - ".$var['postgre_conn']['desc'];
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);	
		
		$res=pg_query($conn1,"SET TIMEZONE TO 'UTC';");
		if ($res===FALSE)
		return false;
		
		pg_free_result($res);
		$this->connected=true;
		$this->conn=$conn1;
		return true;
	}
	/*Not necessary to be called as connection is non-presistent
	function __destruct()
	{	
		if($this->connected===false)
			return;
		
		if(pg_close ($this->conn)===true)
		{	
			$message="Postgres server - ".$var['postgre_conn']['desc']." closed";
			$this->fgt_set_error_report("CORE",$message,E_WARNING);
			$this->connected=false;
			return true;
		}else
		{
			$message="Failed to close postgres server - ".$var['postgre_conn']['desc'];
			$this->fgt_set_error_report("CORE",$message,E_ERROR);
			return false;
		}
			
	}*/
}

?>