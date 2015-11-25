Develop/test uart1-putchar.c works with real SerialForwarder's. 
Uses a Sky mote w the modified uart1-putchar.c.


#1
Start Cooja: sky-shell with modified uart1-putchar.c
Start serial socket (60001)

#2
Start SerialForwarder that connect to above simulated sky-shell
$ cd /cygdrive/c/home/tinyos-main-read-only/support/sdk/java
$ java -cp tinyos.jar net.tinyos.sf.SerialForwarder -debug -comm network@localhost:60001 -no-gui

#3
Start another Cooja and connect with a "TwistMote" (which uses tinyos.jar SerialForwarder)
You have do modify TwistMoteType.java in a delicate way, not well documented here.
* Make sure that the SerialForwader debugging output matches the port that TwistPlugin connects to.
For example 9001 or 9002 or something.
* Comment the entire "Setup port forwarder via ssh"
* Comment the isConnected() check of (conn == null)
* Click Connect in the Twist plugin
