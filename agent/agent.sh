CMD=$1

case "$CMD" in
	"start")
        launchctl load eu.onderweg.follow.plist
        launchctl start eu.onderweg.follow
        launchctl list | grep 'eu.onderweg.follow'
		;;
	"stop")
	    launchctl unload eu.onderweg.follow.plist
		;;
	*)
		echo "Usage: $0 [start|stop]"
		exit 1
		;;
esac