#!/usr/bin/perl -w
use strict;
use Socket;

# RELEASED UNDER GPLv3
#
# Nick Warne (nick@ukfsn.org) - 2007.
# A simple Flightgear Multi-pilot status report.
# This will query a local server (and can only be run on a FGMP server)
# and report current pilots.
#
# To embed in an http page (need to have server-side includes ON, i.e. *.shtml)
# usage:
# <!--#exec cgi="path/to/this_script_whatever_you_call_it.pl" -->
#
# Make the script name something that nobody can guess.
#

sub trim($)
{
    my $string = shift;
    $string =~ s/^\s+//;
    $string =~ s/\s+$//;
    return $string;
}

sub nslookup ($)
{
    my $host = shift;
    my $packed_ip = gethostbyname("$host");
    if (defined $packed_ip)
    {
                 return (inet_ntoa($packed_ip));
    }
    return (0);
}

###################################
#
# you only need to change values
# here in the first block
#
###################################
my $fgms_path = "/home/os/fgms/fgms-0.9.11/src/server";
my $fgms_conf = "$fgms_path/fgms.conf";
my $fgms_log  = "$fgms_path/server.log";
my @server_names;
my @server_ports;
push (@server_names, "mpserver01.flightgear.org");
push (@server_ports, "5001");

my $relays = `grep relay $fgms_conf  | grep -v \"^#\"`;
my @tokens = split ( /\n/, $relays );
my $cnt = @tokens;
my @s1;
my @s2;
my $i;
my $j;

print "Content-type: text/html\n\n";
if (($cnt%2) != 0)
{
    print "something is wrong with list of relays, there should always be two line per relay!\n";
    print "list of relays is:\n";
    print "$relays\n\n";
    exit (0);
}

for ($i = 0; $i < $cnt; $i+=2)
{
    @s1 = split ( /=/, $tokens[$i] );
    @s2 = split ( /=/, $tokens[$i+1] );

    push (@server_names, trim($s1[1]));
    push (@server_ports, trim($s2[1]) + 1);
}

my @server_status;
my @server_ip;
my $status;
for ($i=0; $i < @server_names; $i++)
{
    my $ip = nslookup ($server_names[$i]);
    $status = `nc -z $server_names[$i] $server_ports[$i] && echo ok`;
    push (@server_status, $status);
    push (@server_ip, $ip);
}

print "<font face=Arial size=3 color=black><b><u>Server Relay Status</u></b></font><br><p>\n";
print "<table>\n";
for ($i=0; $i < @server_names; $i++)
{
    if ($server_status[$i])
    {
        print "<tr><td> $server_names[$i] </td><td> - </td><td><font color=limegreen>OK</td></tr></font>\n";
    } else {
        print "<tr><td> $server_names[$i] </td><td> - </td><td><font color=red>Down</td></tr></font>\n";
    }
}
print "</table>\n";

my $getpilotsa;
my $getpilotsb;
my @getpilotsa;
my @getpilotsb;
my $maxpilots;
my $name;
my $aircraft;
my $mpserver;

print "<hr width=30%><p>\n";


# Here we query FG server - raw data from telnet port 5001
# This could, of course, query any FGMP server, but that is not polite unless you have authorisation.
$getpilotsa = `nc localhost 5001`;

chomp($getpilotsa);

@getpilotsa=split(/\n/, $getpilotsa);

print "<font face=Arial size=3 color=black><b><u>User Status</u></b></font><br><p>\n";

foreach $getpilotsb(@getpilotsa) {
    
    # Remove unwanted stuff
    if (($getpilotsb =~ m/# This.*./) || ($getpilotsb =~ m/# FlightGear.*./) || ($getpilotsb =~ m/Bad Client/) ) {
        next;
    }

    # Get the number of pilots on line and push it to the @rray
    if ($getpilotsb =~ m/ online/) {
        $getpilotsb =~ s/# /\<p\># \<b\>/;
        $getpilotsb =~ s/ pilot/\ \<\/b\>pilot/;
        $getpilotsb =~ s/online/online:\<\/p\>\n\<table>\n/;
        push(@getpilotsb, $getpilotsb);
        next;
    }

    # Set up some stuff for getting the aircraft and pilots call-sign
    $name = reverse $getpilotsb;
    $aircraft = $getpilotsb;

    # This removes wrong version clients trying to join
    $aircraft =~ s/\* unknown \*/Aircraft\/unknown\/Models\/unknown\.xml/;

    # Sometimes (somehow?) an MP client doesn't have an aircraft...
    if ($aircraft !~ m/Aircraft\//) {
        next;
    }

    # Assign server FQDM
    $mpserver = "unknown";
    for ($i=0; $i < @server_ip; $i++)
    {
        if ($getpilotsb =~ m/$server_ip[$i]/)
        {
            $mpserver = $server_names[$i];
        }
    }
    if ($mpserver eq "unknown")
    {
        if ($getpilotsb =~ m/LOCAL/)
        {
            $mpserver = "mpserver01.flightgear.org";

        }
        else
        {  # Just use the IP
            $mpserver = $getpilotsb;
            $mpserver =~ s/.*.@//g;
            $mpserver =~ s/:.*.//;
        }
    }

    # Get the pilots call-sign and aircraft name and clean up a bit
    $name =~ s/.*?\@//;
    $name = reverse $name;
    $aircraft =~ s/.*.\///g;
    $aircraft =~ s/\.xml//;
    $aircraft =~ s/\.ac//;
    $aircraft =~ s/\_model//;
    $aircraft =~ s/\-model//;
    $aircraft =~ s/\-anim//;
    
    # Prepare the line for HTML
    $getpilotsb = "<tr><td><font color = crimson>".$name."</font></td><td>"
    ."&nbsp;&nbsp;<font size = 1>on</font>&nbsp;&nbsp;<font color = navy>".$mpserver
    ."</td><td></font>&nbsp;&nbsp;flying --->&nbsp;&nbsp;</td><td><font color = darkred>".$aircraft
    ."</font></td></tr>\n";

    #  All good, so push it to the @array
    if ($getpilotsb) {
    push(@getpilotsb, $getpilotsb);
    }
}


$maxpilots = `tail -n 30 $fgms_log | tac | grep -i -m 1 max`;
chomp($maxpilots);
$maxpilots =~ s/.*?max: //g;

foreach $getpilotsb(@getpilotsb) {
    print "$getpilotsb";
}
print "</table>\n";

print "<table>\n<pre>\n<tr><td></td></tr>\n<tr>";
print "<td><font face=Arial size=2 color=black>";
print "The most pilots seen online together (since server initialisation) is ";
print "<b>$maxpilots</b></font></td></tr>\n</pre>\n</table>\n";

