#EXTRA CFLAGS
ccflags-y += -Werror

# Merrifield board
snd-merr-saltbay-wm8958-objs := merr_saltbay_wm8958.o
snd-merr-dpcm-wm8958-objs := merr_dpcm_wm8958.o
snd-merr-dpcm-dummy-objs := merr_dpcm_dummy.o
snd-merr-dpcm-wm8731-objs := merr_dpcm_wm8731.o

ifdef CONFIG_SST_MRFLD_DPCM
	obj-$(CONFIG_SND_MRFLD_MACHINE) += snd-merr-dpcm-wm8958.o
	obj-$(CONFIG_SND_MRFLD_MACHINE) += snd-merr-dpcm-dummy.o
	obj-$(CONFIG_SND_MRFLD_MACHINE) += snd-merr-dpcm-wm8731.o
else
	obj-$(CONFIG_SND_MRFLD_MACHINE) += snd-merr-saltbay-wm8958.o
endif
