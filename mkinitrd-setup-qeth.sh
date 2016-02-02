#!/bin/bash
#
#%stage: device
#%depends: network
#

if [ -n "${DEBUG}" ]; then
  DEBUG="yes"
else DEBUG="no"
fi

mesg () {
    echo "$@"
}

debug_mesg () {
    case "$DEBUG" in
        yes) mesg "$@" ;;
        *) ;;
    esac
}

copy_udev_rule ()
{
    local interface
    interface=$1
    if [ -e /sys/class/net/$interface/device ]; then
        debug_mesg /sys/class/net/$interface/device does exist.
        devpath=$(cd -P "/sys/class/net/$interface/device"; echo $PWD)
        ccwid=${devpath##*/}
        devtype=${devpath%/*}
        devtype=${devtype##*/}
        read if_name < ${devpath}/if_name
        if_type=${if_name:0:3}
        if [ "${if_type}" == "hsi" ]; then
            devtype="hsi"
        fi
        debug_mesg Will copy the udev rules for ${if_name} which is device ${ccwid}.
        if [ -f /etc/udev/rules.d/51-${devtype}-${ccwid}.rules ]; then
            cp /etc/udev/rules.d/51-${devtype}-${ccwid}.rules $tmp_mnt/etc/udev/rules.d
            load_qeth=1;
        fi
    else
        mesg /sys/class/net/$interface/device doesn\'t exist!
        exit 1
    fi
}

for interface in $static_interfaces $dhcp_interfaces
  do if grep -q VLAN /etc/sysconfig/network/ifcfg-$interface ; then
       debug_mesg "This is a VLAN interface"
       eval $(grep ETHERDEVICE /etc/sysconfig/network/ifcfg-$interface)
       debug_mesg The value of ETHERDEVICE is $ETHERDEVICE
       copy_udev_rule $ETHERDEVICE
     elif grep -q ^BONDING_SLAVE /etc/sysconfig/network/ifcfg-$interface; then
            debug_mesg "This is a channel bonded interface"
            for slave in $(grep ^BONDING_SLAVE /etc/sysconfig/network/ifcfg-$interface | cut -f1 -d=)
              do eval $(grep ^$slave /etc/sysconfig/network/ifcfg-$interface)
                 copy_udev_rule ${!slave}
              done
     else debug_mesg This is not a VLAN or channel bonded interface
          copy_udev_rule $interface
     fi
done
