#!/bin/sh

# this small script checks if fgms is running. If not it restarts fgms and
# sends a message to the admin. Meant to be called by cron
# Written 2008 by Oliver Schroeder

SERVICE='fgms'
EMAIL='fgms@o-schroeder.de'
SOURCE='/home/os/fgms/fgms-0.9.11/src/server/'

export PATH=/usr/local/bin:/usr/bin:/bin:/usr/bin/X11:/usr/games

ps ax | grep -v grep | grep -v $0 | grep $SERVICE  > /dev/null
if [ $? -gt 0 ]; then
    echo "$SERVICE service is not running"
    cd $SOURCE
    ./$SERVICE
#    echo "$SERVICE is not running, restarting!" | mail -s "$SERVICE down" $EMAIL
else
    echo "$SERVICE service running, everything is fine"
fi


