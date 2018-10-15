CMD=$1
BASEDIR=$(dirname "$0")

case "$CMD" in
	"install")
		cp -v $BASEDIR/../eu.onderweg.follow.plist ~/Library/LaunchAgents
		;;
	"start")
        launchctl load eu.onderweg.follow.plist
        launchctl start eu.onderweg.follow
        launchctl list | grep 'eu.onderweg.follow'
		;;
	"stop")
	    launchctl unload eu.onderweg.follow.plist
		;;
	"logs")
	    tail /tmp/follow_the_sun.log
		;;		
	*)
		echo "Usage: $0 [install|start|stop|logs]"
		exit 1
		;;
esac