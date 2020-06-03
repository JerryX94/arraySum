bsub -b -I -J XJY_arraySum -q q_sw_share -n 1 -cgsp 64 -host_stack 1024 -share_size 6500 -sw3runarg "-P master" ./build/arraySum
