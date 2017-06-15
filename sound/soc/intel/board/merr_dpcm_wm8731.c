/*
 *  merr_dpcm_wm8731.c - ASoc DPCM Machine driver for Intel Edison
 *
 *  Author: Sergey Kiselev <sergey.kiselev@intel.com>
 *  Based on merr_dpcm_wm8958.c
 *  Copyright (C) 2013 Intel Corp
 *  Author: Vinod Koul <vinod.koul@intel.com>
 *  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/async.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/intel_scu_pmic.h>
#include <asm/intel_mid_rpmsg.h>
#include <asm/intel_sst_mrfld.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <linux/input.h>

#include "../../codecs/wm8731.h"

/* Codec crystal clk rate */
#define CODEC_SYSCLK_RATE			12288000

static int mrfld_8731_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params)
{
	unsigned int fmt;
	int ret = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;

	/* Edison is master, WM8731 is slave */
	fmt =	SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_IB_NF
		| SND_SOC_DAIFMT_CBS_CFS;

	ret = snd_soc_dai_set_fmt(codec_dai, fmt);
	if (ret < 0) {
		pr_err("Can't set codec DAI configuration %d\n", ret);
		return ret;
	}

	/* Set sysclk */
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8731_SYSCLK_XTAL,
			CODEC_SYSCLK_RATE, SND_SOC_CLOCK_IN);
	if (ret < 0) {
		pr_err("Can't set WM8731 SYSCLK: %d\n", ret);
		return ret;
        }
	
	return ret;
}

static int mrfld_8731_codec_fixup(struct snd_soc_pcm_runtime *rtd,
			    struct snd_pcm_hw_params *params)
{
	struct snd_interval *rate = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_RATE);
	struct snd_interval *channels = hw_param_interval(params,
			SNDRV_PCM_HW_PARAM_CHANNELS);
	struct snd_mask *mask = hw_param_mask(params,
			SNDRV_PCM_HW_PARAM_FORMAT);

	pr_debug("Invoked %s for dailink %s\n", __func__, rtd->dai_link->name);

	/* The DSP will covert the FE rate to 48k, stereo, 24bits */
	rate->min = rate->max = 48000;
	channels->min = channels->max = 2;

	/* set SSP2 to 24-bit */
	snd_mask_none(mask);
	snd_mask_set(mask, SNDRV_PCM_FORMAT_S24_LE);
	return 0;
}

static const struct snd_soc_dapm_widget widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_LINE("Line Jack", NULL),
};

static const struct snd_soc_dapm_route map[] = {
	/* headphones are connected to LHPOUT, RHPOUT */
	{"Headphone Jack", NULL, "LHPOUT"},
	{"Headphone Jack", NULL, "RHPOUT"},

	/* speaker is connected to LOUT, ROUT */
	{"Speaker", NULL, "LOUT"},
	{"Speaker", NULL, "ROUT"},

	/* mic is connected to MICIN */
	{"MICIN", NULL, "Mic Jack"},

	/* line input is connected to LLINEIN, RLINEIN */
	{"LLINEIN", NULL, "Line Jack"},
	{"RLINEIN", NULL, "Line Jack"},

	/* codec BE connections */
	{"Playback", NULL, "codec_out0"},
	{"Playback", NULL, "codec_out1"},
	{"codec_in0", NULL, "Capture"},
	{"codec_in1", NULL, "Capture"},
};

static int mrfld_8731_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;

	snd_soc_dapm_enable_pin(dapm, "Headphone Jack");
	snd_soc_dapm_enable_pin(dapm, "Speaker");
	snd_soc_dapm_enable_pin(dapm, "Mic Jack");
	snd_soc_dapm_enable_pin(dapm, "Line Jack");

	/* signal a DAPM event */
	snd_soc_dapm_sync(dapm);

	return 0;
}

static const unsigned int wm8731_rates_12288000[] = {
	8000, 32000, 48000, 96000,
};

static struct snd_pcm_hw_constraint_list wm8731_constraints_12288000 = {
	.list = wm8731_rates_12288000,
	.count = ARRAY_SIZE(wm8731_rates_12288000),
};

static int mrfld_8731_startup(struct snd_pcm_substream *substream)
{
	/* Set constraints for WM8731 with 12.288 MHz crystal */
	return snd_pcm_hw_constraint_list(substream->runtime, 0,
			SNDRV_PCM_HW_PARAM_RATE,
			&wm8731_constraints_12288000);
}

static struct snd_soc_ops mrfld_8731_ops = {
	.startup = mrfld_8731_startup,
};

static struct snd_soc_ops mrfld_8731_be_ssp2_ops = {
	.hw_params = mrfld_8731_hw_params,
};

struct snd_soc_dai_link mrfld_8731_msic_dailink[] = {
	[MERR_DPCM_AUDIO] = {
		.name = "Media Audio Port",
		.stream_name = "Edison Audio",
		.cpu_dai_name = "Headset-cpu-dai",
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.platform_name = "sst-platform",
		.ignore_suspend = 1,
		.dynamic = 1,
		.init = mrfld_8731_init,
		.ops = &mrfld_8731_ops,
	},
	/* back ends */
	{
		.name = "SSP2-Codec",
		.be_id = 1,
		.cpu_dai_name = "ssp2-codec",
		.platform_name = "sst-platform",
		.no_pcm = 1,
		.codec_dai_name = "wm8731-hifi",
		.codec_name = "wm8731.1-001a",
		.be_hw_params_fixup = mrfld_8731_codec_fixup,
		.ignore_suspend = 1,
		.ops = &mrfld_8731_be_ssp2_ops,
	},
	{
		.name = "SSP1-BTFM",
		.be_id = 2,
		.cpu_dai_name = "snd-soc-dummy-dai",
		.platform_name = "snd-soc-dummy",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.ignore_suspend = 1,
	},
	{
		.name = "SSP0-Modem",
		.be_id = 3,
		.cpu_dai_name = "snd-soc-dummy-dai",
		.platform_name = "snd-soc-dummy",
		.no_pcm = 1,
		.codec_name = "snd-soc-dummy",
		.codec_dai_name = "snd-soc-dummy-dai",
		.ignore_suspend = 1,
	},
};

#ifdef CONFIG_PM_SLEEP
static int snd_mrfld_8731_prepare(struct device *dev)
{
	pr_debug("In %s\n", __func__);
	snd_soc_suspend(dev);
	return 0;
}

static void snd_mrfld_8731_complete(struct device *dev)
{
	pr_debug("In %s\n", __func__);
	snd_soc_resume(dev);
	return;
}

static int snd_mrfld_8731_poweroff(struct device *dev)
{
	pr_debug("In %s\n", __func__);
	snd_soc_poweroff(dev);
	return 0;
}
#else
#define snd_mrfld_8731_prepare NULL
#define snd_mrfld_8731_complete NULL
#define snd_mrfld_8731_poweroff NULL
#endif

/* SoC card */
static struct snd_soc_card snd_soc_card_mrfld = {
	.name = "wm8731-audio",
	.dai_link = mrfld_8731_msic_dailink,
	.num_links = ARRAY_SIZE(mrfld_8731_msic_dailink),
	.dapm_widgets = widgets,
	.num_dapm_widgets = ARRAY_SIZE(widgets),
	.dapm_routes = map,
	.num_dapm_routes = ARRAY_SIZE(map),
};

static int snd_mrfld_8731_mc_probe(struct platform_device *pdev)
{
	int ret_val = 0;

	pr_debug("In %s\n", __func__);

	/* register the soc card */
	snd_soc_card_mrfld.dev = &pdev->dev;
	ret_val = snd_soc_register_card(&snd_soc_card_mrfld);
	if (ret_val) {
		pr_err("snd_soc_register_card failed %d\n", ret_val);
		return ret_val;
	}
	platform_set_drvdata(pdev, &snd_soc_card_mrfld);
	pr_info("%s successful\n", __func__);
	return ret_val;
}

static int snd_mrfld_8731_mc_remove(struct platform_device *pdev)
{
	struct snd_soc_card *soc_card = platform_get_drvdata(pdev);
	struct mrfld_8731_mc_private *drv = snd_soc_card_get_drvdata(soc_card);

	pr_debug("In %s\n", __func__);
	kfree(drv);
	snd_soc_card_set_drvdata(soc_card, NULL);
	snd_soc_unregister_card(soc_card);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

const struct dev_pm_ops snd_mrfld_8731_mc_pm_ops = {
	.prepare = snd_mrfld_8731_prepare,
	.complete = snd_mrfld_8731_complete,
	.poweroff = snd_mrfld_8731_poweroff,
};

static struct platform_driver snd_mrfld_8731_mc_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "mrfld_wm8731",
		.pm = &snd_mrfld_8731_mc_pm_ops,
	},
	.probe = snd_mrfld_8731_mc_probe,
	.remove = snd_mrfld_8731_mc_remove,
};
module_platform_driver(snd_mrfld_8731_mc_driver)
MODULE_DESCRIPTION("ASoC Machine driver for Intel Edison with WM8731 codec");
MODULE_AUTHOR("Sergey Kiselev <sergey.kiselev@intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:mrfld_wm8731");
