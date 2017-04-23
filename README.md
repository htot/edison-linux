# edison-linux kernel 
This repository contains linux kernels for the Intel Edison.

It is pulled by [https://github.com/htot/meta-intel-edison](URL)

You will find more details on the kernel in the README file in this directory

# What is here

This is a fork of [https://github.com/01org/edison-linux](URL)

Currently the **master** branch tracks [https://github.com/01org/edison-linux/tree/edison-3.10.98](URL). There is not much to see here (last 8 Sep 2016). There are 3 new branches here:

  * **edison-3.10.17**. This branch has the edison-3.10.17 kernel from 01org with a patch <u>1)</u> to increase the HSU transmit buffer size, and to prevent interchar gaps when transmitting at high speed (tested at 2Mb/s). It also builds the FTDI usb serial converter and the SMSC95xx usb ethernet converter.
  * **edison-3.10.98**. This branch has the edison-3.10.98 kernel from 01org with a patch to increase the HSU transmit buffer size, and to prevent interchar gaps when transmitting at high speed (tested at 2Mb/s). It also builds the FTDI usb serial converter and the SMSC95xx usb ethernet converter.
  * **edison-3.10.17-rt**. This branch extends the 3.10.17 kernel by applying PREEMPT_Rt patches retrieved from [http://yoneken.sakura.ne.jp/share/rt_edison.tar.bz2](URL) by Kenta Yonekura and applies the required config changes to make them work.

<u>1)</u> The patch is a bit ugly as it simply resets head/tail of the circular TX buffer. I am open to proper solutions that would probably create a seperate linear TX DMA buffer and copy data from the circular serial TX buffer.

# How to use this

You normally would not want to clone this directly as cloning is done by linux-externalsrc.bb in the [https://github.com/htot/meta-intel-edison](URL) layer.