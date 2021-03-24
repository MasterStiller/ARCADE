// license:BSD-3-Clause
// copyright-holders:hap
// thanks-to:Sean Riddle
/***************************************************************************

National Semiconductor COPS(MM57 MCU series) handhelds

MCU die label for MM5799 games says MM4799, but they are in fact MM5799.

TODO:
- qkracerm link cable

***************************************************************************/

#include "emu.h"

#include "cpu/cops1/mm5799.h"
#include "machine/ds8874.h"
#include "video/pwm.h"
#include "sound/spkrdev.h"

#include "speaker.h"

// internal artwork
#include "mbaskb.lh"
#include "mhockey.lh"
#include "msoccer.lh"
#include "qkracerm.lh"
#include "qkspeller.lh"

//#include "hh_cops1_test.lh" // common test-layout - use external artwork


class hh_cops1_state : public driver_device
{
public:
	hh_cops1_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_display(*this, "display"),
		m_speaker(*this, "speaker"),
		m_inputs(*this, "IN.%u", 0)
	{ }

	// devices
	required_device<cops1_base_device> m_maincpu;
	optional_device<pwm_display_device> m_display;
	optional_device<speaker_sound_device> m_speaker;
	optional_ioport_array<9> m_inputs; // max 9

	u16 m_inp_mux = 0;
	u16 m_grid = 0;

	// MCU output pin state
	u8 m_s = 0;
	u8 m_do = 0;
	u8 m_f = 0;
	int m_blk = false;

	u8 read_inputs(int columns);

protected:
	virtual void machine_start() override;
	virtual void machine_reset() override;
};


// machine start/reset

void hh_cops1_state::machine_start()
{
	// register for savestates
	save_item(NAME(m_inp_mux));
	save_item(NAME(m_grid));
	save_item(NAME(m_s));
	save_item(NAME(m_do));
	save_item(NAME(m_f));
	save_item(NAME(m_blk));
}

void hh_cops1_state::machine_reset()
{
}



/***************************************************************************

  Helper Functions

***************************************************************************/

// generic input handlers

u8 hh_cops1_state::read_inputs(int columns)
{
	u8 ret = 0;

	// read selected input rows
	for (int i = 0; i < columns; i++)
		if (m_inp_mux >> i & 1)
			ret |= m_inputs[i]->read();

	return ret;
}



/***************************************************************************

  Minidrivers (subclass, I/O, Inputs, Machine Config, ROM Defs)

***************************************************************************/

namespace {

/***************************************************************************

  Mattel Basketball (model 2437)
  * PCB label: MA 6017/18/19
  * MM5799 MCU bonded directly to PCB (die label MM4799 C NCX)
  * 4001 and 74145, also bonded to PCB
  * 2-digit 7seg led display, 21 leds, 2-bit sound

  Mattel Soccer (model 2678)
  * MCU die label MM4799 C NDC
  * same hardware as Basketball

  Mattel Hockey (model 2946)
  * MCU die label MM4799 C NFR
  * same hardware as Basketball

  Judging from videos online, there are two versions of Basketball. One where
  the display shows "12" at power-on(as on MAME), and one that shows "15".

***************************************************************************/

class mbaskb_state : public hh_cops1_state
{
public:
	mbaskb_state(const machine_config &mconfig, device_type type, const char *tag) :
		hh_cops1_state(mconfig, type, tag)
	{ }

	void update_display();
	void write_do(u8 data);
	void write_blk(int state);
	void write_s(u8 data);
	void write_f(u8 data);
	u8 read_f();

	void mbaskb(machine_config &config);
	void msoccer(machine_config &config);
	void mhockey(machine_config &config);
};

// handlers

void mbaskb_state::update_display()
{
	// DO4321: 74154 CBAD
	u8 d = (m_do >> 1 & 7) | (m_do << 3 & 8);
	u16 sel = m_blk ? (1 << d) : 0;

	// 74154 output 2,6: speaker out
	u8 spk = bitswap<2>(sel, 6,2);
	m_speaker->level_w(spk);

	// 74154 output 3-5,10-13: digit/led select
	u8 dsp = bitswap<7>(sel, 11,4,12,5,13,3,10);
	m_display->matrix((m_f << 5 & 0x80) | dsp, m_s);
}

void mbaskb_state::write_do(u8 data)
{
	// DO: 74154 inputs
	m_do = data;
	update_display();
}

void mbaskb_state::write_blk(int state)
{
	// BLK: 74154 enable
	m_blk = state;
	update_display();
}

void mbaskb_state::write_s(u8 data)
{
	// Sa-Sg: digit segment/led data
	m_s = data;
	update_display();
}

void mbaskb_state::write_f(u8 data)
{
	// F3: led data
	m_f = data;
	update_display();
}

u8 mbaskb_state::read_f()
{
	// F1: difficulty switch
	// F2: N/C
	return m_inputs[2]->read() | (m_f & 2);
}

// config

static INPUT_PORTS_START( mbaskb )
	PORT_START("IN.0") // port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_16WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_16WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_16WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_16WAY

	PORT_START("IN.1") // INB
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 )

	PORT_START("IN.2") // F1
	PORT_CONFNAME( 0x01, 0x00, DEF_STR( Difficulty ) )
	PORT_CONFSETTING(    0x00, "1" )
	PORT_CONFSETTING(    0x01, "2" )
INPUT_PORTS_END

void mbaskb_state::mbaskb(machine_config &config)
{
	/* basic machine hardware */
	MM5799(config, m_maincpu, 370000); // approximation
	m_maincpu->write_do().set(FUNC(mbaskb_state::write_do));
	m_maincpu->write_blk().set(FUNC(mbaskb_state::write_blk));
	m_maincpu->write_s().set(FUNC(mbaskb_state::write_s));
	m_maincpu->write_f().set(FUNC(mbaskb_state::write_f));
	m_maincpu->read_f().set(FUNC(mbaskb_state::read_f));
	m_maincpu->read_k().set_ioport("IN.0");
	m_maincpu->read_inb().set_ioport("IN.1");

	/* video hardware */
	PWM_DISPLAY(config, m_display).set_size(8, 7);
	m_display->set_segmask(3, 0x7f);
	m_display->set_bri_levels(0.015, 0.2); // ball led is brighter
	config.set_default_layout(layout_mbaskb);

	/* sound hardware */
	SPEAKER(config, "mono").front_center();
	SPEAKER_SOUND(config, m_speaker);
	static const double speaker_levels[4] = { 0.0, 1.0, -1.0, 0.0 };
	m_speaker->set_levels(4, speaker_levels);
	m_speaker->add_route(ALL_OUTPUTS, "mono", 0.25);
}

void mbaskb_state::msoccer(machine_config &config)
{
	mbaskb(config);
	config.set_default_layout(layout_msoccer);
}

void mbaskb_state::mhockey(machine_config &config)
{
	mbaskb(config);
	config.set_default_layout(layout_mhockey);
}

// roms

ROM_START( mbaskb )
	ROM_REGION( 0x0800, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "mm4799_c_ncx", 0x0000, 0x0200, CRC(d0e5fce1) SHA1(04708c043a763bf92d765095e9551388a1d967e8) )
	ROM_CONTINUE(             0x0400, 0x0400 )

	ROM_REGION( 254, "maincpu:opla", 0 )
	ROM_LOAD( "mm5799_common1_output.pla", 0, 254, CRC(c8d225f1) SHA1(4f1e1977e96e53d1d716b7785c4c3971ed9ff65b) )
ROM_END

ROM_START( msoccer )
	ROM_REGION( 0x0800, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "mm4799_c_ndc", 0x0000, 0x0200, CRC(4b5ce604) SHA1(6b3d58f633b4b36f533e9a3b3ca091b2e5ea5018) )
	ROM_CONTINUE(             0x0400, 0x0400 )

	ROM_REGION( 254, "maincpu:opla", 0 )
	ROM_LOAD( "mm5799_common1_output.pla", 0, 254, CRC(c8d225f1) SHA1(4f1e1977e96e53d1d716b7785c4c3971ed9ff65b) )
ROM_END

ROM_START( mhockey )
	ROM_REGION( 0x0800, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "mm4799_c_nfr", 0x0000, 0x0200, CRC(979e5c5b) SHA1(e6c81572f47d93f4c13472f477aac67e05841976) )
	ROM_CONTINUE(             0x0400, 0x0400 )

	ROM_REGION( 254, "maincpu:opla", 0 )
	ROM_LOAD( "mm5799_common1_output.pla", 0, 254, CRC(c8d225f1) SHA1(4f1e1977e96e53d1d716b7785c4c3971ed9ff65b) )
ROM_END





/***************************************************************************

  National Semiconductor QuizKid Racer (MM5799 version)
  * MM5799 MCU bonded directly to PCB (die label MM4799 C DUZ)
  * DS8874 LED driver, die bonded to PCB as well
  * 8-digit 7seg led display(1 custom digit), 1 green led, no sound
  * optional link cable to compete with another player (see patent US4051605)

  This is the first version of QuizKid Racer, the 2nd release is on a
  COP420 MCU, see hh_cop400.cpp.

***************************************************************************/

class qkracerm_state : public hh_cops1_state
{
public:
	qkracerm_state(const machine_config &mconfig, device_type type, const char *tag) :
		hh_cops1_state(mconfig, type, tag),
		m_ds8874(*this, "ds8874")
	{ }

	required_device<ds8874_device> m_ds8874;
	void ds8874_output_w(u16 data);

	void update_display();
	void write_do(u8 data);
	void write_s(u8 data);
	u8 read_f();
	u8 read_k();
	int read_si();
	void qkracerm(machine_config &config);
};

// handlers

void qkracerm_state::update_display()
{
	m_display->matrix(m_grid, m_s);
}

void qkracerm_state::ds8874_output_w(u16 data)
{
	// DS8874 outputs: digit select, input mux
	m_grid = ~data;
	m_inp_mux = m_grid >> 3;
	update_display();
}

void qkracerm_state::write_do(u8 data)
{
	// DO1: DS8874 CP
	// DO4: DS8874 _DATA
	m_ds8874->cp_w(BIT(data, 0));
	m_ds8874->data_w(BIT(data, 3));
}

void qkracerm_state::write_s(u8 data)
{
	// Sa-Sg: digit segment data
	// Sp: link data out
	m_s = data;
	update_display();
}

u8 qkracerm_state::read_f()
{
	// F1: N/C
	// F2: link cable detected
	// F3: link data in
	return m_maincpu->f_output_r() & 1;
}

u8 qkracerm_state::read_k()
{
	// K: multiplexed inputs
	return read_inputs(5);
}

int qkracerm_state::read_si()
{
	// SI: link master(1)/slave(0)
	return 0;
}

// config

static INPUT_PORTS_START( qkracerm )
	PORT_START("IN.0") // DS8874 OUT 4 port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_Q) PORT_NAME("Amateur")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_W) PORT_NAME("Pro")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_E) PORT_NAME("Complex")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_R) PORT_NAME("Tables")

	PORT_START("IN.1") // DS8874 OUT 5 port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_7) PORT_CODE(KEYCODE_7_PAD) PORT_NAME("7")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_8) PORT_CODE(KEYCODE_8_PAD) PORT_NAME("8")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_9) PORT_CODE(KEYCODE_9_PAD) PORT_NAME("9")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_SLASH_PAD) PORT_NAME(UTF8_DIVIDE)

	PORT_START("IN.2") // DS8874 OUT 6 port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_4) PORT_CODE(KEYCODE_4_PAD) PORT_NAME("4")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_5) PORT_CODE(KEYCODE_5_PAD) PORT_NAME("5")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_6) PORT_CODE(KEYCODE_6_PAD) PORT_NAME("6")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_ASTERISK) PORT_NAME(UTF8_MULTIPLY)

	PORT_START("IN.3") // DS8874 OUT 7 port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_1) PORT_CODE(KEYCODE_1_PAD) PORT_NAME("1")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_2) PORT_CODE(KEYCODE_2_PAD) PORT_NAME("2")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_3) PORT_CODE(KEYCODE_3_PAD) PORT_NAME("3")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_MINUS_PAD) PORT_NAME("-")

	PORT_START("IN.4") // DS8874 OUT 8 port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_MINUS) PORT_NAME("Slow")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_EQUALS) PORT_NAME("Fast")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_0) PORT_CODE(KEYCODE_0_PAD) PORT_NAME("0")
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYPAD ) PORT_CODE(KEYCODE_PLUS_PAD) PORT_NAME("+")
INPUT_PORTS_END

void qkracerm_state::qkracerm(machine_config &config)
{
	/* basic machine hardware */
	MM5799(config, m_maincpu, 220000); // approximation
	m_maincpu->set_option_ram_d12(true);
	m_maincpu->set_option_lb_10(5);
	m_maincpu->write_do().set(FUNC(qkracerm_state::write_do));
	m_maincpu->write_s().set(FUNC(qkracerm_state::write_s));
	m_maincpu->read_f().set(FUNC(qkracerm_state::read_f));
	m_maincpu->read_k().set(FUNC(qkracerm_state::read_k));
	m_maincpu->read_si().set(FUNC(qkracerm_state::read_si));

	/* video hardware */
	DS8874(config, m_ds8874).write_output().set(FUNC(qkracerm_state::ds8874_output_w));
	PWM_DISPLAY(config, m_display).set_size(9, 7);
	m_display->set_segmask(0xdf, 0x7f);
	m_display->set_segmask(0x20, 0x41); // equals sign
	config.set_default_layout(layout_qkracerm);

	/* no sound! */
}

// roms

ROM_START( qkracerm )
	ROM_REGION( 0x0800, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "mm4799_c_duz", 0x0000, 0x0200, CRC(8b484d2a) SHA1(809e902a11e23bed010ac795ab8dc50e5c1869dc) )
	ROM_CONTINUE(             0x0400, 0x0400 )

	ROM_REGION( 254, "maincpu:opla", 0 )
	ROM_LOAD( "mm5799_qkracerm_output.pla", 0, 254, CRC(f095fc51) SHA1(e3321d5cc4ecef363df7ef94f47e860c7a6d8c7e) )
ROM_END





/***************************************************************************

  National Semiconductor QuizKid Speller
  * MM5799 MCU bonded directly to PCB (die label MM4799 C NDF)
  * 2-digit 7seg led display, green led, red led, no sound

  The manual included 99 pictures for matching the words, with increased
  difficulty. For example 10 = owl, 90 = kangaroo.

  Modes:
  - Spell: match numbered picture with word
  - Learn: same as Spell, but only the 1st letter
  - Game: player 1 enters word, player 2 needs to guess it

***************************************************************************/

class qkspeller_state : public hh_cops1_state
{
public:
	qkspeller_state(const machine_config &mconfig, device_type type, const char *tag) :
		hh_cops1_state(mconfig, type, tag)
	{ }

	void update_display();
	void write_do(u8 data);
	void write_s(u8 data);
	void write_f(u8 data);
	u8 read_f();
	u8 read_k();
	void qkspeller(machine_config &config);
};

// handlers

void qkspeller_state::update_display()
{
	m_display->matrix((m_f << 1 & 0xc) | (m_do & 3), m_inp_mux);
}

void qkspeller_state::write_do(u8 data)
{
	// DO1,DO2: digit select
	m_do = data;
	update_display();
}

void qkspeller_state::write_s(u8 data)
{
	// S: digit segment data, input mux
	m_inp_mux = data;
	update_display();
}

void qkspeller_state::write_f(u8 data)
{
	// F2,F3: led data
	m_f = data;
	update_display();
}

u8 qkspeller_state::read_f()
{
	// F1: 3-pos switch (GND, floating, Vcc)
	if (m_inputs[8]->read() == 2)
		return m_f;
	else
		return (m_inputs[8]->read() & 1) | (m_f & ~1);
}

u8 qkspeller_state::read_k()
{
	// K: multiplexed inputs
	return read_inputs(8);
}

// config

static INPUT_PORTS_START( qkspeller )
	PORT_START("IN.0") // Sa port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')

	PORT_START("IN.1") // Sb port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')

	PORT_START("IN.2") // Sc port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')

	PORT_START("IN.3") // Sd port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')

	PORT_START("IN.4") // Se port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSPACE) PORT_CODE(KEYCODE_DEL) PORT_CHAR(8) PORT_NAME("Erase")

	PORT_START("IN.5") // Sf port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')

	PORT_START("IN.6") // Sg port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')

	PORT_START("IN.7") // Sp port K
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_NAME("Start")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_ENTER) PORT_CODE(KEYCODE_ENTER_PAD) PORT_CHAR(13) PORT_NAME("Next")

	PORT_START("IN.8") // F1
	PORT_CONFNAME( 0x03, 0x00, "Mode" )
	PORT_CONFSETTING(    0x00, "Spell" )
	PORT_CONFSETTING(    0x02, "Learn" )
	PORT_CONFSETTING(    0x01, "Game" )

	PORT_START("TEST.0") // INB test pad
	PORT_CONFNAME( 0x01, 0x00, "Factory Test 1" )
	PORT_CONFSETTING(    0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(    0x01, DEF_STR( On ) )

	PORT_START("TEST.1") // DO3 test pad
	PORT_CONFNAME( 0x01, 0x00, "Factory Test 2" )
	PORT_CONFSETTING(    0x00, DEF_STR( Off ) )
	PORT_CONFSETTING(    0x01, DEF_STR( On ) )
INPUT_PORTS_END

void qkspeller_state::qkspeller(machine_config &config)
{
	/* basic machine hardware */
	MM5799(config, m_maincpu, 220000); // approximation
	m_maincpu->write_do().set(FUNC(qkspeller_state::write_do));
	m_maincpu->write_s().set(FUNC(qkspeller_state::write_s));
	m_maincpu->write_f().set(FUNC(qkspeller_state::write_f));
	m_maincpu->read_f().set(FUNC(qkspeller_state::read_f));
	m_maincpu->read_k().set(FUNC(qkspeller_state::read_k));
	m_maincpu->read_inb().set_ioport("TEST.0");
	m_maincpu->read_do3().set_ioport("TEST.1");

	/* video hardware */
	PWM_DISPLAY(config, m_display).set_size(4, 7);
	m_display->set_segmask(3, 0x7f);
	config.set_default_layout(layout_qkspeller);

	/* no sound! */
}

// roms

ROM_START( qkspeller )
	ROM_REGION( 0x0800, "maincpu", ROMREGION_ERASE00 )
	ROM_LOAD( "mm4799_c_ndf", 0x0000, 0x0200, CRC(0f497a12) SHA1(78eb79dcc414ed2a955450ffb3be431ecc2edb0c) )
	ROM_CONTINUE(             0x0400, 0x0400 )

	ROM_REGION( 254, "maincpu:opla", 0 )
	ROM_LOAD( "mm5799_common1_output.pla", 0, 254, CRC(c8d225f1) SHA1(4f1e1977e96e53d1d716b7785c4c3971ed9ff65b) )
ROM_END



} // anonymous namespace

/***************************************************************************

  Game driver(s)

***************************************************************************/

//    YEAR  NAME       PARENT  CMP MACHINE    INPUT      CLASS            INIT        COMPANY, FULLNAME, FLAGS
CONS( 1978, mbaskb,    0,       0, mbaskb,    mbaskb,    mbaskb_state,    empty_init, "Mattel", "Basketball (Mattel)", MACHINE_SUPPORTS_SAVE )
CONS( 1978, msoccer,   0,       0, msoccer,   mbaskb,    mbaskb_state,    empty_init, "Mattel", "Soccer (Mattel)", MACHINE_SUPPORTS_SAVE )
CONS( 1978, mhockey,   0,       0, mhockey,   mbaskb,    mbaskb_state,    empty_init, "Mattel", "Hockey (Mattel)", MACHINE_SUPPORTS_SAVE )

CONS( 1977, qkracerm,  qkracer, 0, qkracerm,  qkracerm,  qkracerm_state,  empty_init, "National Semiconductor", "QuizKid Racer (MM5799 version)", MACHINE_SUPPORTS_SAVE | MACHINE_NO_SOUND_HW | MACHINE_NODEVICE_LAN )
CONS( 1978, qkspeller, 0,       0, qkspeller, qkspeller, qkspeller_state, empty_init, "National Semiconductor", "QuizKid Speller", MACHINE_SUPPORTS_SAVE | MACHINE_NO_SOUND_HW )
