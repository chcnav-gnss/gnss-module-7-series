source /opt/imx6/environment-setup-cortexa7hf-vfp-neon-poky-linux-gnueabi
export LANGUAGE=en_us
set +x

VERSION=`svn info -r 'HEAD' | grep "Last Changed Rev:" | egrep -o "[0-9]+"`
CURDIR=$(pwd)

sed -i "s/^.*SVN_VERSION.*/#define SVN_VERSION    \"$VERSION\"/" $CURDIR/Version.h
#build a single project
build_project()
{
	echo -e "\033[32mbuilding project : " $1 "\033[0m"

	if [ -f Makefile ];then
		make all
	else
		echo -e "\033[31mProject \"$1\" do not have a \"Makefile\" yet\033[0m"
#	exit 1
	fi

	return 0
}

clean_project()
{
	echo -e "\033[32mcleanup project \033[0m"

	if [ -f Makefile ];then
		make clean
	else
		echo -e "\033[31mProject cgi do not have a \"Makefile\" yet\033[0m"
	fi

	return 0
}

usage()
{
	echo "`basename $0` args:"
	echo "all: make all"
	echo "clean: make clean"
	echo "$VERSION"
	echo "$CURDIR"
	echo
}

#check parameters and execute the commands
COMMAND=`basename $0`

if [ $# -lt 1 ]; then
	usage
	exit 1
fi

while [ $# -ne 0 ]
do
	case $1 in
		all)
			echo "build this project starting ..."
			build_project
			;;
		clean)
			echo "clean a single project..."
			clean_project
			;;
		help)
			usage
			;;
	esac

	shift
done

