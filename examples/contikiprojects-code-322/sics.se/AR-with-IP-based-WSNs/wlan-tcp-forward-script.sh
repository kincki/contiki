#!/bin/bash

#defensive programming
set -o nounset # set -u -> exit your script if you try to use an uninitialised variable
set -o errexit # set -e -> exit the script if any statement returns a non true r eturn value

targetNodeIP1=172.16.1.0
targetNodeIP2=172.16.2.0
targetNodeIP3=172.16.3.0
targetNodeIP4=172.16.4.0
targetNodePort=8080
targetNodePort2=8181
targetNodePort3=8282
targetNodePort4=8383

#echo Target Node Ip: \"$targetNodeIP\" Port: \"$targetNodePort\"

iptables -A PREROUTING -t nat -i wlan0 -p tcp --dport $targetNodePort -j DNAT --to $targetNodeIP1:$targetNodePort
iptables -A PREROUTING -t nat -i wlan0 -p tcp --dport $targetNodePort2 -j DNAT --to $targetNodeIP2:$targetNodePort
iptables -A PREROUTING -t nat -i wlan0 -p tcp --dport $targetNodePort3 -j DNAT --to $targetNodeIP3:$targetNodePort
iptables -A PREROUTING -t nat -i wlan0 -p tcp --dport $targetNodePort4 -j DNAT --to $targetNodeIP4:$targetNodePort

iptables -A FORWARD -p tcp -i wlan0 -o tun0 -d $targetNodeIP1 --dport $targetNodePort -j ACCEPT
iptables -A FORWARD -p tcp -i wlan0 -o tun0 -d $targetNodeIP2 --dport $targetNodePort -j ACCEPT
iptables -A FORWARD -p tcp -i wlan0 -o tun0 -d $targetNodeIP3 --dport $targetNodePort -j ACCEPT
iptables -A FORWARD -p tcp -i wlan0 -o tun0 -d $targetNodeIP4 --dport $targetNodePort -j ACCEPT
/sbin/sysctl -w net.ipv4.ip_forward=1 
