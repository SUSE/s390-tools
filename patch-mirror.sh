# Patches:
#wget -m -np http://www10.software.ibm.com/developerworks/opensource/linux390/alpha_src/
PATCHES=www10.software.ibm.com/developerworks/opensource/linux390/alpha_src

# md5sums:
#wget -m -np http://www10.software.ibm.com/developerworks/opensource/linux390/MD5/alpha_src/
MD5SUMS=www10.software.ibm.com/developerworks/opensource/linux390/MD5/alpha_src


set -eu

TOPDIR=$PWD
#ALLTGZDIR=$TOPDIR/$PATCHES
#ALLMD5DIR=$TOPDIR/$MD5SUMS
ALLTGZDIR=$TOPDIR/ALL/TGZ
ALLMD5DIR=$TOPDIR/ALL/MD5
MD5DIR=$TOPDIR/MD5
TGZDIR=$TOPDIR/TGZ

create_alldirs () {
	old     $ALLTGZDIR
	mkdir -p $ALLTGZDIR

	cp -av $PATCHES/*.gz          $ALLTGZDIR
	cp -av $PATCHES/skua/*.gz     $ALLTGZDIR

	old     $ALLMD5DIR
	mkdir -p $ALLMD5DIR

	cp -av $MD5SUMS/*.md5          $ALLMD5DIR
	cp -av $MD5SUMS/skua/*.md5     $ALLMD5DIR
}
create_alldirs

old $MD5DIR
mkdir $MD5DIR

cd $ALLTGZDIR
for file in *.gz 
do
	echo "Creating md5sum for $file..."
	md5sum $file > $MD5DIR/$file.md5
done

old $TGZDIR
mkdir $TGZDIR

cd $MD5DIR

rm -fv `ls | grep -v s390-tools`

for md5 in *.md5
do
	if [ ! -f $ALLMD5DIR/$md5 ]
	then
		echo "md5sum for $md5 does not exist!"
		exit 2
	fi
	if cmp $md5 $ALLMD5DIR/$md5
	then
		echo "Checked: $md5"
		cp -av $ALLTGZDIR/${md5%.md5} $TGZDIR
	else
		echo "md5sum $md5 does not match:"
		echo "Local:"
		cat $md5
		echo "IBM:"
		cat $ALLMD5DIR/$md5
		exit 1
	fi
done



#old IBM-md5sums
#mv www10.software.ibm.com/developerworks/opensource/linux390/MD5/src IBM-md5sums
#tar cvfz IBM-md5sums.tar.gz IBM-md5sums
