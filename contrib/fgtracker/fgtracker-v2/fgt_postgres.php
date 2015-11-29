<?php
class fgt_postgres{
	/*Postgres manager
	Manage connections to postgresSQL
	*/
	var $conn;
	var $connected;
	
	public function __construct () 
	{
		global $var;
		$this->connected=false;
		$this->conn=NULL;
		$this->connectmaster();
	}
	
	public function connectmaster()
	{
		/*connect to PostgreSQL. This function will never return until a SQL connection has been successfully made
		return true if a new SQL connection has been made.
		return false if SQL connection is untouched
		*/
		global $var,$fgt_error_report,$fgt_conn;
		/*No connection is needed if PostgreSQL is connected*/
		if ($this->connected===true)
			return false;
		
		/* if $fgt_conn is not null, close all connection first*/
		if ($fgt_conn !=null)
		{
			$fgt_conn->close_all_connections();
			$fgt_conn=null;
		}
		if ($this->conn!=NULL)
		{
			pg_close ($this->conn);
			$this->conn=NULL;
		}
		/*connect to Server 1*/
		$message="Connecting to postgres server - ".$var['postgre_conn']['desc'];
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);	

		if ($var['postgre_conn']['host']=="")
		$conn1=pg_connect("dbname=".$var['postgre_conn']['db']." user=".$var['postgre_conn']['uname']." password=".$var['postgre_conn']['pass'] ." connect_timeout=5",PGSQL_CONNECT_FORCE_NEW);
		else
		$conn1=pg_connect("host=".$var['postgre_conn']['host']." port=".$var['postgre_conn']['port']." dbname=".$var['postgre_conn']['db']." user=".$var['postgre_conn']['uname']." password=".$var['postgre_conn']['pass'] ." connect_timeout=5",PGSQL_CONNECT_FORCE_NEW);
		if ($conn1 ===FALSE)
		{
			$message="Failed to connect to postgres server - ".$var['postgre_conn']['desc'].". Will retry in 1 minute";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);	
			$last_failed=time();
			while (1)
			{
				sleep(1);
				if(time()-$last_failed>10)
				{
					$this->connectmaster();
					break;
				}
				if($var['exitflag']===true)
					exit;
			}
			return true;
		}
		
		$message="Connected to postgres server - ".$var['postgre_conn']['desc'];
		$fgt_error_report->fgt_set_error_report("CORE",$message,E_WARNING);	
		
		$res=pg_query($conn1,"SET TIMEZONE TO 'UTC';");
		pg_free_result($res);
		$this->connected=true;
		$this->conn=$conn1;
		$fgt_conn=new fgt_connection_mgr();
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