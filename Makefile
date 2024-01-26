intercept.so: intercept.c
	gcc -Wall -fPIC -shared -o intercept.so intercept.c -lcares

check:
	LD_PRELOAD=./intercept.so  getent hosts google.com; [[ $$? == 66 ]]
	LD_PRELOAD=./intercept.so  nslookup google.com; [[ $$? == 77 ]]
	LD_PRELOAD=./intercept.so  ping -c1 google.com; [[ $$? == 77 ]]
	LD_PRELOAD=./intercept.so  dig google.com; [[ $$? == 77 ]]
