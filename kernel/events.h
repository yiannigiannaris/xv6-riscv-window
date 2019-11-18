#define EV_SYN			0x00
#define EV_KEY			0x01
#define EV_REL			0x02
#define EV_ABS			0x03
#define EV_MSC			0x04
#define EV_SW			0x05
#define EV_LED			0x11
#define EV_SND			0x12
#define EV_REP			0x14
#define EV_FF			0x15
#define EV_PWR			0x16
#define EV_FF_STATUS		0x17
#define EV_MAX			0x1f
#define EV_CNT			(EV_MAX+1)

#define SYN_REPORT		0
#define SYN_CONFIG		1
#define SYN_MT_REPORT		2
#define SYN_DROPPED		3
#define SYN_MAX			0xf
#define SYN_CNT			(SYN_MAX+1)

#define BTN_MOUSE		0x110
#define BTN_LEFT		0x110
#define BTN_RIGHT		0x111
#define BTN_WHEEL		0x150
#define BTN_GEAR_DOWN		0x150
#define BTN_GEAR_UP		0x151
#define KEY_MAX			0x2ff
#define KEY_CNT			(KEY_MAX+1)

#define REL_X			0x00
#define REL_Y			0x01
#define REL_MAX			0x0f
#define REL_CNT			(REL_MAX+1)
