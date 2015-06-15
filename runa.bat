adb shell "/data/local/tmp/busybox nohup  /data/local/tmp/memtest -t bitchk 100M 1 &"
adb shell "/data/local/tmp/busybox nohup  /data/local/tmp/pyrun /data/local/tmp/ddr.py &"
