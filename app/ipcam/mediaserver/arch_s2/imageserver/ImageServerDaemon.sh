#!/bin/sh
#
# ImageServerDaemon start image control server
#

start()
{
    echo -n "Starting ImageServerDaemon: "
    /sbin/ImageServerDaemon > /dev/null
    if [ $? -eq 1 ]
    then
        echo "Failed!"
	else
        echo "OK!"
    fi
}

stop()
{
    echo -n "Stopping ImageServerDaemon: "
    /sbin/ImageServerDaemon -k > /dev/null
    echo "OK!"
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        stop && start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
esac

exit $?

