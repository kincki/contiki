Contiki MB851 (STM32W108) port


------------------------------------------------------------------------------
Prerequisites:
------------------------------------------------------------------------------

In order to compile using the IAR toolchain:

- IAR Embedded Workbench for ARM 5.41.
- Windows operating system (required by IAR EWARM).
- In order to compile with makefiles: any GNU building toolset, like the one
  provided by Cygwin or MSYS (make, sed, ls).


In order to compile using the GNU toolchain:

- The GNU toolchain (e.g., the one from CodeSourcery, the latest 2010q1-188-arm
  none-eabi release is recommended).
- A Linux operating system or a Windows OS with a Linux-like environment (Cygwin
  is recommended).

------------------------------------------------------------------------------
Building applications:
------------------------------------------------------------------------------

> Using the IAR toolchain

  With IAR IDE:

   You can compile the examples with IAR IDE, if you like. Some project files
   for IAR are present under 'build' folder. See IAR documentation for more
   details about the IAR build system.


  With GNU Make:

   In order to compile a project using make utility and the IAR toolchain, make
   sure to have the IAR bin folder in the PATH variable.
   You have also to specify the variable IAR_PATH to let know the compiler where
   are some system specific header files (for example, if you are in Cygwin:
   IAR_PATH = C:/Program\ Files/IAR\ Systems/Embedded\ Workbench\ 5.4). You also
   have to tell Make to build with IAR, defining an 'IAR' environment variable.

   $ set IAR=1

   $ make

   To flash the binary file into the micro with JLink you can type:

   $ make application.flash

   where application is the name of your application.
   You must specify the PORT variable if you want to flash using the
   on-board serial bootloader, without JLink:

   $ make application.flash PORT=COM4


> Using the GNU toolchain

  With GNU Make:

  See the section above. If you don't define the IAR variable, make will try to
  build using the GNU ARM Toolchain.
	


In 'examples' folder there are some tested examples. 'apps' folder contains all
the applications required by the examples provided. Other examples and apps
can be found in the main project. (http://www.sics.se/contiki/)



------------------------------------------------------------------------------
Notes:
------------------------------------------------------------------------------

This project is based on the 2.4 version of contiki, but there are
some changes to the core files of the operating system.


Differences in core files between the official 2.4 version of Contiki
and the STM32W port:

Replaced all (void *) occurences with  (u8_t *) for compatibility with
IAR.

In core:

	net/uip.h: redefined uip_ipaddr_prefixcmp for arbitrary prefix lengths.
	net/uip6.c:
		now we accept all-node multicast UDP packets (e.g, they
				are widely used in rime-udp).
	net/mac/sicslowmac.c: added code for MAC filtering for testing
				purposes.
	net/rime/:
		in debugging strings: last two bytes of rime address are used to
		identify the node, instead of the first two.
	net/rime/netflood:
		changed hdrsize parameter in ipolite_send call.
	net/routing/rimeroute: significant change that allow the nodes to
		send packets outside the lowpan, forwarding them towards
		a gateway node. The module can also use an IPv6 routing table
		to implement a simple edge router.
		Added call to route_init() function.
	net/routing/rimeroute.h: added description of rimeroute module.
	net/routing/uip6-route-table.c/.h:	added
	net/routing/uip6-route.c/.h:		added

	sys/rtimer.c: Fixed a bug that prevent a timer to work when it expires
			before the previously inserted ones.
	sys/rtimer.h: rtimer_clock_t type is configurable.


In apps:

	webserver/httpd.c:
		corrected httpd_sprint_ip6 funcion;
		fixed bug related to missing deallocations.
	webserver/Makefile.webserver: removed file names that are already
				present in Makefile.include.


