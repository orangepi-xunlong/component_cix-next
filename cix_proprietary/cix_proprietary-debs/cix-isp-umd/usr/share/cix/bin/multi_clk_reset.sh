#!/bin/bash


devmem 0x14270300 32 0x10000800  	#piexl_sel:CSI0-->CSIDMA0;CSI1-->CSIDMA1    Enable left&right PHY
devmem 0x142D0300 32 0x32000800  	#piexl_sel:CSI2-->CSIDMA2;CSI3-->CSIDMA3    Enable left&right PHY
#CLK
devmem 0x0657211c 32 0x00001100
devmem 0x06572114 32 0x1070502
devmem 0x06572118 32 0x00000001

#csr_en_csi_rcsu0_apb_clk0[8]       APB_CSI0
#csr_en_csi_rcsu0_apb_clk1[9]       APB_CSI1
#csr_en_csi_rcsu0_apb_clk2[10]      APB_DPY0
#csr_en_csi_rcsu0_apb_clk3[11]      APB_DMA0
#csr_en_csi_rcsu0_apb_clk4[12]      APB_DMA1
#csr_en_phy0_psm_clk[13]            APB_DPY0_PSM
#csr_en_pixel_clk_if0_0[16]         PIXEL_STREAM0_CSI0
#csr_en_pixel_clk_if0_1[17]         PIXEL_STREAM1_CSI0
#csr_en_pixel_clk_if0_2[18]         PIXEL_STREAM2_CSI0
#csr_en_pixel_clk_if0_3[19]         PIXEL_STREAM3_CSI0
#csr_en_pixel_clk_if1[20]           PIXEL_STREAM0_CSI1
#csr_en_sys_clk0[21]				SYSCLK_CSIDMA0
#csr_en_sys_clk1[22]				SYSCLK_CSIDMA1
devmem 0x14270310 32 0x7f9f00


#csr_en_csi_rcsu0_apb_clk0[8]       APB_CSI2
#csr_en_csi_rcsu0_apb_clk1[9]       APB_CSI3
#csr_en_csi_rcsu0_apb_clk2[10]      APB_DPY1
#csr_en_csi_rcsu0_apb_clk3[11]      APB_DMA2
#csr_en_csi_rcsu0_apb_clk4[12]      APB_DMA3
#csr_en_phy0_psm_clk[13]            APB_DPY1_PSM
#csr_en_pixel_clk_if0_0[16]         PIXEL_STREAM0_CSI2
#csr_en_pixel_clk_if0_1[17]         PIXEL_STREAM1_CSI2
#csr_en_pixel_clk_if0_2[18]         PIXEL_STREAM2_CSI2
#csr_en_pixel_clk_if0_3[19]         PIXEL_STREAM3_CSI2
#csr_en_pixel_clk_if1[20]           PIXEL_STREAM0_CSI3
#csr_en_sys_clk0[21]				SYSCLK_CSIDMA2
#csr_en_sys_clk1[22]				SYSCLK_CSIDMA3
devmem 0x142D0310 32 0x7f9f00




#reset
devmem 0x16000400 32 0xffff0fff
devmem 0x16000400 32 0xffffffff
devmem 0x16000404 32 0xffffc003
devmem 0x16000404 32 0xffffffff


devmem 0x14270300 32 0x10000800  
devmem 0x142D0300 32 0x32000800
devmem 0x14270300 32 0x10000800  
devmem 0x142D0300 32 0x32000800
