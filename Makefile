intercept.so: intercept.c
	gcc -ggdb3 -Wall -fPIC -shared -o intercept.so intercept.c -lcares

check:
	LD_PRELOAD=./intercept.so  ping -c1 -v google.com
	DEBUG=1 LOCAL_IP4=127.0.0.1 LD_PRELOAD=./intercept.so  ping -c1 -v google.com
	DEBUG=1 LOCAL_IP4=127.0.0.1 LD_PRELOAD=./intercept.so  ping -c1 -v -4 google.com
	# DEBUG=1 LOCAL_IP4=127.0.0.1 LD_PRELOAD=./intercept.so  ping -c1 -v -6 google.com # if ipv6 enabled in your network
	DEBUG=1 LOCAL_IP4=127.0.0.1 SERVERS_CSV=127.0.0.53 LD_PRELOAD=./intercept.so   ping -c1 -v google.com
	DEBUG=1 SERVERS_CSV=127.0.0.53 LD_PRELOAD=./intercept.so   ping -c1 -v google.com
	DEBUG=1 SERVERS_CSV=8.8.8.8 LD_PRELOAD=./intercept.so   ping -c1 -v google.com

	LD_PRELOAD=./intercept.so  valgrind --error-exitcode=55 --show-reachable=no ping -c1 -v google.com

	# wlan0 ip
	# DEBUG=1  LOCAL_IP4=192.168.130.150 SERVERS_CSV=8.8.8.8 LD_PRELOAD=./intercept.so   ping -c1 -v google.com

	LD_PRELOAD=./intercept.so  nslookup google.com
	LD_PRELOAD=./intercept.so  dig +short google.com
	LD_PRELOAD=./intercept.so  host google.com

	# gethostbyname not implemented yet
	LD_PRELOAD=./intercept.so  getent hosts google.com; [[ $$? == 66 ]]
	# fping support not implemented yet
