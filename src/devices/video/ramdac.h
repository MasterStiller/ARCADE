// license:BSD-3-Clause
// copyright-holders:Angelo Salese
/***************************************************************************

    Generic Palette RAMDAC device

***************************************************************************/

#ifndef MAME_VIDEO_RAMDAC_H
#define MAME_VIDEO_RAMDAC_H

#pragma once



//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_RAMDAC_ADD(_tag, _map, _palette_tag) \
	MCFG_DEVICE_ADD(_tag, RAMDAC, 0) \
	MCFG_DEVICE_ADDRESS_MAP(0, _map) \
	downcast<ramdac_device &>(*device).set_palette_tag(_palette_tag);

#define MCFG_RAMDAC_COLOR_BASE(_color_base) \
	downcast<ramdac_device &>(*device).set_color_base(_color_base);

#define MCFG_RAMDAC_SPLIT_READ(_split) \
	downcast<ramdac_device &>(*device).set_split_read(_split);


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> ramdac_device

class ramdac_device :   public device_t,
						public device_memory_interface
{
public:
	// construction/destruction
	ramdac_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// configuration
	void set_palette_tag(const char *tag) { m_palette.set_tag(tag); }
	void set_color_base(uint32_t color_base) { m_color_base = color_base; }
	void set_split_read(int split) { m_split_read_reg = split; }

	// I/O operations
	DECLARE_READ8_MEMBER( index_r );
	DECLARE_READ8_MEMBER( pal_r );
	DECLARE_READ8_MEMBER( mask_r );
	DECLARE_WRITE8_MEMBER( index_w );
	DECLARE_WRITE8_MEMBER( index_r_w );
	DECLARE_WRITE8_MEMBER( pal_w );
	DECLARE_WRITE8_MEMBER( mask_w );

	DECLARE_READ8_MEMBER( ramdac_pal_r );
	DECLARE_WRITE8_MEMBER( ramdac_rgb666_w );
	DECLARE_WRITE8_MEMBER( ramdac_rgb888_w );

	void ramdac_palram(address_map &map);

protected:
	// device-level overrides
	virtual void device_validity_check(validity_checker &valid) const override;
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual space_config_vector memory_space_config() const override;

private:
	uint8_t m_pal_index[2];
	uint8_t m_pal_mask;
	uint8_t m_int_index[2];
	std::unique_ptr<uint8_t[]> m_palram;

	const address_space_config      m_space_config;
	required_device<palette_device> m_palette;

	uint32_t m_color_base;
	uint8_t m_split_read_reg; // read register index is separated, seen in rltennis

	inline uint8_t readbyte(offs_t address);
	inline void writebyte(offs_t address, uint8_t data);
	inline void reg_increment(uint8_t inc_type);
};


// device type definition
DECLARE_DEVICE_TYPE(RAMDAC, ramdac_device)

#endif // MAME_VIDEO_RAMDAC_H
