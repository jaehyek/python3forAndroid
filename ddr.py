# -*- coding: utf-8 -*-
from socket import socket, AF_INET, SOCK_STREAM
import inspect
import os
import time


import subprocess
import threading

__author__ = 'jaehyek.choi'

"""
purpose :

"""

"""
from __future__ import division
"""


class CLSVAR():
    pass

class clsMsgClent():
    def __init__(self,ID="id",modelname="model", address='172.21.26.41'):
        self.address = address
        self.ID = ID
        self.modelname = modelname
        self.port = 20000
        self.NoRetry = 5
        self.onlylocal = False
        self.s = 0
        # self.CreateSocket()

    def CreateSocket(self):
        temptry = self.NoRetry
        while temptry :
            try:
                self.s = socket(AF_INET, SOCK_STREAM)
                self.s.connect((self.address, self.port))
                break
            except :
                print ("can't connect to sever ")
                time.sleep(1)
                print ("retry to connect ")
                temptry -= 1
        if temptry == 0 :
            del self.s
            self.s = 0
            return False
        else:
            return True
    def SendMsg(self, msg):
        print(msg)
        if (self.onlylocal == True) :
            return

        if self.s == 0 :
            if self.CreateSocket() == False :
                print("Cant Create socket")
                return

        curframe = inspect.currentframe()
        calframe = inspect.getouterframes(curframe, 2)

        # caution:  \n  must be attached.
        # msg = self.ID + "," + self.modelname + "," + mod.__name__ + "," +  msg + "\n"
        msg = self.ID + "," + self.modelname + "," + calframe[1][3] + "," +  msg + "\n"
        temptry = self.NoRetry

        while(temptry):
            try:
                self.s.send(msg.encode())
                resp = self.s.recv(8192)
                break
            except :
                print ("trying to send msg ....")
                self.CreateSocket()
                temptry -= 1
                continue
        if temptry == 0 :
            self.s.close()
            print ('fail to send msg .....')


def getModelName(clsvar):
    return os.popen("getprop ro.product.model").readline().strip()

def getDeviceSerialNo(clsvar):
    return os.popen("getprop ro.serialno").readline().strip()

def getDDRSize(clsvar):
    return os.popen("cat /proc/meminfo").readline().split()[-2][:-3]

class CoreOp():
    def __init__(self):
        pass

    def getCoreInfo(self, clsvar):

        strcmd = "cat /sys/devices/system/cpu/present"
        strcmd = "cat /sys/devices/system/cpu/cpu%s/cpufreq/scaling_available_frequencies"

        return  "300000 384000 600000 787200 998400 1094400 1190400".split()

    def getCoreFreq(self, clsvar):
        strcmd = "cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq"
        strret = os.popen(strcmd).readline().strip()
        return strret

    def setCoreFreq(self, clsvar, corefreq):

        strcmd = "echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
        os.system(strcmd)

        strcmd = "echo %s > /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq" % corefreq
        os.system(strcmd)

        strcmd = "echo %s > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq" % corefreq
        os.system(strcmd)

        strret = self.getCoreFreq(clsvar)
        if strret == corefreq :
            return True
        else:
            return False

class DDROp():
    def __init__(self) :
        self.listDDRfreq = "200 400 533.3".split()

        strret = os.popen("cat  /sys/kernel/debug/msm-bus-dbg/shell-client/ab").readline().strip()
        if strret == "0" :
            os.system("echo 1 > /sys/kernel/debug/msm-bus-dbg/shell-client/ab")
            os.system("echo 1 > /sys/kernel/debug/msm-bus-dbg/shell-client/mas")
            os.system("echo 512  > /sys/kernel/debug/msm-bus-dbg/shell-client/slv")
            time.sleep(1)

    def getDDRInfo(self):
        return self.listDDRfreq


    def getDDRFreq(self):
        strcmd = "cat /sys/kernel/debug/clk/bimc_clk/measure"
        strret = os.popen(strcmd).readline().strip()
        return strret

    def setDDRFreq(self, ddrfreq):

        notry = 5
        while(notry) :
            freq = int(float(ddrfreq) * 1000000 * 8)
            strcmd ="echo %s > /sys/kernel/debug/msm-bus-dbg/shell-client/ib"%freq
            print("cmd : %s"%strcmd)
            os.system(strcmd)
            os.system("echo 1  > /sys/kernel/debug/msm-bus-dbg/shell-client/update_request")
            time.sleep(1)

            freqcurr = self.getDDRFreq()
            print("freqcurr,%s"%freqcurr )

            if int(float(ddrfreq)) == int(int(int(freqcurr) + 99000 ) / 1000000) :
                return True
            notry -= 1

        return False


class DeepSleepWake():
    def __init__(self) :
        fileprefer = "/data/data/com.lge.emmctest/"


        if  os.path.exists(fileprefer) == False :
            raise Exception("No eMMCTest.apk")

    def sleep(self, sec):
        if sec <= 10 :
            sec = 10
        elif sec <= 20 :
            sec = 20
        else:
            sec = 30

        adbcmd = "am broadcast -a com.lge.emmctest.TEST_START_ACTION --include-stopped-packages "
        adbcmd += " --ez sleepwakeup true "
        adbcmd += " --ez randomwrite  false "
        adbcmd += " --ez sequentialwrite  false "
        adbcmd += " --ez swreset   false "
        adbcmd += " --el sleep_time  %s "
        adbcmd += " --el wakeup_time  5 "
        adbcmd += " --el repeat 1 "

        cmd = adbcmd%(sec)

        os.system(cmd)
        time.sleep(10)


def getDeepSleepCount(clsvar):

    strcmd = "cat /sys/kernel/debug/rpm_stats"

    deepsleepcount = 0
    strret = os.popen(strcmd).readlines()

    for line in strret :
        if "reserved" in line :
            deepsleepcount = line.split(":")[1].strip()
            break

    return int(deepsleepcount)

def checkbitflip(clsvar, msg_client):

    # read the memtest result
    os.system("setprop memtest other")
    os.system("echo 1 > startcmd")
    while(True) :
        time.sleep(0.5)
        strret = os.popen("getprop memtest").readline().strip()
        print("getprop memtest,%s"%strret)
        if strret == "pass" :
            msg_client.SendMsg("memtest,res,pass")
            os.system("echo 1 > pausecmd")
            return True
        elif strret == "other" :
            continue
        elif strret == "fail" :
            msg_client.SendMsg("memtest,res,fail")
            os.system("echo 1 > pausecmd")
            return False


def ddr():
    try:
        os.chdir("/data/local/tmp")
        clsvar = CLSVAR()

        clsvar.passcmd = "setprop memtest pass"
        clsvar.failcmd = "setprop memtest fail"
        clsvar.othercmd = "setprop memtest other"

        clsvar.modelname = getModelName(clsvar)
        clsvar.DeviceSerialNo = getDeviceSerialNo(clsvar)
        clsvar.DDRSize = getDDRSize(clsvar)
        clsvar.taskname = "DDRCheck"

        coreop = CoreOp()
        clsvar.corelist = coreop.getCoreInfo(clsvar)

        ddrop = DDROp()
        clsvar.ddrlist = ddrop.getDDRInfo()


        SERVERIP = '172.21.26.41'
        msg_client = clsMsgClent( "%s_%s"%(clsvar.DeviceSerialNo , clsvar.taskname) ,clsvar.modelname,SERVERIP )

        clsvar.listtaskcount = [aa for aa in range(1000)]
        clsvar.listdeepsleepcount = [aa for aa in range(10)]
        clsvar.deepsleepsec = 10

        clsdeepsleep = DeepSleepWake()
        countdeepsleepprev = getDeepSleepCount(clsvar)

        # pause the process "memtester"
        os.system("echo 1 > pausecmd ")

        for taskcount in clsvar.listtaskcount :

            for ddrfreq in clsvar.ddrlist :

                if ddrop.setDDRFreq(ddrfreq) == False:
                    msg_client.SendMsg("error,ddrfreq")
                    break

                for corefreq in clsvar.corelist :
                    coreop.setCoreFreq(clsvar, corefreq)

                    time.sleep(1)
                    if checkbitflip(clsvar, msg_client) == False :
                            return

                    for deepsleepcount in clsvar.listdeepsleepcount :
                        msg_client.SendMsg("curr_set,%s,%s,%s,%s"%(taskcount, ddrfreq ,corefreq,deepsleepcount))

                        clsdeepsleep.sleep(clsvar.deepsleepsec)

                        try:
                            time.sleep(clsvar.deepsleepsec+5)
                            countdeepsleep = getDeepSleepCount(clsvar)

                            if ( countdeepsleepprev == countdeepsleep) :
                                msg_client.SendMsg("No_Deep_Sleep")
                            else:
                                msg_client.SendMsg("countdeepsleep,%s"%countdeepsleep)
                            countdeepsleepprev = countdeepsleep

                        except:
                            msg_client.SendMsg("fail,DeepSleepCountReadingError")


                        if checkbitflip(clsvar, msg_client) == False :
                            return



    except KeyboardInterrupt:
        pass
    finally:
        os.system( "echo 1 > /data/local/tmp/stopcmd" )

if __name__ == "__main__":
    ddr()
