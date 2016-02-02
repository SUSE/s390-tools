#!/bin/bash
#
# mkdump
#
# Copyright (c) 2004 Hannes Reinecke, SuSE AG
#
# Preparing a disk device for use as S/390 dump device
#
# Return values:
# 0       Success
# 11      Invalid or unuseable disk (fatal)
# 12      Minor error, can be corrected with -f
# 13      Missing support programs
# other   Support program failed
#

DEBUG=
FORCE=

VOLID=/sbin/vol_id
MKFS=/sbin/mke2fs
SFDISK=/sbin/sfdisk
FDASD=/sbin/fdasd
DASDVIEW=/sbin/dasdview
DASDFMT=/sbin/dasdfmt
ZIPL=/sbin/zipl
UDEVINFO=/sbin/udevinfo
MNTPOINT=/var/run/mkdump.$PPID

# Check for installed programs
if [ ! -x ${VOLID} ]; then
    echo "vol_id not found, please install udev"
    exit 13
fi

if [ ! -x ${SFDISK} ]; then
    echo "${SFDISK} not found, please install"
    exit 13
fi

if [ ! -x ${ZIPL} ]; then
    echo "zipl not found, please install s390-tools"
    exit 13
fi

if [ ! -x ${UDEVINFO} ]; then
    UDEVINFO=/usr/bin/udevinfo
    if [ ! -x ${UDEVINFO} ]; then
	UDEVINFO=false
    fi
fi

#
# Subroutines
#
check_devsize() {
    local geomstr
    local size
    local mazsize

    geomstr=$(${SFDISK} -l $1 | grep of)
    if [ -z "$geomstr" ]; then
	echo "-1"
    else
	set -- $geomstr
	maxsize=`echo "2 k 10 1024 * $5 1024 / / 0.99 + 0 k 1 / p q" |dc`
	echo "$maxsize"
    fi
}

check_partitions() {
    local device
    local num_part

    num_part=0
    device=${1#/dev/}
    while read maj min size dev; do
	case "$dev" in
	    $device*)
		if [ "$dev" != "$device" ]; then
		    num_part=$(expr $num_part + 1)
		fi
		;;
	esac
    done < /proc/partitions
    echo "$num_part"
}

prepare_dasddump() {
    local device
    local fmtstr
    local err

    device=$1

    fmtstr=`${DASDVIEW} -x -f $device | grep ^format | cut -f 3`
    case "$fmtstr" in
	NOT*)
	    echo "Unformatted disk, formatting."
	    ${ECHO} ${DASDFMT} -b 4096 -y -f $device
	    err=$?
	    if [ $err -ne 0 ]; then
		echo "dasdfmt failed with $err"
		return $err
	    fi
	    ;;
	LDL*)
	    echo "Linux disk layout, reformatting."
	    ${ECHO} ${DASDFMT} -b 4096 -y -f $device
	    err=$?
	    if [ $err -ne 0 ]; then
		echo "dasdfmt failed with $err"
		return $err
	    fi
	    ;;
	CDL*)
	    echo "Compatible layout, ok to use."
	    ;;
	*)
	    echo "Unknown layout ($fmtstr), cannot use disk."
	    return 11
	    ;;
    esac

    num_part=$(check_partitions $device)
    if [ "$num_part" -gt 1 -a -z "$FORCE" ]; then
	echo "$num_part partitions detected, cannot use disk."
	return 12
    fi

    if [ "$FORCE" -o "$num_part" -lt 1 ]; then
	echo "Re-partitioning disk."
	${ECHO} ${FDASD} -a $device
	err=$?
	if [ $err -ne 0 ]; then
	    echo "FDASD failed with $err"
	    return $err
	fi
    fi

    # Wait for udev to finish processing
    /sbin/udevsettle

    if [ -z "$FORCE" ]; then
	volid_result=$($ECHO $VOLID -t ${device}1)
	if [ -n "$volid_result" ]; then
	    echo "Device ${device}1 already contains a filesystem of type $volid_result"
	    return 12
	fi
    fi
    return 0
}

install_dasddump() {
    local device
    local err

    device=$1

    echo "Creating dump record"
    ${ECHO} ${ZIPL} -n -d ${device}1
    err=$?
    if [ $err -ne 0 ]; then
	echo "FAILED with error code $err"
    fi

    return $err
}

prepare_zfcpdump() {
    local device
    local err

    device=$1

    num_part=$(check_partitions $device)
    max_size=$(check_devsize $device)
    if [ -z "$DEBUG" -a "$max_size" -lt 0 ]; then
	echo "Cannot get disk size information"
	return 11
    fi

    if [ "$num_part" -eq 1 -a -z "$FORCE" ]; then
	echo "$num_part partition detected, cannot use disk."
	return 12
    fi
    if [ "$num_part" -gt 2 -a -z "$FORCE" ]; then
	echo "$num_part partitions detected, cannot use disk."
	return 12
    fi

    if [ "$FORCE" -o "$num_part" -lt 1 ]; then
	echo "Re-partitioning disk ${device}"

	${ECHO} ${SFDISK} ${device} >/dev/null 2>&1 <<EOF
0,$max_size,L,*
$max_size,,L
;
;
EOF
	err=$?
	if [ $err -ne 0 ]; then
	    echo "sfdisk failed with $err"
	    return $err
	fi
    fi

    # Wait for udev to finish processing
    /sbin/udevsettle

    volid_result=`$ECHO $VOLID -t ${device}1`
    if [ "$volid_result" != "ext2" -a -z "$FORCE" ]; then
	echo "Fstype for Device ${device}1 is $volid_result, cannot use disk."
	return 12
    fi

    if [ "$FORCE" ]; then
	echo -n "Formatting device ${device}1: "
	${ECHO} ${MKFS} -q ${device}1
	err=$?
	if [ $err -eq 0 ]; then
	    echo "ok"
	else
	    echo "failed with $err"
	    return $err
	fi
    fi

    volid_result=`$ECHO $VOLID -t ${device}2`
    if [ "$volid_result" != "ext2" -a -z "$FORCE" ]; then
	echo "Fstype for Device ${device}2 is $volid_result, cannot use disk."
	return 11
    fi

    if [ "$FORCE" ]; then
	echo -n "Formatting device ${device}2: "
	${ECHO} ${MKFS} -q ${device}2
	err=$?
	if [ $err -eq 0 ]; then
	    echo "ok"
	else
	    echo "failed with $err"
	    return $err
	fi
    fi

    return 0
}

install_zfcpdump() {
    local device
    local err

    device=$1

    echo -n "Mounting device: "
    ${ECHO} mkdir -p ${MNTPOINT}
    ${ECHO} mount ${device}1 ${MNTPOINT}
    err=$?
    if [ $err -eq 0 ]; then
	echo "ok"
    else
	echo "failed with $err"
	return $err
    fi

    [ -d ${MNTPOINT}/boot ] || ${ECHO} mkdir ${MNTPOINT}/boot
    [ -d ${MNTPOINT}/boot/zipl ] || ${ECHO} mkdir ${MNTPOINT}/boot/zipl

    echo "Creating dump record"
    ${ECHO} ${ZIPL} -D ${device}2 -t ${MNTPOINT}/boot/zipl
    err=$?
    if [ $err -ne 0 ]; then
	echo "FAILED with error code $err"
    fi

    ${ECHO} umount ${MNTPOINT}
    ${ECHO} rmdir ${MNTPOINT}
    
    return $err
}

usage() {
    cat <<EOF
Prepare a device for use as S/390 dump device. Supported devices are
ECKD DASD and zfcp disks. If the device is found to be already formatted
and/or configured, the script will refuse to install the dump record
unless the -f switch ('force') is given.

mkdump [options] device

options:
    -d   Debug mode, do not execute programs
    -f   Force installation of dump record
    -h   This text

EOF
exit 1
}

#
# main script program
#

while getopts :fh a ; do
    case $a in
	d)
	    DEBUG=1
	    ;;
	f)
	    FORCE=1
	    ;;
	h)  
	    usage
	    exit 0
	    ;;
	t)
	    TEST=1
	    ;;
    esac
done
shift $(expr $OPTIND - 1)

if [ "$DEBUG" ]; then
    ECHO=echo
fi

if [ $# -lt 1 ]
then
	usage
fi
DEVICE=$1

# Check whether the given device is valid
DEVPATH=`$UDEVINFO -q path -n ${DEVICE#/dev/}`
err=$?
if [ $err -eq 1 ]; then
    DEVPATH="/block/${DEVICE#/dev/}"
    if [ ! -d "/sys$DEVPATH" ]; then
	echo "Device $DEVICE is not found"
	echo "either the device does not exist or refers to a partition"
	exit 11
    fi
    DEVNODE=${DEVICE#/dev/}
elif [ $err -eq 0 ]; then
    DEVNODE=$($UDEVINFO -q name -p $DEVPATH)
    DEVICE=/dev/$DEVNODE
fi

if [ ! -d "/sys$DEVPATH/device" ]; then
    echo "Invalid dump device $DEVICE (no device link)"
    exit 11
fi

# Check for DASD
if [ -r "/sys$DEVPATH/device/discipline" ]; then
    read DEVTYPE < "/sys$DEVPATH/device/discipline"
    # Only newer zipl-versions support dump to FBA disks
    if [ "$DEVTYPE" = "FBA" ]; then
	echo "Dump to FBA disks not yet supported"
	exit 11
    fi
fi

# Check for zfcp
if [ -e "/sys$DEVPATH/device/hba_id" ]; then
    read DEVTYPE < "/sys$DEVPATH/device/type"
    # SCSI type '0' is disks; we can't really dump to anything else
    if [ "$DEVTYPE" -eq 0 ]; then
	DEVTYPE="ZFCP"
    else
	DEVTYPE=
    fi
fi

if [ -z "$DEVTYPE" ]; then
    echo Unsupported disk type
    exit 11
fi

if [ "$DEVTYPE" = "ZFCP" ]; then
    if [ -d ${MNTPOINT} ]; then
	echo "Mountpoint $MNTPOINT exists, aborting"
	exit 13
    fi

    prepare_zfcpdump $DEVICE
    err=$?
    if [ $err -ne 0 ]; then
	exit $err
    fi

    install_zfcpdump $DEVICE
    err=$?
    if [ $err -ne 0 ]; then
	exit $err
    fi
else
    prepare_dasddump $DEVICE
    err=$?
    if [ $err -ne 0 ]; then
	exit $err
    fi
    
    install_dasddump $DEVICE
    err=$?
    if [ $err -ne 0 ]; then
	exit $err
    fi
fi

exit 0

