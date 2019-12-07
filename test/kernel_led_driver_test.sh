#!/bin/env bash
#
# A simple bash script to test the gpio_driver kernel module
#

# Arbitary stuff
WAITTIME=0.1

# LED definitions
LED1="/dev/led0"
LED2="/dev/led1"
LED3="/dev/led2"
LED4="/dev/led3"

# Funkctions ---------------------------------------

# Initialize kernel driver
init(){
    # Set up LEDs
    sudo -- sh -c 'modprobe led_driver_quad'   
}

# Unload kernel driver
deinit(){
    # Clear all LEDs
    sudo -- sh -c "echo "off" > ${LED1}"
    sudo -- sh -c "echo "off" > ${LED2}"
    sudo -- sh -c "echo "off" > ${LED3}"
    sudo -- sh -c "echo "off" > ${LED4}"
    
    # Deinit LEDs
    sudo -- sh -c 'modprobe -r led_driver_quad'
}

# Callback function for SIGINT
on_exit(){
    deinit
    exit 1
}

# --------------------------------------------------

trap on_exit SIGINT 
init

while [ true ]
do
    # enable
    for i in $LED1 $LED2 $LED3 $LED4
    do
        sudo -- sh -c "echo "on" > ${i}";
        sleep ${WAITTIME};
    done
    
    # disable
    for i in $LED1 $LED2 $LED3 $LED4
    do
        sudo -- sh -c "echo "off" > ${i}";
        sleep ${WAITTIME};
    done

done
