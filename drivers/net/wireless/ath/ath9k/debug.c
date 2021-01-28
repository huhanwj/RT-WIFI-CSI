/*
 * Copyright (c) 2008-2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/export.h>
#include <linux/relay.h>
#include <asm/unaligned.h>

#include "ath9k.h"

#define REG_WRITE_D(_ah, _reg, _val) \
	ath9k_hw_common(_ah)->ops->write((_ah), (_val), (_reg))
#define REG_READ_D(_ah, _reg) \
	ath9k_hw_common(_ah)->ops->read((_ah), (_reg))

void ath9k_debug_sync_cause(struct ath_softc *sc, u32 sync_cause)
{
	if (sync_cause)
		sc->debug.stats.istats.sync_cause_all++;
	if (sync_cause & AR_INTR_SYNC_RTC_IRQ)
		sc->debug.stats.istats.sync_rtc_irq++;
	if (sync_cause & AR_INTR_SYNC_MAC_IRQ)
		sc->debug.stats.istats.sync_mac_irq++;
	if (sync_cause & AR_INTR_SYNC_EEPROM_ILLEGAL_ACCESS)
		sc->debug.stats.istats.eeprom_illegal_access++;
	if (sync_cause & AR_INTR_SYNC_APB_TIMEOUT)
		sc->debug.stats.istats.apb_timeout++;
	if (sync_cause & AR_INTR_SYNC_PCI_MODE_CONFLICT)
		sc->debug.stats.istats.pci_mode_conflict++;
	if (sync_cause & AR_INTR_SYNC_HOST1_FATAL)
		sc->debug.stats.istats.host1_fatal++;
	if (sync_cause & AR_INTR_SYNC_HOST1_PERR)
		sc->debug.stats.istats.host1_perr++;
	if (sync_cause & AR_INTR_SYNC_TRCV_FIFO_PERR)
		sc->debug.stats.istats.trcv_fifo_perr++;
	if (sync_cause & AR_INTR_SYNC_RADM_CPL_EP)
		sc->debug.stats.istats.radm_cpl_ep++;
	if (sync_cause & AR_INTR_SYNC_RADM_CPL_DLLP_ABORT)
		sc->debug.stats.istats.radm_cpl_dllp_abort++;
	if (sync_cause & AR_INTR_SYNC_RADM_CPL_TLP_ABORT)
		sc->debug.stats.istats.radm_cpl_tlp_abort++;
	if (sync_cause & AR_INTR_SYNC_RADM_CPL_ECRC_ERR)
		sc->debug.stats.istats.radm_cpl_ecrc_err++;
	if (sync_cause & AR_INTR_SYNC_RADM_CPL_TIMEOUT)
		sc->debug.stats.istats.radm_cpl_timeout++;
	if (sync_cause & AR_INTR_SYNC_LOCAL_TIMEOUT)
		sc->debug.stats.istats.local_timeout++;
	if (sync_cause & AR_INTR_SYNC_PM_ACCESS)
		sc->debug.stats.istats.pm_access++;
	if (sync_cause & AR_INTR_SYNC_MAC_AWAKE)
		sc->debug.stats.istats.mac_awake++;
	if (sync_cause & AR_INTR_SYNC_MAC_ASLEEP)
		sc->debug.stats.istats.mac_asleep++;
	if (sync_cause & AR_INTR_SYNC_MAC_SLEEP_ACCESS)
		sc->debug.stats.istats.mac_sleep_access++;
}

static ssize_t ath9k_debugfs_read_buf(struct file *file, char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	u8 *buf = file->private_data;
	return simple_read_from_buffer(user_buf, count, ppos, buf, strlen(buf));
}

static int ath9k_debugfs_release_buf(struct inode *inode, struct file *file)
{
	vfree(file->private_data);
	return 0;
}

#ifdef CONFIG_ATH_DEBUG

static ssize_t read_file_debug(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "0x%08x\n", common->debug_mask);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_debug(struct file *file, const char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	unsigned long mask;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &mask))
		return -EINVAL;

	common->debug_mask = mask;
	return count;
}

static const struct file_operations fops_debug = {
	.read = read_file_debug,
	.write = write_file_debug,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#endif

#define DMA_BUF_LEN 1024


static ssize_t read_file_ani(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct ath_hw *ah = sc->sc_ah;
	unsigned int len = 0;
	const unsigned int size = 1024;
	ssize_t retval = 0;
	char *buf;
	int i;
	struct {
		const char *name;
		unsigned int val;
	} ani_info[] = {
		{ "ANI RESET", ah->stats.ast_ani_reset },
		{ "OFDM LEVEL", ah->ani.ofdmNoiseImmunityLevel },
		{ "CCK LEVEL", ah->ani.cckNoiseImmunityLevel },
		{ "SPUR UP", ah->stats.ast_ani_spurup },
		{ "SPUR DOWN", ah->stats.ast_ani_spurup },
		{ "OFDM WS-DET ON", ah->stats.ast_ani_ofdmon },
		{ "OFDM WS-DET OFF", ah->stats.ast_ani_ofdmoff },
		{ "MRC-CCK ON", ah->stats.ast_ani_ccklow },
		{ "MRC-CCK OFF", ah->stats.ast_ani_cckhigh },
		{ "FIR-STEP UP", ah->stats.ast_ani_stepup },
		{ "FIR-STEP DOWN", ah->stats.ast_ani_stepdown },
		{ "INV LISTENTIME", ah->stats.ast_ani_lneg_or_lzero },
		{ "OFDM ERRORS", ah->stats.ast_ani_ofdmerrs },
		{ "CCK ERRORS", ah->stats.ast_ani_cckerrs },
	};

	buf = kzalloc(size, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	len += scnprintf(buf + len, size - len, "%15s: %s\n", "ANI",
			 common->disable_ani ? "DISABLED" : "ENABLED");

	if (common->disable_ani)
		goto exit;

	for (i = 0; i < ARRAY_SIZE(ani_info); i++)
		len += scnprintf(buf + len, size - len, "%15s: %u\n",
				 ani_info[i].name, ani_info[i].val);

exit:
	if (len > size)
		len = size;

	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;
}

static ssize_t write_file_ani(struct file *file,
			      const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	unsigned long ani;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &ani))
		return -EINVAL;

	if (ani > 1)
		return -EINVAL;

	common->disable_ani = !ani;

	if (common->disable_ani) {
		clear_bit(ATH_OP_ANI_RUN, &common->op_flags);
		ath_stop_ani(sc);
	} else {
		ath_check_ani(sc);
	}

	return count;
}

static const struct file_operations fops_ani = {
	.read = read_file_ani,
	.write = write_file_ani,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#ifdef CONFIG_ATH9K_BTCOEX_SUPPORT

static ssize_t read_file_bt_ant_diversity(struct file *file,
					  char __user *user_buf,
					  size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%d\n", common->bt_ant_diversity);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_bt_ant_diversity(struct file *file,
					   const char __user *user_buf,
					   size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct ath9k_hw_capabilities *pCap = &sc->sc_ah->caps;
	unsigned long bt_ant_diversity;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	if (!(pCap->hw_caps & ATH9K_HW_CAP_BT_ANT_DIV))
		goto exit;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &bt_ant_diversity))
		return -EINVAL;

	common->bt_ant_diversity = !!bt_ant_diversity;
	ath9k_ps_wakeup(sc);
	ath9k_hw_set_bt_ant_diversity(sc->sc_ah, common->bt_ant_diversity);
	ath_dbg(common, CONFIG, "Enable WLAN/BT RX Antenna diversity: %d\n",
		common->bt_ant_diversity);
	ath9k_ps_restore(sc);
exit:
	return count;
}

static const struct file_operations fops_bt_ant_diversity = {
	.read = read_file_bt_ant_diversity,
	.write = write_file_bt_ant_diversity,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#endif

void ath9k_debug_stat_ant(struct ath_softc *sc,
			  struct ath_hw_antcomb_conf *div_ant_conf,
			  int main_rssi_avg, int alt_rssi_avg)
{
	struct ath_antenna_stats *as_main = &sc->debug.stats.ant_stats[ANT_MAIN];
	struct ath_antenna_stats *as_alt = &sc->debug.stats.ant_stats[ANT_ALT];

	as_main->lna_attempt_cnt[div_ant_conf->main_lna_conf]++;
	as_alt->lna_attempt_cnt[div_ant_conf->alt_lna_conf]++;

	as_main->rssi_avg = main_rssi_avg;
	as_alt->rssi_avg = alt_rssi_avg;
}

static ssize_t read_file_antenna_diversity(struct file *file,
					   char __user *user_buf,
					   size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_hw *ah = sc->sc_ah;
	struct ath9k_hw_capabilities *pCap = &ah->caps;
	struct ath_antenna_stats *as_main = &sc->debug.stats.ant_stats[ANT_MAIN];
	struct ath_antenna_stats *as_alt = &sc->debug.stats.ant_stats[ANT_ALT];
	struct ath_hw_antcomb_conf div_ant_conf;
	unsigned int len = 0;
	const unsigned int size = 1024;
	ssize_t retval = 0;
	char *buf;
	static const char *lna_conf_str[4] = {
		"LNA1_MINUS_LNA2", "LNA2", "LNA1", "LNA1_PLUS_LNA2"
	};

	buf = kzalloc(size, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	if (!(pCap->hw_caps & ATH9K_HW_CAP_ANT_DIV_COMB)) {
		len += scnprintf(buf + len, size - len, "%s\n",
				 "Antenna Diversity Combining is disabled");
		goto exit;
	}

	ath9k_ps_wakeup(sc);
	ath9k_hw_antdiv_comb_conf_get(ah, &div_ant_conf);
	len += scnprintf(buf + len, size - len, "Current MAIN config : %s\n",
			 lna_conf_str[div_ant_conf.main_lna_conf]);
	len += scnprintf(buf + len, size - len, "Current ALT config  : %s\n",
			 lna_conf_str[div_ant_conf.alt_lna_conf]);
	len += scnprintf(buf + len, size - len, "Average MAIN RSSI   : %d\n",
			 as_main->rssi_avg);
	len += scnprintf(buf + len, size - len, "Average ALT RSSI    : %d\n\n",
			 as_alt->rssi_avg);
	ath9k_ps_restore(sc);

	len += scnprintf(buf + len, size - len, "Packet Receive Cnt:\n");
	len += scnprintf(buf + len, size - len, "-------------------\n");

	len += scnprintf(buf + len, size - len, "%30s%15s\n",
			 "MAIN", "ALT");
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "TOTAL COUNT",
			 as_main->recv_cnt,
			 as_alt->recv_cnt);
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "LNA1",
			 as_main->lna_recv_cnt[ATH_ANT_DIV_COMB_LNA1],
			 as_alt->lna_recv_cnt[ATH_ANT_DIV_COMB_LNA1]);
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "LNA2",
			 as_main->lna_recv_cnt[ATH_ANT_DIV_COMB_LNA2],
			 as_alt->lna_recv_cnt[ATH_ANT_DIV_COMB_LNA2]);
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "LNA1 + LNA2",
			 as_main->lna_recv_cnt[ATH_ANT_DIV_COMB_LNA1_PLUS_LNA2],
			 as_alt->lna_recv_cnt[ATH_ANT_DIV_COMB_LNA1_PLUS_LNA2]);
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "LNA1 - LNA2",
			 as_main->lna_recv_cnt[ATH_ANT_DIV_COMB_LNA1_MINUS_LNA2],
			 as_alt->lna_recv_cnt[ATH_ANT_DIV_COMB_LNA1_MINUS_LNA2]);

	len += scnprintf(buf + len, size - len, "\nLNA Config Attempts:\n");
	len += scnprintf(buf + len, size - len, "--------------------\n");

	len += scnprintf(buf + len, size - len, "%30s%15s\n",
			 "MAIN", "ALT");
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "LNA1",
			 as_main->lna_attempt_cnt[ATH_ANT_DIV_COMB_LNA1],
			 as_alt->lna_attempt_cnt[ATH_ANT_DIV_COMB_LNA1]);
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "LNA2",
			 as_main->lna_attempt_cnt[ATH_ANT_DIV_COMB_LNA2],
			 as_alt->lna_attempt_cnt[ATH_ANT_DIV_COMB_LNA2]);
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "LNA1 + LNA2",
			 as_main->lna_attempt_cnt[ATH_ANT_DIV_COMB_LNA1_PLUS_LNA2],
			 as_alt->lna_attempt_cnt[ATH_ANT_DIV_COMB_LNA1_PLUS_LNA2]);
	len += scnprintf(buf + len, size - len, "%-14s:%15d%15d\n",
			 "LNA1 - LNA2",
			 as_main->lna_attempt_cnt[ATH_ANT_DIV_COMB_LNA1_MINUS_LNA2],
			 as_alt->lna_attempt_cnt[ATH_ANT_DIV_COMB_LNA1_MINUS_LNA2]);

exit:
	if (len > size)
		len = size;

	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;
}

static const struct file_operations fops_antenna_diversity = {
	.read = read_file_antenna_diversity,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int read_file_dma(struct seq_file *file, void *data)
{
	struct ieee80211_hw *hw = dev_get_drvdata(file->private);
	struct ath_softc *sc = hw->priv;
	struct ath_hw *ah = sc->sc_ah;
	u32 val[ATH9K_NUM_DMA_DEBUG_REGS];
	int i, qcuOffset = 0, dcuOffset = 0;
	u32 *qcuBase = &val[0], *dcuBase = &val[4];

	ath9k_ps_wakeup(sc);

	REG_WRITE_D(ah, AR_MACMISC,
		  ((AR_MACMISC_DMA_OBS_LINE_8 << AR_MACMISC_DMA_OBS_S) |
		   (AR_MACMISC_MISC_OBS_BUS_1 <<
		    AR_MACMISC_MISC_OBS_BUS_MSB_S)));

	seq_puts(file, "Raw DMA Debug values:\n");

	for (i = 0; i < ATH9K_NUM_DMA_DEBUG_REGS; i++) {
		if (i % 4 == 0)
			seq_puts(file, "\n");

		val[i] = REG_READ_D(ah, AR_DMADBG_0 + (i * sizeof(u32)));
		seq_printf(file, "%d: %08x ", i, val[i]);
	}

	seq_puts(file, "\n\n");
	seq_puts(file, "Num QCU: chain_st fsp_ok fsp_st DCU: chain_st\n");

	for (i = 0; i < ATH9K_NUM_QUEUES; i++, qcuOffset += 4, dcuOffset += 5) {
		if (i == 8) {
			qcuOffset = 0;
			qcuBase++;
		}

		if (i == 6) {
			dcuOffset = 0;
			dcuBase++;
		}

		seq_printf(file, "%2d          %2x      %1x     %2x           %2x\n",
			   i, (*qcuBase & (0x7 << qcuOffset)) >> qcuOffset,
			   (*qcuBase & (0x8 << qcuOffset)) >> (qcuOffset + 3),
			   (val[2] & (0x7 << (i * 3))) >> (i * 3),
			   (*dcuBase & (0x1f << dcuOffset)) >> dcuOffset);
	}

	seq_puts(file, "\n");

	seq_printf(file, "qcu_stitch state:   %2x    qcu_fetch state:        %2x\n",
		   (val[3] & 0x003c0000) >> 18, (val[3] & 0x03c00000) >> 22);
	seq_printf(file, "qcu_complete state: %2x    dcu_complete state:     %2x\n",
		   (val[3] & 0x1c000000) >> 26, (val[6] & 0x3));
	seq_printf(file, "dcu_arb state:      %2x    dcu_fp state:           %2x\n",
		   (val[5] & 0x06000000) >> 25, (val[5] & 0x38000000) >> 27);
	seq_printf(file, "chan_idle_dur:     %3d    chan_idle_dur_valid:     %1d\n",
		   (val[6] & 0x000003fc) >> 2, (val[6] & 0x00000400) >> 10);
	seq_printf(file, "txfifo_valid_0:      %1d    txfifo_valid_1:          %1d\n",
		   (val[6] & 0x00000800) >> 11, (val[6] & 0x00001000) >> 12);
	seq_printf(file, "txfifo_dcu_num_0:   %2d    txfifo_dcu_num_1:       %2d\n",
		   (val[6] & 0x0001e000) >> 13, (val[6] & 0x001e0000) >> 17);

	seq_printf(file, "pcu observe: 0x%x\n", REG_READ_D(ah, AR_OBS_BUS_1));
	seq_printf(file, "AR_CR: 0x%x\n", REG_READ_D(ah, AR_CR));

	ath9k_ps_restore(sc);

	return 0;
}

static const struct file_operations fops_dma = {
	.read = read_file_dma,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


void ath_debug_stat_interrupt(struct ath_softc *sc, enum ath9k_int status)
{
	if (status)
		sc->debug.stats.istats.total++;
	if (sc->sc_ah->caps.hw_caps & ATH9K_HW_CAP_EDMA) {
		if (status & ATH9K_INT_RXLP)
			sc->debug.stats.istats.rxlp++;
		if (status & ATH9K_INT_RXHP)
			sc->debug.stats.istats.rxhp++;
		if (status & ATH9K_INT_BB_WATCHDOG)
			sc->debug.stats.istats.bb_watchdog++;
	} else {
		if (status & ATH9K_INT_RX)
			sc->debug.stats.istats.rxok++;
	}
	if (status & ATH9K_INT_RXEOL)
		sc->debug.stats.istats.rxeol++;
	if (status & ATH9K_INT_RXORN)
		sc->debug.stats.istats.rxorn++;
	if (status & ATH9K_INT_TX)
		sc->debug.stats.istats.txok++;
	if (status & ATH9K_INT_TXURN)
		sc->debug.stats.istats.txurn++;
	if (status & ATH9K_INT_RXPHY)
		sc->debug.stats.istats.rxphyerr++;
	if (status & ATH9K_INT_RXKCM)
		sc->debug.stats.istats.rx_keycache_miss++;
	if (status & ATH9K_INT_SWBA)
		sc->debug.stats.istats.swba++;
	if (status & ATH9K_INT_BMISS)
		sc->debug.stats.istats.bmiss++;
	if (status & ATH9K_INT_BNR)
		sc->debug.stats.istats.bnr++;
	if (status & ATH9K_INT_CST)
		sc->debug.stats.istats.cst++;
	if (status & ATH9K_INT_GTT)
		sc->debug.stats.istats.gtt++;
	if (status & ATH9K_INT_TIM)
		sc->debug.stats.istats.tim++;
	if (status & ATH9K_INT_CABEND)
		sc->debug.stats.istats.cabend++;
	if (status & ATH9K_INT_DTIMSYNC)
		sc->debug.stats.istats.dtimsync++;
	if (status & ATH9K_INT_DTIM)
		sc->debug.stats.istats.dtim++;
	if (status & ATH9K_INT_TSFOOR)
		sc->debug.stats.istats.tsfoor++;
	if (status & ATH9K_INT_MCI)
		sc->debug.stats.istats.mci++;
	if (status & ATH9K_INT_GENTIMER)
		sc->debug.stats.istats.gen_timer++;
}

static int read_file_interrupt(struct seq_file *file, void *data)
{
	struct ieee80211_hw *hw = dev_get_drvdata(file->private);
	struct ath_softc *sc = hw->priv;

#define PR_IS(a, s)						\
	do {							\
		seq_printf(file, "%21s: %10u\n", a,		\
			   sc->debug.stats.istats.s);		\
	} while (0)

	if (sc->sc_ah->caps.hw_caps & ATH9K_HW_CAP_EDMA) {
		PR_IS("RXLP", rxlp);
		PR_IS("RXHP", rxhp);
		PR_IS("WATHDOG", bb_watchdog);
	} else {
		PR_IS("RX", rxok);
	}
	PR_IS("RXEOL", rxeol);
	PR_IS("RXORN", rxorn);
	PR_IS("TX", txok);
	PR_IS("TXURN", txurn);
	PR_IS("MIB", mib);
	PR_IS("RXPHY", rxphyerr);
	PR_IS("RXKCM", rx_keycache_miss);
	PR_IS("SWBA", swba);
	PR_IS("BMISS", bmiss);
	PR_IS("BNR", bnr);
	PR_IS("CST", cst);
	PR_IS("GTT", gtt);
	PR_IS("TIM", tim);
	PR_IS("CABEND", cabend);
	PR_IS("DTIMSYNC", dtimsync);
	PR_IS("DTIM", dtim);
	PR_IS("TSFOOR", tsfoor);
	PR_IS("MCI", mci);
	PR_IS("GENTIMER", gen_timer);
	PR_IS("TOTAL", total);

	seq_puts(file, "SYNC_CAUSE stats:\n");

	PR_IS("Sync-All", sync_cause_all);
	PR_IS("RTC-IRQ", sync_rtc_irq);
	PR_IS("MAC-IRQ", sync_mac_irq);
	PR_IS("EEPROM-Illegal-Access", eeprom_illegal_access);
	PR_IS("APB-Timeout", apb_timeout);
	PR_IS("PCI-Mode-Conflict", pci_mode_conflict);
	PR_IS("HOST1-Fatal", host1_fatal);
	PR_IS("HOST1-Perr", host1_perr);
	PR_IS("TRCV-FIFO-Perr", trcv_fifo_perr);
	PR_IS("RADM-CPL-EP", radm_cpl_ep);
	PR_IS("RADM-CPL-DLLP-Abort", radm_cpl_dllp_abort);
	PR_IS("RADM-CPL-TLP-Abort", radm_cpl_tlp_abort);
	PR_IS("RADM-CPL-ECRC-Err", radm_cpl_ecrc_err);
	PR_IS("RADM-CPL-Timeout", radm_cpl_timeout);
	PR_IS("Local-Bus-Timeout", local_timeout);
	PR_IS("PM-Access", pm_access);
	PR_IS("MAC-Awake", mac_awake);
	PR_IS("MAC-Asleep", mac_asleep);
	PR_IS("MAC-Sleep-Access", mac_sleep_access);

	return 0;
}

static const struct file_operations fops_interrupt = {
	.read = read_file_interrupt,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int read_file_xmit(struct seq_file *file, void *data)
{
	struct ieee80211_hw *hw = dev_get_drvdata(file->private);
	struct ath_softc *sc = hw->priv;

	seq_printf(file, "%30s %10s%10s%10s\n\n", "BE", "BK", "VI", "VO");

	PR("MPDUs Queued:    ", queued);
	PR("MPDUs Completed: ", completed);
	PR("MPDUs XRetried:  ", xretries);
	PR("Aggregates:      ", a_aggr);
	PR("AMPDUs Queued HW:", a_queued_hw);
	PR("AMPDUs Queued SW:", a_queued_sw);
	PR("AMPDUs Completed:", a_completed);
	PR("AMPDUs Retried:  ", a_retries);
	PR("AMPDUs XRetried: ", a_xretries);
	PR("TXERR Filtered:  ", txerr_filtered);
	PR("FIFO Underrun:   ", fifo_underrun);
	PR("TXOP Exceeded:   ", xtxop);
	PR("TXTIMER Expiry:  ", timer_exp);
	PR("DESC CFG Error:  ", desc_cfg_err);
	PR("DATA Underrun:   ", data_underrun);
	PR("DELIM Underrun:  ", delim_underrun);
	PR("TX-Pkts-All:     ", tx_pkts_all);
	PR("TX-Bytes-All:    ", tx_bytes_all);
	PR("HW-put-tx-buf:   ", puttxbuf);
	PR("HW-tx-start:     ", txstart);
	PR("HW-tx-proc-desc: ", txprocdesc);
	PR("TX-Failed:       ", txfailed);

	return 0;
}

static void print_queue(struct ath_softc *sc, struct ath_txq *txq,
			struct seq_file *file)
{
	ath_txq_lock(sc, txq);

	seq_printf(file, "%s: %d ", "qnum", txq->axq_qnum);
	seq_printf(file, "%s: %2d ", "qdepth", txq->axq_depth);
	seq_printf(file, "%s: %2d ", "ampdu-depth", txq->axq_ampdu_depth);
	seq_printf(file, "%s: %3d ", "pending", txq->pending_frames);
	seq_printf(file, "%s: %d\n", "stopped", txq->stopped);

	ath_txq_unlock(sc, txq);
}

static int read_file_queues(struct seq_file *file, void *data)
{
	struct ieee80211_hw *hw = dev_get_drvdata(file->private);
	struct ath_softc *sc = hw->priv;
	struct ath_txq *txq;
	int i;
	static const char *qname[4] = {
		"VO", "VI", "BE", "BK"
	};

	for (i = 0; i < IEEE80211_NUM_ACS; i++) {
		txq = sc->tx.txq_map[i];
		seq_printf(file, "(%s):  ", qname[i]);
		print_queue(sc, txq, file);
	}

	seq_puts(file, "(CAB): ");
	print_queue(sc, sc->beacon.cabq, file);

	return 0;
}

static int read_file_misc(struct seq_file *file, void *data)
{
	struct ieee80211_hw *hw = dev_get_drvdata(file->private);
	struct ath_softc *sc = hw->priv;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	struct ath9k_vif_iter_data iter_data;
	struct ath_chanctx *ctx;
	unsigned int reg;
	u32 rxfilter, i;

	seq_printf(file, "BSSID: %pM\n", common->curbssid);
	seq_printf(file, "BSSID-MASK: %pM\n", common->bssidmask);
	seq_printf(file, "OPMODE: %s\n",
		   ath_opmode_to_string(sc->sc_ah->opmode));

	ath9k_ps_wakeup(sc);
	rxfilter = ath9k_hw_getrxfilter(sc->sc_ah);
	ath9k_ps_restore(sc);

	seq_printf(file, "RXFILTER: 0x%x", rxfilter);

	if (rxfilter & ATH9K_RX_FILTER_UCAST)
		seq_puts(file, " UCAST");
	if (rxfilter & ATH9K_RX_FILTER_MCAST)
		seq_puts(file, " MCAST");
	if (rxfilter & ATH9K_RX_FILTER_BCAST)
		seq_puts(file, " BCAST");
	if (rxfilter & ATH9K_RX_FILTER_CONTROL)
		seq_puts(file, " CONTROL");
	if (rxfilter & ATH9K_RX_FILTER_BEACON)
		seq_puts(file, " BEACON");
	if (rxfilter & ATH9K_RX_FILTER_PROM)
		seq_puts(file, " PROM");
	if (rxfilter & ATH9K_RX_FILTER_PROBEREQ)
		seq_puts(file, " PROBEREQ");
	if (rxfilter & ATH9K_RX_FILTER_PHYERR)
		seq_puts(file, " PHYERR");
	if (rxfilter & ATH9K_RX_FILTER_MYBEACON)
		seq_puts(file, " MYBEACON");
	if (rxfilter & ATH9K_RX_FILTER_COMP_BAR)
		seq_puts(file, " COMP_BAR");
	if (rxfilter & ATH9K_RX_FILTER_PSPOLL)
		seq_puts(file, " PSPOLL");
	if (rxfilter & ATH9K_RX_FILTER_PHYRADAR)
		seq_puts(file, " PHYRADAR");
	if (rxfilter & ATH9K_RX_FILTER_MCAST_BCAST_ALL)
		seq_puts(file, " MCAST_BCAST_ALL");
	if (rxfilter & ATH9K_RX_FILTER_CONTROL_WRAPPER)
		seq_puts(file, " CONTROL_WRAPPER");

	seq_puts(file, "\n");

	reg = sc->sc_ah->imask;

	seq_printf(file, "INTERRUPT-MASK: 0x%x", reg);

	if (reg & ATH9K_INT_SWBA)
		seq_puts(file, " SWBA");
	if (reg & ATH9K_INT_BMISS)
		seq_puts(file, " BMISS");
	if (reg & ATH9K_INT_CST)
		seq_puts(file, " CST");
	if (reg & ATH9K_INT_RX)
		seq_puts(file, " RX");
	if (reg & ATH9K_INT_RXHP)
		seq_puts(file, " RXHP");
	if (reg & ATH9K_INT_RXLP)
		seq_puts(file, " RXLP");
	if (reg & ATH9K_INT_BB_WATCHDOG)
		seq_puts(file, " BB_WATCHDOG");

	seq_puts(file, "\n");

	i = 0;
	ath_for_each_chanctx(sc, ctx) {
		if (list_empty(&ctx->vifs))
			continue;
		ath9k_calculate_iter_data(sc, ctx, &iter_data);

		seq_printf(file,
			   "VIFS: CTX %i(%i) AP: %i STA: %i MESH: %i WDS: %i",
			   i++, (int)(ctx->assigned), iter_data.naps,
			   iter_data.nstations,
			   iter_data.nmeshes, iter_data.nwds);
		seq_printf(file, " ADHOC: %i TOTAL: %hi BEACON-VIF: %hi\n",
			   iter_data.nadhocs, sc->cur_chan->nvifs,
			   sc->nbcnvifs);
	}

	return 0;
}

static int read_file_reset(struct seq_file *file, void *data)
{
	struct ieee80211_hw *hw = dev_get_drvdata(file->private);
	struct ath_softc *sc = hw->priv;
	static const char * const reset_cause[__RESET_TYPE_MAX] = {
		[RESET_TYPE_BB_HANG] = "Baseband Hang",
		[RESET_TYPE_BB_WATCHDOG] = "Baseband Watchdog",
		[RESET_TYPE_FATAL_INT] = "Fatal HW Error",
		[RESET_TYPE_TX_ERROR] = "TX HW error",
		[RESET_TYPE_TX_GTT] = "Transmit timeout",
		[RESET_TYPE_TX_HANG] = "TX Path Hang",
		[RESET_TYPE_PLL_HANG] = "PLL RX Hang",
		[RESET_TYPE_MAC_HANG] = "MAC Hang",
		[RESET_TYPE_BEACON_STUCK] = "Stuck Beacon",
		[RESET_TYPE_MCI] = "MCI Reset",
		[RESET_TYPE_CALIBRATION] = "Calibration error",
	};
	int i;

	for (i = 0; i < ARRAY_SIZE(reset_cause); i++) {
		if (!reset_cause[i])
		    continue;

		seq_printf(file, "%17s: %2d\n", reset_cause[i],
			   sc->debug.stats.reset[i]);
	}

	return 0;
}

void ath_debug_stat_tx(struct ath_softc *sc, struct ath_buf *bf,
		       struct ath_tx_status *ts, struct ath_txq *txq,
		       unsigned int flags)
{
	int qnum = txq->axq_qnum;

	TX_STAT_INC(qnum, tx_pkts_all);
	sc->debug.stats.txstats[qnum].tx_bytes_all += bf->bf_mpdu->len;

	if (bf_isampdu(bf)) {
		if (flags & ATH_TX_ERROR)
			TX_STAT_INC(qnum, a_xretries);
		else
			TX_STAT_INC(qnum, a_completed);
	} else {
		if (ts->ts_status & ATH9K_TXERR_XRETRY)
			TX_STAT_INC(qnum, xretries);
		else
			TX_STAT_INC(qnum, completed);
	}

	if (ts->ts_status & ATH9K_TXERR_FILT)
		TX_STAT_INC(qnum, txerr_filtered);
	if (ts->ts_status & ATH9K_TXERR_FIFO)
		TX_STAT_INC(qnum, fifo_underrun);
	if (ts->ts_status & ATH9K_TXERR_XTXOP)
		TX_STAT_INC(qnum, xtxop);
	if (ts->ts_status & ATH9K_TXERR_TIMER_EXPIRED)
		TX_STAT_INC(qnum, timer_exp);
	if (ts->ts_flags & ATH9K_TX_DESC_CFG_ERR)
		TX_STAT_INC(qnum, desc_cfg_err);
	if (ts->ts_flags & ATH9K_TX_DATA_UNDERRUN)
		TX_STAT_INC(qnum, data_underrun);
	if (ts->ts_flags & ATH9K_TX_DELIM_UNDERRUN)
		TX_STAT_INC(qnum, delim_underrun);
}

static const struct file_operations fops_xmit = {
	.read = read_file_xmit,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_queues = {
	.read = read_file_queues,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_misc = {
	.read = read_file_misc,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_reset = {
	.read = read_file_reset,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_recv(struct file *file, char __user *user_buf,
			      size_t count, loff_t *ppos)
{
#define PHY_ERR(s, p) \
	len += scnprintf(buf + len, size - len, "%22s : %10u\n", s, \
			 sc->debug.stats.rxstats.phy_err_stats[p]);

#define RXS_ERR(s, e)					    \
	do {						    \
		len += scnprintf(buf + len, size - len,	    \
				 "%22s : %10u\n", s,	    \
				 sc->debug.stats.rxstats.e);\
	} while (0)

	struct ath_softc *sc = file->private_data;
	char *buf;
	unsigned int len = 0, size = 1600;
	ssize_t retval = 0;

	buf = kzalloc(size, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	RXS_ERR("CRC ERR", crc_err);
	RXS_ERR("DECRYPT CRC ERR", decrypt_crc_err);
	RXS_ERR("PHY ERR", phy_err);
	RXS_ERR("MIC ERR", mic_err);
	RXS_ERR("PRE-DELIM CRC ERR", pre_delim_crc_err);
	RXS_ERR("POST-DELIM CRC ERR", post_delim_crc_err);
	RXS_ERR("DECRYPT BUSY ERR", decrypt_busy_err);
	RXS_ERR("RX-LENGTH-ERR", rx_len_err);
	RXS_ERR("RX-OOM-ERR", rx_oom_err);
	RXS_ERR("RX-RATE-ERR", rx_rate_err);
	RXS_ERR("RX-TOO-MANY-FRAGS", rx_too_many_frags_err);

	PHY_ERR("UNDERRUN ERR", ATH9K_PHYERR_UNDERRUN);
	PHY_ERR("TIMING ERR", ATH9K_PHYERR_TIMING);
	PHY_ERR("PARITY ERR", ATH9K_PHYERR_PARITY);
	PHY_ERR("RATE ERR", ATH9K_PHYERR_RATE);
	PHY_ERR("LENGTH ERR", ATH9K_PHYERR_LENGTH);
	PHY_ERR("RADAR ERR", ATH9K_PHYERR_RADAR);
	PHY_ERR("SERVICE ERR", ATH9K_PHYERR_SERVICE);
	PHY_ERR("TOR ERR", ATH9K_PHYERR_TOR);
	PHY_ERR("OFDM-TIMING ERR", ATH9K_PHYERR_OFDM_TIMING);
	PHY_ERR("OFDM-SIGNAL-PARITY ERR", ATH9K_PHYERR_OFDM_SIGNAL_PARITY);
	PHY_ERR("OFDM-RATE ERR", ATH9K_PHYERR_OFDM_RATE_ILLEGAL);
	PHY_ERR("OFDM-LENGTH ERR", ATH9K_PHYERR_OFDM_LENGTH_ILLEGAL);
	PHY_ERR("OFDM-POWER-DROP ERR", ATH9K_PHYERR_OFDM_POWER_DROP);
	PHY_ERR("OFDM-SERVICE ERR", ATH9K_PHYERR_OFDM_SERVICE);
	PHY_ERR("OFDM-RESTART ERR", ATH9K_PHYERR_OFDM_RESTART);
	PHY_ERR("FALSE-RADAR-EXT ERR", ATH9K_PHYERR_FALSE_RADAR_EXT);
	PHY_ERR("CCK-TIMING ERR", ATH9K_PHYERR_CCK_TIMING);
	PHY_ERR("CCK-HEADER-CRC ERR", ATH9K_PHYERR_CCK_HEADER_CRC);
	PHY_ERR("CCK-RATE ERR", ATH9K_PHYERR_CCK_RATE_ILLEGAL);
	PHY_ERR("CCK-SERVICE ERR", ATH9K_PHYERR_CCK_SERVICE);
	PHY_ERR("CCK-RESTART ERR", ATH9K_PHYERR_CCK_RESTART);
	PHY_ERR("CCK-LENGTH ERR", ATH9K_PHYERR_CCK_LENGTH_ILLEGAL);
	PHY_ERR("CCK-POWER-DROP ERR", ATH9K_PHYERR_CCK_POWER_DROP);
	PHY_ERR("HT-CRC ERR", ATH9K_PHYERR_HT_CRC_ERROR);
	PHY_ERR("HT-LENGTH ERR", ATH9K_PHYERR_HT_LENGTH_ILLEGAL);
	PHY_ERR("HT-RATE ERR", ATH9K_PHYERR_HT_RATE_ILLEGAL);

	RXS_ERR("RX-Pkts-All", rx_pkts_all);
	RXS_ERR("RX-Bytes-All", rx_bytes_all);
	RXS_ERR("RX-Beacons", rx_beacons);
	RXS_ERR("RX-Frags", rx_frags);
	RXS_ERR("RX-Spectral", rx_spectral);

	if (len > size)
		len = size;

	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;

#undef RXS_ERR
#undef PHY_ERR
}

void ath_debug_stat_rx(struct ath_softc *sc, struct ath_rx_status *rs)
{
	ath9k_cmn_debug_stat_rx(&sc->debug.stats.rxstats, rs);
	
}
static const struct file_operations fops_recv = {
	.read = read_file_recv,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_spec_scan_ctl(struct file *file, char __user *user_buf,
				       size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	char *mode = "";
	unsigned int len;

	switch (sc->spectral_mode) {
	case SPECTRAL_DISABLED:
		mode = "disable";
		break;
	case SPECTRAL_BACKGROUND:
		mode = "background";
		break;
	case SPECTRAL_CHANSCAN:
		mode = "chanscan";
		break;
	case SPECTRAL_MANUAL:
		mode = "manual";
		break;
	}
	len = strlen(mode);
	return simple_read_from_buffer(user_buf, count, ppos, mode, len);
}

static ssize_t write_file_spec_scan_ctl(struct file *file,
					const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	char buf[32];
	ssize_t len;

	if (config_enabled(CPTCFG_ATH9K_TX99))
		return -EOPNOTSUPP;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';

	if (strncmp("trigger", buf, 7) == 0) {
		ath9k_spectral_scan_trigger(sc->hw);
	} else if (strncmp("background", buf, 9) == 0) {
		ath9k_spectral_scan_config(sc->hw, SPECTRAL_BACKGROUND);
		ath_dbg(common, CONFIG, "spectral scan: background mode enabled\n");
	} else if (strncmp("chanscan", buf, 8) == 0) {
		ath9k_spectral_scan_config(sc->hw, SPECTRAL_CHANSCAN);
		ath_dbg(common, CONFIG, "spectral scan: channel scan mode enabled\n");
	} else if (strncmp("manual", buf, 6) == 0) {
		ath9k_spectral_scan_config(sc->hw, SPECTRAL_MANUAL);
		ath_dbg(common, CONFIG, "spectral scan: manual mode enabled\n");
	} else if (strncmp("disable", buf, 7) == 0) {
		ath9k_spectral_scan_config(sc->hw, SPECTRAL_DISABLED);
		ath_dbg(common, CONFIG, "spectral scan: disabled\n");
	} else {
		return -EINVAL;
	}

	return count;
}

static const struct file_operations fops_spec_scan_ctl = {
	.read = read_file_spec_scan_ctl,
	.write = write_file_spec_scan_ctl,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_spectral_short_repeat(struct file *file,
					       char __user *user_buf,
					       size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%d\n", sc->spec_config.short_repeat);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_spectral_short_repeat(struct file *file,
						const char __user *user_buf,
						size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	unsigned long val;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0 || val > 1)
		return -EINVAL;

	sc->spec_config.short_repeat = val;
	return count;
}

static const struct file_operations fops_spectral_short_repeat = {
	.read = read_file_spectral_short_repeat,
	.write = write_file_spectral_short_repeat,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_spectral_count(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%d\n", sc->spec_config.count);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_spectral_count(struct file *file,
					 const char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	unsigned long val;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0 || val > 255)
		return -EINVAL;

	sc->spec_config.count = val;
	return count;
}

static const struct file_operations fops_spectral_count = {
	.read = read_file_spectral_count,
	.write = write_file_spectral_count,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_spectral_period(struct file *file,
					 char __user *user_buf,
					 size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%d\n", sc->spec_config.period);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_spectral_period(struct file *file,
					  const char __user *user_buf,
					  size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	unsigned long val;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0 || val > 255)
		return -EINVAL;

	sc->spec_config.period = val;
	return count;
}

static const struct file_operations fops_spectral_period = {
	.read = read_file_spectral_period,
	.write = write_file_spectral_period,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_spectral_fft_period(struct file *file,
					     char __user *user_buf,
					     size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%d\n", sc->spec_config.fft_period);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_spectral_fft_period(struct file *file,
					      const char __user *user_buf,
					      size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	unsigned long val;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0 || val > 15)
		return -EINVAL;

	sc->spec_config.fft_period = val;
	return count;
}

static const struct file_operations fops_spectral_fft_period = {
	.read = read_file_spectral_fft_period,
	.write = write_file_spectral_fft_period,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static struct dentry *create_buf_file_handler(const char *filename,
					      struct dentry *parent,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,3,0))
					      umode_t mode,
#else
					      int mode,
#endif
					      struct rchan_buf *buf,
					      int *is_global)
{
	struct dentry *buf_file;

	buf_file = debugfs_create_file(filename, mode, parent, buf,
				       &relay_file_operations);
	*is_global = 1;
	return buf_file;
}

static int remove_buf_file_handler(struct dentry *dentry)
{
	debugfs_remove(dentry);

	return 0;
}

void ath_debug_send_fft_sample(struct ath_softc *sc,
			       struct fft_sample_tlv *fft_sample_tlv)
{
	int length;
	if (!sc->rfs_chan_spec_scan)
		return;

	length = __be16_to_cpu(fft_sample_tlv->length) +
		 sizeof(*fft_sample_tlv);
	relay_write(sc->rfs_chan_spec_scan, fft_sample_tlv, length);
}

static struct rchan_callbacks rfs_spec_scan_cb = {
	.create_buf_file = create_buf_file_handler,
	.remove_buf_file = remove_buf_file_handler,
};


static ssize_t read_file_regidx(struct file *file, char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "0x%08x\n", sc->debug.regidx);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_regidx(struct file *file, const char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	unsigned long regidx;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &regidx))
		return -EINVAL;

	sc->debug.regidx = regidx;
	return count;
}

static const struct file_operations fops_regidx = {
	.read = read_file_regidx,
	.write = write_file_regidx,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_regval(struct file *file, char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_hw *ah = sc->sc_ah;
	char buf[32];
	unsigned int len;
	u32 regval;

	ath9k_ps_wakeup(sc);
	regval = REG_READ_D(ah, sc->debug.regidx);
	ath9k_ps_restore(sc);
	len = sprintf(buf, "0x%08x\n", regval);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_regval(struct file *file, const char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_hw *ah = sc->sc_ah;
	unsigned long regval;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &regval))
		return -EINVAL;

	ath9k_ps_wakeup(sc);
	REG_WRITE_D(ah, sc->debug.regidx, regval);
	ath9k_ps_restore(sc);
	return count;
}

static const struct file_operations fops_regval = {
	.read = read_file_regval,
	.write = write_file_regval,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


static ssize_t read_file_tx_chainmask(struct file *file,
		  char __user *user_buf,
		  size_t count, loff_t *ppos)
{
	struct ath_hw *ah = file->private_data;
	char buf[8];
	u8 len;

	len = sprintf(buf, "%u\n", ah->txchainmask);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_tx_chainmask(struct file *file,
		   const char __user *user_buf,
		   size_t count, loff_t *ppos)
{
	struct ath_hw *ah = file->private_data;
	u8 tx_chainmask;
	char buf[8];
	u8 len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou8(buf, 0, &tx_chainmask))
		return -EINVAL;
	ah->txchainmask = tx_chainmask;
	ar9003_hw_set_chain_masks(ah,ah->rxchainmask,ah->txchainmask);
	return count;
}

static const struct file_operations fops_tx_chainmask = {
	.read = read_file_tx_chainmask,
	.write = write_file_tx_chainmask,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_rx_chainmask(struct file *file,
		  char __user *user_buf,
		  size_t count, loff_t *ppos)
{
	struct ath_hw *ah = file->private_data;
	char buf[8];
	u8 len;

	len = sprintf(buf, "%u\n", ah->rxchainmask);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_rx_chainmask(struct file *file,
		   const char __user *user_buf,
		   size_t count, loff_t *ppos)
{
	struct ath_hw *ah = file->private_data;
	u8 rx_chainmask;
	char buf[8];
	u8 len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou8(buf, 0, &rx_chainmask))
		return -EINVAL;
	ah->rxchainmask = rx_chainmask;
	ar9003_hw_set_chain_masks(ah,ah->rxchainmask,ah->txchainmask);
	return count;
}

static const struct file_operations fops_rx_chainmask = {
	.read = read_file_rx_chainmask,
	.write = write_file_rx_chainmask,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#define REGDUMP_LINE_SIZE	20

static int open_file_regdump(struct inode *inode, struct file *file)
{
	struct ath_softc *sc = inode->i_private;
	unsigned int len = 0;
	u8 *buf;
	int i;
	unsigned long num_regs, regdump_len, max_reg_offset;

	max_reg_offset = AR_SREV_9300_20_OR_LATER(sc->sc_ah) ? 0x16bd4 : 0xb500;
	num_regs = max_reg_offset / 4 + 1;
	regdump_len = num_regs * REGDUMP_LINE_SIZE + 1;
	buf = vmalloc(regdump_len);
	if (!buf)
		return -ENOMEM;

	ath9k_ps_wakeup(sc);
	for (i = 0; i < num_regs; i++)
		len += scnprintf(buf + len, regdump_len - len,
			"0x%06x 0x%08x\n", i << 2, REG_READ(sc->sc_ah, i << 2));
	ath9k_ps_restore(sc);

	file->private_data = buf;

	return 0;
}

static const struct file_operations fops_regdump = {
	.open = open_file_regdump,
	.read = ath9k_debugfs_read_buf,
	.release = ath9k_debugfs_release_buf,
	.owner = THIS_MODULE,
	.llseek = default_llseek,/* read accesses f_pos */
};

static int read_file_dump_nfcal(struct seq_file *file, void *data)
{
	struct ieee80211_hw *hw = dev_get_drvdata(file->private);
	struct ath_softc *sc = hw->priv;
	struct ath_hw *ah = sc->sc_ah;
	struct ath9k_nfcal_hist *h = sc->cur_chan->caldata.nfCalHist;
	struct ath_common *common = ath9k_hw_common(ah);
	struct ieee80211_conf *conf = &common->hw->conf;
	u32 i, j;
	u8 chainmask = (ah->rxchainmask << 3) | ah->rxchainmask;
	u8 nread;

	seq_printf(file, "Channel Noise Floor : %d\n", ah->noise);
	seq_puts(file, "Chain | privNF | # Readings | NF Readings\n");
	for (i = 0; i < NUM_NF_READINGS; i++) {
		if (!(chainmask & (1 << i)) ||
		    ((i >= AR5416_MAX_CHAINS) && !conf_is_ht40(conf)))
			continue;

		nread = AR_PHY_CCA_FILTERWINDOW_LENGTH - h[i].invalidNFcount;
		seq_printf(file, " %d\t %d\t %d\t\t", i, h[i].privNF, nread);
		for (j = 0; j < nread; j++)
			seq_printf(file, " %d", h[i].nfCalBuffer[j]);
		seq_puts(file, "\n");
	}

	return 0;
}

static int open_file_dump_nfcal(struct inode *inode, struct file *f)
{
	return single_open(f, read_file_dump_nfcal, inode->i_private);
}

static const struct file_operations fops_dump_nfcal = {
	.read = seq_read,
	.open = open_file_dump_nfcal,
	.owner = THIS_MODULE,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t read_file_base_eeprom(struct file *file, char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_hw *ah = sc->sc_ah;
	u32 len = 0, size = 1500;
	ssize_t retval = 0;
	char *buf;

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len = ah->eep_ops->dump_eeprom(ah, true, buf, len, size);

	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;
}

static const struct file_operations fops_base_eeprom = {
	.read = read_file_base_eeprom,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_modal_eeprom(struct file *file, char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_hw *ah = sc->sc_ah;
	u32 len = 0, size = 6000;
	char *buf;
	size_t retval;

	buf = kzalloc(size, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	len = ah->eep_ops->dump_eeprom(ah, false, buf, len, size);

	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;
}

static const struct file_operations fops_modal_eeprom = {
	.read = read_file_modal_eeprom,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#ifdef CONFIG_ATH9K_BTCOEX_SUPPORT
static ssize_t read_file_btcoex(struct file *file, char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	u32 len = 0, size = 1500;
	char *buf;
	size_t retval;

	buf = kzalloc(size, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	if (!sc->sc_ah->common.btcoex_enabled) {
		len = scnprintf(buf, size, "%s\n",
				"BTCOEX is disabled");
		goto exit;
	}

	len = ath9k_dump_btcoex(sc, buf, size);
exit:
	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;
}

static const struct file_operations fops_btcoex = {
	.read = read_file_btcoex,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};
#endif

static ssize_t read_file_node_stat(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	struct ath_node *an = file->private_data;
	struct ath_softc *sc = an->sc;
	struct ath_atx_tid *tid;
	struct ath_atx_ac *ac;
	struct ath_txq *txq;
	u32 len = 0, size = 4096;
	char *buf;
	size_t retval;
	int tidno, acno;

	buf = kzalloc(size, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	if (!an->sta->ht_cap.ht_supported) {
		len = scnprintf(buf, size, "%s\n",
				"HT not supported");
		goto exit;
	}

	len = scnprintf(buf, size, "Max-AMPDU: %d\n",
			an->maxampdu);
	len += scnprintf(buf + len, size - len, "MPDU Density: %d\n\n",
			 an->mpdudensity);

	len += scnprintf(buf + len, size - len,
			 "%2s%7s\n", "AC", "SCHED");

	for (acno = 0, ac = &an->ac[acno];
	     acno < IEEE80211_NUM_ACS; acno++, ac++) {
		txq = ac->txq;
		ath_txq_lock(sc, txq);
		len += scnprintf(buf + len, size - len,
				 "%2d%7d\n",
				 acno, ac->sched);
		ath_txq_unlock(sc, txq);
	}

	len += scnprintf(buf + len, size - len,
			 "\n%3s%11s%10s%10s%10s%10s%9s%6s%8s\n",
			 "TID", "SEQ_START", "SEQ_NEXT", "BAW_SIZE",
			 "BAW_HEAD", "BAW_TAIL", "BAR_IDX", "SCHED", "PAUSED");

	for (tidno = 0, tid = &an->tid[tidno];
	     tidno < IEEE80211_NUM_TIDS; tidno++, tid++) {
		txq = tid->ac->txq;
		ath_txq_lock(sc, txq);
		len += scnprintf(buf + len, size - len,
				 "%3d%11d%10d%10d%10d%10d%9d%6d%8d\n",
				 tid->tidno, tid->seq_start, tid->seq_next,
				 tid->baw_size, tid->baw_head, tid->baw_tail,
				 tid->bar_index, tid->sched, tid->paused);
		ath_txq_unlock(sc, txq);
	}
exit:
	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;
}

static const struct file_operations fops_node_stat = {
	.read = read_file_node_stat,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

void ath9k_sta_add_debugfs(struct ieee80211_hw *hw,
			   struct ieee80211_vif *vif,
			   struct ieee80211_sta *sta,
			   struct dentry *dir)
{
	struct ath_node *an = (struct ath_node *)sta->drv_priv;
	debugfs_create_file("node_stat", S_IRUGO, dir, an, &fops_node_stat);
}

#ifdef CONFIG_ATH9K_DYNACK
static ssize_t read_file_ackto(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_hw *ah = sc->sc_ah;
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%u %c\n", ah->dynack.ackto,
		      (ah->dynack.enabled) ? 'A' : 'S');

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_ackto = {
	.read = read_file_ackto,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};
#endif

#ifdef CONFIG_ATH9K_WOW

static ssize_t read_file_wow(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	unsigned int len = 0, size = 32;
	ssize_t retval;
	char *buf;

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, size - len, "WOW: %s\n",
			 sc->force_wow ? "ENABLED" : "DISABLED");

	if (len > size)
		len = size;

	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;
}

static ssize_t write_file_wow(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	unsigned long val;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val != 1)
		return -EINVAL;

	if (!sc->force_wow) {
		sc->force_wow = true;
		ath9k_init_wow(sc->hw);
	}

	return count;
}

static const struct file_operations fops_wow = {
	.read = read_file_wow,
	.write = write_file_wow,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#endif

static ssize_t read_file_tpc(struct file *file, char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_hw *ah = sc->sc_ah;
	unsigned int len = 0, size = 32;
	ssize_t retval;
	char *buf;

	buf = kzalloc(size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, size - len, "%s\n",
			 ah->tpc_enabled ? "ENABLED" : "DISABLED");

	if (len > size)
		len = size;

	retval = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return retval;
}

static ssize_t write_file_tpc(struct file *file, const char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_hw *ah = sc->sc_ah;
	unsigned long val;
	char buf[32];
	ssize_t len;
	bool tpc_enabled;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &val))
		return -EINVAL;

	if (val < 0 || val > 1)
		return -EINVAL;

	tpc_enabled = !!val;

	if (tpc_enabled != ah->tpc_enabled) {
		ah->tpc_enabled = tpc_enabled;

		mutex_lock(&sc->mutex);
		ath9k_set_txpower(sc, NULL);
		mutex_unlock(&sc->mutex);
	}

	return count;
}

static const struct file_operations fops_tpc = {
	.read = read_file_tpc,
	.write = write_file_tpc,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* Ethtool support for get-stats */

#define AMKSTR(nm) #nm "_BE", #nm "_BK", #nm "_VI", #nm "_VO"
static const char ath9k_gstrings_stats[][ETH_GSTRING_LEN] = {
	"tx_pkts_nic",
	"tx_bytes_nic",
	"rx_pkts_nic",
	"rx_bytes_nic",
	AMKSTR(d_tx_pkts),
	AMKSTR(d_tx_bytes),
	AMKSTR(d_tx_mpdus_queued),
	AMKSTR(d_tx_mpdus_completed),
	AMKSTR(d_tx_mpdu_xretries),
	AMKSTR(d_tx_aggregates),
	AMKSTR(d_tx_ampdus_queued_hw),
	AMKSTR(d_tx_ampdus_queued_sw),
	AMKSTR(d_tx_ampdus_completed),
	AMKSTR(d_tx_ampdu_retries),
	AMKSTR(d_tx_ampdu_xretries),
	AMKSTR(d_tx_fifo_underrun),
	AMKSTR(d_tx_op_exceeded),
	AMKSTR(d_tx_timer_expiry),
	AMKSTR(d_tx_desc_cfg_err),
	AMKSTR(d_tx_data_underrun),
	AMKSTR(d_tx_delim_underrun),
	"d_rx_crc_err",
	"d_rx_decrypt_crc_err",
	"d_rx_phy_err",
	"d_rx_mic_err",
	"d_rx_pre_delim_crc_err",
	"d_rx_post_delim_crc_err",
	"d_rx_decrypt_busy_err",

	"d_rx_phyerr_radar",
	"d_rx_phyerr_ofdm_timing",
	"d_rx_phyerr_cck_timing",

};
#define ATH9K_SSTATS_LEN ARRAY_SIZE(ath9k_gstrings_stats)

void ath9k_get_et_strings(struct ieee80211_hw *hw,
			  struct ieee80211_vif *vif,
			  u32 sset, u8 *data)
{
	if (sset == ETH_SS_STATS)
		memcpy(data, *ath9k_gstrings_stats,
		       sizeof(ath9k_gstrings_stats));
}

int ath9k_get_et_sset_count(struct ieee80211_hw *hw,
			    struct ieee80211_vif *vif, int sset)
{
	if (sset == ETH_SS_STATS)
		return ATH9K_SSTATS_LEN;
	return 0;
}

#define AWDATA(elem)							\
	do {								\
		data[i++] = sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_BE)].elem; \
		data[i++] = sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_BK)].elem; \
		data[i++] = sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_VI)].elem; \
		data[i++] = sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_VO)].elem; \
	} while (0)

#define AWDATA_RX(elem)						\
	do {							\
		data[i++] = sc->debug.stats.rxstats.elem;	\
	} while (0)

void ath9k_get_et_stats(struct ieee80211_hw *hw,
			struct ieee80211_vif *vif,
			struct ethtool_stats *stats, u64 *data)
{
	struct ath_softc *sc = hw->priv;
	int i = 0;

	data[i++] = (sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_BE)].tx_pkts_all +
		     sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_BK)].tx_pkts_all +
		     sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_VI)].tx_pkts_all +
		     sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_VO)].tx_pkts_all);
	data[i++] = (sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_BE)].tx_bytes_all +
		     sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_BK)].tx_bytes_all +
		     sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_VI)].tx_bytes_all +
		     sc->debug.stats.txstats[PR_QNUM(IEEE80211_AC_VO)].tx_bytes_all);
	AWDATA_RX(rx_pkts_all);
	AWDATA_RX(rx_bytes_all);

	AWDATA(tx_pkts_all);
	AWDATA(tx_bytes_all);
	AWDATA(queued);
	AWDATA(completed);
	AWDATA(xretries);
	AWDATA(a_aggr);
	AWDATA(a_queued_hw);
	AWDATA(a_queued_sw);
	AWDATA(a_completed);
	AWDATA(a_retries);
	AWDATA(a_xretries);
	AWDATA(fifo_underrun);
	AWDATA(xtxop);
	AWDATA(timer_exp);
	AWDATA(desc_cfg_err);
	AWDATA(data_underrun);
	AWDATA(delim_underrun);

	AWDATA_RX(crc_err);
	AWDATA_RX(decrypt_crc_err);
	AWDATA_RX(phy_err);
	AWDATA_RX(mic_err);
	AWDATA_RX(pre_delim_crc_err);
	AWDATA_RX(post_delim_crc_err);
	AWDATA_RX(decrypt_busy_err);

	AWDATA_RX(phy_err_stats[ATH9K_PHYERR_RADAR]);
	AWDATA_RX(phy_err_stats[ATH9K_PHYERR_OFDM_TIMING]);
	AWDATA_RX(phy_err_stats[ATH9K_PHYERR_CCK_TIMING]);

	WARN_ON(i != ATH9K_SSTATS_LEN);
}

void ath9k_deinit_debug(struct ath_softc *sc)
{
	ath9k_cmn_spectral_deinit_debug(&sc->spec_priv);
}

static ssize_t read_file_tx99(struct file *file, char __user *user_buf,
			      size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	char buf[3];
	unsigned int len;

	len = sprintf(buf, "%d\n", sc->tx99_state);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_tx99(struct file *file, const char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	struct ath_common *common = ath9k_hw_common(sc->sc_ah);
	char buf[32];
	bool start;
	ssize_t len;
	int r;

	if (sc->nvifs > 1)
		return -EOPNOTSUPP;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	if (strtobool(buf, &start))
		return -EINVAL;

	if (start == sc->tx99_state) {
		if (!start)
			return count;
		ath_dbg(common, XMIT, "Resetting TX99\n");
		ath9k_tx99_deinit(sc);
	}

	if (!start) {
		ath9k_tx99_deinit(sc);
		return count;
	}

	r = ath9k_tx99_init(sc);
	if (r)
		return r;

	return count;
}

static const struct file_operations fops_tx99 = {
	.read = read_file_tx99,
	.write = write_file_tx99,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_tx99_power(struct file *file,
				    char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	char buf[32];
	unsigned int len;

	len = sprintf(buf, "%d (%d dBm)\n",
		      sc->tx99_power,
		      sc->tx99_power / 2);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t write_file_tx99_power(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath_softc *sc = file->private_data;
	int r;
	u8 tx_power;

	r = kstrtou8_from_user(user_buf, count, 0, &tx_power);
	if (r)
		return r;

	if (tx_power > MAX_RATE_POWER)
		return -EINVAL;

	sc->tx99_power = tx_power;

	ath9k_ps_wakeup(sc);
	ath9k_hw_tx99_set_txpower(sc->sc_ah, sc->tx99_power);
	ath9k_ps_restore(sc);

	return count;
}

static const struct file_operations fops_tx99_power = {
	.read = read_file_tx99_power,
	.write = write_file_tx99_power,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

int ath9k_init_debug(struct ath_hw *ah)
{
	struct ath_common *common = ath9k_hw_common(ah);
	struct ath_softc *sc = (struct ath_softc *) common->priv;

	sc->debug.debugfs_phy = debugfs_create_dir("ath9k",
						   sc->hw->wiphy->debugfsdir);
	if (!sc->debug.debugfs_phy)
		return -ENOMEM;

#ifdef CONFIG_ATH_DEBUG
	debugfs_create_file("debug", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			    sc, &fops_debug);
#endif

	ath9k_dfs_init_debug(sc);
	ath9k_tx99_init_debug(sc);
	ath9k_cmn_spectral_init_debug(&sc->spec_priv, sc->debug.debugfs_phy);

	debugfs_create_devm_seqfile(sc->dev, "dma", sc->debug.debugfs_phy,
				    read_file_dma);
	debugfs_create_devm_seqfile(sc->dev, "interrupt", sc->debug.debugfs_phy,
				    read_file_interrupt);
	debugfs_create_devm_seqfile(sc->dev, "xmit", sc->debug.debugfs_phy,
				    read_file_xmit);
	debugfs_create_devm_seqfile(sc->dev, "queues", sc->debug.debugfs_phy,
				    read_file_queues);
	debugfs_create_u32("qlen_bk", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			   &sc->tx.txq_max_pending[IEEE80211_AC_BK]);
	debugfs_create_u32("qlen_be", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			   &sc->tx.txq_max_pending[IEEE80211_AC_BE]);
	debugfs_create_u32("qlen_vi", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			   &sc->tx.txq_max_pending[IEEE80211_AC_VI]);
	debugfs_create_u32("qlen_vo", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			   &sc->tx.txq_max_pending[IEEE80211_AC_VO]);
	debugfs_create_devm_seqfile(sc->dev, "misc", sc->debug.debugfs_phy,
				    read_file_misc);
	debugfs_create_devm_seqfile(sc->dev, "reset", sc->debug.debugfs_phy,
				    read_file_reset);

	ath9k_cmn_debug_recv(sc->debug.debugfs_phy, &sc->debug.stats.rxstats);
	ath9k_cmn_debug_phy_err(sc->debug.debugfs_phy, &sc->debug.stats.rxstats);

	debugfs_create_file("rx_chainmask", S_IRWXU | S_IWUSR, sc->debug.debugfs_phy, ah,
					    &fops_rx_chainmask);
	debugfs_create_file("tx_chainmask", S_IRWXU | S_IWUSR, sc->debug.debugfs_phy, ah,
				        &fops_tx_chainmask);
	debugfs_create_file("ani", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc, &fops_ani);
	debugfs_create_bool("paprd", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			    &sc->sc_ah->config.enable_paprd);
	debugfs_create_file("regidx", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			    sc, &fops_regidx);
	debugfs_create_file("regval", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			    sc, &fops_regval);
	debugfs_create_bool("ignore_extcca", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy,
			    &ah->config.cwm_ignore_extcca);
	debugfs_create_file("regdump", S_IRUSR, sc->debug.debugfs_phy, sc,
			    &fops_regdump);
	debugfs_create_devm_seqfile(sc->dev, "dump_nfcal",
				    sc->debug.debugfs_phy,
				    read_file_dump_nfcal);

	ath9k_cmn_debug_base_eeprom(sc->debug.debugfs_phy, sc->sc_ah);
	ath9k_cmn_debug_modal_eeprom(sc->debug.debugfs_phy, sc->sc_ah);
	debugfs_create_file("spectral_scan_ctl", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc,
			    &fops_spec_scan_ctl);
	debugfs_create_file("spectral_short_repeat", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc,
			    &fops_spectral_short_repeat);
	debugfs_create_file("spectral_count", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc, &fops_spectral_count);
	debugfs_create_file("spectral_period", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc, &fops_spectral_period);
	debugfs_create_file("spectral_fft_period", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc,
			    &fops_spectral_fft_period);
	debugfs_create_u32("gpio_mask", S_IRUSR | S_IWUSR,
			   sc->debug.debugfs_phy, &sc->sc_ah->gpio_mask);
	debugfs_create_u32("gpio_val", S_IRUSR | S_IWUSR,
			   sc->debug.debugfs_phy, &sc->sc_ah->gpio_val);
	debugfs_create_file("antenna_diversity", S_IRUSR,
			    sc->debug.debugfs_phy, sc, &fops_antenna_diversity);
#ifdef CONFIG_ATH9K_BTCOEX_SUPPORT
	debugfs_create_file("bt_ant_diversity", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc, &fops_bt_ant_diversity);
	debugfs_create_file("btcoex", S_IRUSR, sc->debug.debugfs_phy, sc,
			    &fops_btcoex);
#endif

#ifdef CONFIG_ATH9K_WOW
	debugfs_create_file("wow", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc, &fops_wow);
#endif

#ifdef CONFIG_ATH9K_DYNACK
	debugfs_create_file("ack_to", S_IRUSR | S_IWUSR, sc->debug.debugfs_phy,
			    sc, &fops_ackto);
#endif
	debugfs_create_file("tpc", S_IRUSR | S_IWUSR,
			    sc->debug.debugfs_phy, sc, &fops_tpc);
	if (config_enabled(CPTCFG_ATH9K_TX99) &&
	    AR_SREV_9300_20_OR_LATER(ah)) {
		debugfs_create_file("tx99", S_IRUSR | S_IWUSR,
				    sc->debug.debugfs_phy, sc,
				    &fops_tx99);
		debugfs_create_file("tx99_power", S_IRUSR | S_IWUSR,
				    sc->debug.debugfs_phy, sc,
				    &fops_tx99_power);
	}

	return 0;
}
