#!/usr/bin/perl -w
use strict;
use CGI;
use Socket;

my $request = new CGI;
my @names = $request->param;
my $server = $request->param("server");
if ($server eq "")
{
    $server = "mpserver01.flightgear.org";
}

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
my $fgms_path = "/home/oliver/fgms";
my $fgms_conf = "$fgms_path/fgms.conf";
my $fgms_log  = "$fgms_path/fgms.log";
my @server_names;
my @server_ports;
my @server_locations;

push (@server_names, "mpserver01.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Frankfurt/Germany");

push (@server_names, "mpserver02.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Kansas/USA");

push (@server_names, "mpserver03.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Germany");

push (@server_names, "mpserver04.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "United Kingdom");

push (@server_names, "mpserver05.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Chicago/USA");

push (@server_names, "mpserver06.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Sweden");

push (@server_names, "mpserver07.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Wisconsin, USA");

push (@server_names, "mpserver08.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Germany");

push (@server_names, "mpserver09.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Germany");

push (@server_names, "mpserver10.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Montpellier, France");

push (@server_names, "mpserver11.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Vilnius, Lithuania");

push (@server_names, "mpserver12.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Amsterdam, Netherlands");

push (@server_names, "mpserver13.flightgear.org");
push (@server_ports, "5001");
push (@server_locations, "Grenoble, France");

print "Content-type: text/html\n\n";
my @server_status;
my @server_users;
my @server_locals;
my @server_ip;
my @server_version;
my $status;
my $userlist;
my $users;
my $locals;
my $version;
my $tmp;
my $i;
my $j;
my $current_pilots;
for ($i=0; $i < @server_names; $i++)
{
    my $ip = nslookup ($server_names[$i]);
    $userlist = `nc -w 2 $server_names[$i] $server_ports[$i] 2>&1`;
    if ($server eq $server_names[$i])
    {
        $current_pilots = $userlist;
    }
    $status  = index ($userlist, "Connection");
    if ($status < 0)
    {
        $users   = int(`echo "$userlist" | grep -v '#' | wc -l`) - 1;
        $locals  = `echo "$userlist" | grep -c '\@LOCAL'`;
        $tmp = `echo "$userlist" | grep 'FlightGear Multiplayer Server'`;
        $version = substr($tmp, 32, 8);
    }
    else
    {
        $users  = 0;
        $locals = 0;
        $version = 0;
    }
    push (@server_status, $status);
    push (@server_users, $users);
    push (@server_locals, $locals);
    push (@server_version, $version);
    push (@server_ip, $ip);
}

print "<font face=Arial size=3 color=black>";
print "<b><u>Server Online Status</u></b>";
print "</font><br><p>\n";
print "<table>\n";
for ($i=0; $i < @server_names; $i++)
{
    if ($server_status[$i] < 0)
    {
        $users  = $server_users[$i];
        $locals = $server_locals[$i];
        $version = $server_version[$i];
        print "<tr><td>";
        print "<a href='?server=$server_names[$i]'>";
        print "$server_names[$i]</a> $version ($server_locations[$i])";
        print "</td><td> - </td><td>";
        print "<font color=limegreen>OK</font> ($users total / $locals local clients)";
        print "</td>";
        print "</tr>\n";
    } else {
        $status = $server_status[$i];
        print "<tr><td>";
        print "<a href='?server=$server_names[$i]'>";
        print "$server_names[$i]</a> ($server_locations[$i])";
        print "</td><td> - </td><td><font color=red>DOWN</td></tr></font>\n";
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
# $getpilotsa = `nc $server 5001`;
$getpilotsa = $current_pilots;

chomp($getpilotsa);

@getpilotsa=split(/\n/, $getpilotsa);

print "<font face=Arial size=3 color=black>";
print "<u>User Status</u> on server <b>$server</b>";
print "</font><br><p>\n";

foreach $getpilotsb(@getpilotsa)
{
    # Remove unwanted stuff
    if (($getpilotsb =~ m/# This.*./)
    ||  ($getpilotsb =~ m/# FlightGear.*./)
    ||  ($getpilotsb =~ m/Bad Client/) )
    {
        next;
    }
    # Get the number of pilots on line and push it to the @rray
    if ($getpilotsb =~ m/ online/)
    {
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
    if ($aircraft !~ m/Aircraft\//)
    {
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
            if ($server eq "localhost")
            {
                $mpserver = "mpserver01";
            }
            else
            {
                $mpserver = $server;
            }
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
    if ($getpilotsb)
    {
        push(@getpilotsb, $getpilotsb);
    }
}


$maxpilots = `tail -n 30 $fgms_log | tac | grep -i -m 1 max`;
chomp($maxpilots);
$maxpilots =~ s/.*?max: //g;

foreach $getpilotsb(@getpilotsb)
{
    print "$getpilotsb";
}
print "</table>\n";

print "<table>\n<pre>\n<tr><td></td></tr>\n<tr>";
print "<td><font face=Arial size=2 color=black>";
print "The most pilots seen online together (since server initialisation) is ";
print "<b>$maxpilots</b></font></td></tr>\n</pre>\n</table>\n";

