#ifndef FM34_AMB_H
#define FM34_AMB_H
struct fm34_platform_data {
	unsigned rst_pin;
	unsigned pwdn_pin;
	unsigned spk_mute_pin;
	unsigned rst_delay;
	unsigned short *conf;
	int cconf;
};

#endif
