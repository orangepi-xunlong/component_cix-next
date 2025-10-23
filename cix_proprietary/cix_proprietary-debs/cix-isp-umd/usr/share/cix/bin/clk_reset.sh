#!/bin/bash

#CLK
devmem 0x0657211c 32 0x00001100		#en_gater_isp_aclk[8] | en_gater_isp_sclk[12]  : enable isp_ack &isp_sclk 
devmem 0x06572114 32 0x1070502		#set coef_div_cfg_isp_aclk[27:24] to 1         : set isp_ack to 600M 
devmem 0x06572118 32 0x00000001     #set coef_div_cfg_isp_sclk[3:0] to 1           : set isp_sck to 600M 

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


#reset
#isp_sreset_n | isp_areset_n | isp_hreset_n | isp_gdcreset_n [12:15]	
devmem 0x16000400 32 0xffff0fff
devmem 0x16000400 32 0xffffffff

#csidphy_prst0_n |csidphy_cmnrst0_n | csi0_rst_n | csidphy_prst1_n| csidphy_cmnrst1_n
#csi1_rst_n | csi2_rst_n | csi3_rst_n | csibridge0_rst_n |csibridge1_rst_n
#csibridge2_rst_n | csibridge3_rst_n   [2:13]
devmem 0x16000404 32 0xffffc003
devmem 0x16000404 32 0xffffffff
