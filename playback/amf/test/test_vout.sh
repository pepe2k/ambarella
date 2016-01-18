#!/bin/sh

switch_2160p30_1080p60() {
	while true;
	do
		test2 -V2160p30 --hdmi --cs rgb --idle
		test2 -V1080p --hdmi --cs rgb --idle
	done
}

switch_2160p30_2160p24() {
	while true;
	do
		test2 -V2160p30 --hdmi --cs rgb --idle
		test2 -V2160p24 --hdmi --cs rgb --idle
	done
}

switch_2160p24_1080p60() {
	while true;
	do
		test2 -V2160p24 --hdmi --cs rgb --idle
		test2 -V1080p --hdmi --cs rgb --idle
	done
}

switch_2160p30_2160p24_1080p60() {
	while true;
	do
		test2 -V2160p30 --hdmi --cs rgb --idle
		test2 -V2160p24 --hdmi --cs rgb --idle
		test2 -V1080p --hdmi --cs rgb --idle
	done
}

switch_2160p30_1080p60

