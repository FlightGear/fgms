<?php
// signal handler function
function sig_handler($signo)
{
	global $var,$fgt_error_report,$fgt_postgres;
     switch ($signo) {
		case SIGTERM:
			$message="SIGTERM received";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
			exit;
		 case SIGINT:
			// handle shutdown tasks
			$message="SIGINT received. Prepareing to quit";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
			$var['exitflag']=true;
			break;
		case SIGQUIT:
			break;
		case SIGHUP:
			// handle restart tasks
			 //echo "SIGHUP received. Prepareing the input.\n";
			break;
		case SIGCHLD:
			$message="SIGCHLD received";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
			break;
		case SIGUSR1:
			break;
		case SIGALRM:
			if ($fgt_postgres->connected===false)
				$fgt_postgres->connectmaster();
			break;
		default:
			// handle all other signals
			$message="Signo $signo is received but no action will be performed";
			$fgt_error_report->fgt_set_error_report("CORE",$message,E_ERROR);
     }

}


$message="Installing signal handler...";
$fgt_error_report->fgt_set_error_report("CORE",$message,E_NOTICE);

// setup signal handlers
pcntl_signal(SIGCHLD, "sig_handler");
pcntl_signal(SIGALRM, "sig_handler");
pcntl_signal(SIGTERM, "sig_handler");
pcntl_signal(SIGINT, "sig_handler");
pcntl_signal(SIGQUIT, "sig_handler");
?>