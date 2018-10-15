CMD=$1
BASEDIR=$(dirname "$0")
PLIST=$BASEDIR/eu.onderweg.follow.plist

case "$CMD" in
	"install")
		cp -v $PLIST ~/Library/LaunchAgents
		;;
	"start")
        launchctl load $PLIST
        launchctl start eu.onderweg.follow
        launchctl list | grep 'eu.onderweg.follow'
		;;
	"stop")
	    launchctl unload $PLIST
		;;
	"logs")
	    tail /tmp/follow_the_sun.log
		;;		
	*)
		echo "Usage: $0 [install|start|stop|logs]"
		exit 1
		;;
esac