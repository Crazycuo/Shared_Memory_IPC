#########################################################################
# File Name: start.sh
# Author: Crazycuo
# mail: 740094202@qq.com
# Created Time: 2016年03月10日 星期四 20时03分06秒
#########################################################################
#!/bin/bash
cd read
	rm shmread
	make
	cp shmread ../	
cd ../write
	rm shmwrite
	make
	cp shmwrite ../

