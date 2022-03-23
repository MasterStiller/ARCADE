// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
#pragma once

#ifndef MAME_INCLUDES_NEXT_H
#define MAME_INCLUDES_NEXT_H

#include "cpu/m68000/m68000.h"
#include "imagedev/floppy.h"
#include "machine/nscsi_bus.h"
#include "machine/mccs1850.h"
#include "machine/8530scc.h"
#include "machine/nextkbd.h"
#include "machine/upd765.h"
#include "machine/ncr5390.h"
#include "machine/mb8795.h"
#include "machine/nextmo.h"
#include "imagedev/chd_cd.h"
#include "imagedev/harddriv.h"

class next_state : public driver_device
{
public:
	next_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			maincpu(*this, "maincpu"),
			rtc(*this, "rtc"),
			scc(*this, "scc"),
			keyboard(*this, "keyboard"),
			scsibus(*this, "scsibus"),
			scsi(*this, "scsibus:7:ncr5390"),
			net(*this, "net"),
			mo(*this, "mo"),
			fdc(*this, "fdc"),
			floppy0(*this, "fdc:0"),
			vram(*this, "vram") { }

	void next_mo_config(machine_config &config);
	void next_fdc_config(machine_config &config);
	void next_base(machine_config &config);
	void next_mo_base(machine_config &config);
	void next_fdc_base(machine_config &config);
	void next_mo_fdc_base(machine_config &config);
	void nextst(machine_config &config);
	void nextsc(machine_config &config);
	void nextct(machine_config &config);
	void nexts2(machine_config &config);
	void nextctc(machine_config &config);
	void next(machine_config &config);
	void nextc(machine_config &config);
	void nextstc(machine_config &config);
	void nexts(machine_config &config);

	void init_nexts2();
	void init_next();
	void init_nextc();
	void init_nextsc();
	void init_nextst();
	void init_nextct();
	void init_nextstc();
	void init_nextctc();
	void init_nexts();

private:
	required_device<cpu_device> maincpu;
	required_device<mccs1850_device> rtc;
	required_device<scc8530_legacy_device> scc;
	required_device<nextkbd_device> keyboard;
	required_device<nscsi_bus_device> scsibus;
	required_device<ncr5390_device> scsi;
	required_device<mb8795_device> net;
	optional_device<nextmo_device> mo; // cube only
	optional_device<n82077aa_device> fdc; // 040 only
	optional_device<floppy_connector> floppy0; // 040 only

	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	void setup(uint32_t scr1, int size_x, int size_y, int skip, bool color);

	uint32_t rom_map_r();
	uint32_t scr2_r();
	void scr2_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t scr1_r();
	uint32_t irq_status_r();
	uint32_t irq_mask_r();
	void irq_mask_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t event_counter_r(offs_t offset, uint32_t mem_mask = ~0);
	uint32_t dsp_r();
	uint32_t fdc_control_r();
	void fdc_control_w(uint32_t data);
	uint32_t dma_ctrl_r(offs_t offset);
	void dma_ctrl_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t dma_regs_r(offs_t offset);
	void dma_regs_w(offs_t offset, uint32_t data);
	uint32_t scsictrl_r(offs_t offset, uint32_t mem_mask = ~0);
	void scsictrl_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t phy_r(offs_t offset);
	void phy_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t timer_data_r();
	void timer_data_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	uint32_t timer_ctrl_r();
	void timer_ctrl_w(offs_t offset, uint32_t data, uint32_t mem_mask = ~0);
	void ramdac_w(offs_t offset, uint8_t data);

	uint32_t scr1;
	uint32_t scr2;
	uint32_t irq_status;
	uint32_t irq_mask = 0;
	int irq_level = 0;
	required_shared_ptr<uint32_t> vram;
	uint8_t scsictrl = 0, scsistat = 0;

	uint32_t phy[2];

	attotime timer_tbase;
	uint16_t timer_vbase = 0;
	uint32_t timer_data = 0, timer_next_data = 0;
	uint32_t timer_ctrl = 0;
	emu_timer *timer_tm = nullptr;

	uint32_t eventc_latch = 0;

	DECLARE_WRITE_LINE_MEMBER(scc_irq);
	DECLARE_WRITE_LINE_MEMBER(keyboard_irq);
	DECLARE_WRITE_LINE_MEMBER(power_irq);
	DECLARE_WRITE_LINE_MEMBER(nmi_irq);

	DECLARE_WRITE_LINE_MEMBER(scsi_irq);
	DECLARE_WRITE_LINE_MEMBER(scsi_drq);

	DECLARE_WRITE_LINE_MEMBER(fdc_irq);
	DECLARE_WRITE_LINE_MEMBER(fdc_drq);

	DECLARE_WRITE_LINE_MEMBER(net_tx_irq);
	DECLARE_WRITE_LINE_MEMBER(net_rx_irq);
	DECLARE_WRITE_LINE_MEMBER(net_tx_drq);
	DECLARE_WRITE_LINE_MEMBER(net_rx_drq);

	DECLARE_WRITE_LINE_MEMBER(mo_irq);
	DECLARE_WRITE_LINE_MEMBER(mo_drq);

	DECLARE_WRITE_LINE_MEMBER(vblank_w);

	void ncr5390(device_t *device);
	void next_0b_m_mem(address_map &map);
	void next_0b_m_mo_mem(address_map &map);
	void next_0b_m_nofdc_mem(address_map &map);
	void next_0c_c_mem(address_map &map);
	void next_0c_m_mem(address_map &map);
	void next_0c_c_mo_mem(address_map &map);
	void next_0c_m_mo_mem(address_map &map);
	void next_2c_c_mem(address_map &map);
	void next_fdc_mem(address_map &map);
	void next_mo_mem(address_map &map);
	void next_mem(address_map &map);

	struct dma_slot {
		uint32_t start = 0, limit = 0, chain_start = 0, chain_limit = 0, current = 0;
		uint8_t state = 0;
		bool supdate = false, restart = false, drq = false;
	};

	enum {
		// write bits
		DMA_SETENABLE    = 0x01,
		DMA_SETSUPDATE   = 0x02,
		DMA_SETREAD      = 0x04,
		DMA_CLRCOMPLETE  = 0x08,
		DMA_RESET        = 0x10,
		DMA_INITBUF      = 0x20,
		DMA_INITBUFTURBO = 0x40,

		// read bits
		DMA_ENABLE       = 0x01,
		DMA_SUPDATE      = 0x02,
		DMA_READ         = 0x04,
		DMA_COMPLETE     = 0x08,
		DMA_BUSEXC       = 0x10
	};

	static char const *const dma_targets[0x20];
	static int const dma_irqs[0x20];
	static bool const dma_has_saved[0x20];
	static int const scsi_clocks[4];

	dma_slot dma_slots[0x20];
	uint32_t esp = 0;

	int screen_sx = 0, screen_sy = 0, screen_skip = 0;
	bool screen_color = false;
	bool vbl_enabled = false;

	virtual void machine_start() override;
	virtual void machine_reset() override;

	virtual void device_timer(emu_timer &timer, device_timer_id id, int param) override;
	void timer_start();
	void timer_update();

	void irq_set(int id, bool raise);
	void irq_check();
	const char *dma_name(int slot);
	void dma_do_ctrl_w(int slot, uint8_t data);
	void dma_drq_w(int slot, bool state);
	void dma_read(int slot, uint8_t &val, bool &eof, bool &err);
	void dma_write(int slot, uint8_t val, bool eof, bool &err);
	void dma_check_update(int slot);
	void dma_check_end(int slot, bool eof);
	void dma_end(int slot);
};

#endif
