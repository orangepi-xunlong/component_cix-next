#!/bin/bash

#csi0
devmem 0x14280004 32 0x1   			#enable front and protocol
devmem 0x14280008 32 0x44			#lan_nb[7:4]  en_lrte[2]
devmem 0x14280108 32 0x10000		#All VC select
devmem 0x1428010C 32 0x100			#FIFO mode
devmem 0x14280100 32 0x1			#stream0 start


#CSIDMA0
#to ddr 4K YUV422-8bit 3840*2160
devmem 0x142b0138 32 0x1E00				#stride: size*bits
#devmem 0x142b0118 32 0xA0000000
#devmem 0x142b0118 32 0xBFE00000			#bufffer data addr
devmem 0x142b0104 32 0x1E005			#data Fomate
devmem 0x142b0028 32 0x08700F00			#size : 2160X3840
devmem 0x142b0108 32 0x1D00
devmem 0x142b000C 32 0x1F			#CNT_EN_CTL
devmem 0x142b0004 32 0x4			#DMA_START
devmem 0x142b0000 32 0x2			#DMA_BRIDGE_EN