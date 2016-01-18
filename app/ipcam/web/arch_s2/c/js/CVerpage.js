/////////////////////////////////////////////////////////////////////
//
// CVerpage.js
//
/////////History:
//	2010/02/28 - [Yi Zhu] Created this file
//	2010/06/01 - [Jian Tang] Added ImageParam struct
//
// Copyright (C) 2007 - 2010, Ambarella, Inc.
//
// All rights reserved. No Part of this file may be reproduced, stored
// in a retrieval system, or transmitted, in any form, or by any means,
// electronic, mechanical, photocopying, recording, or otherwise,
// without the prior consent of Ambarella, Inc.
//
/////////////////////////////////////////////////////////////////////

function VinVoutParam()
{
	this.mem = new Array(
		['vin_mode', 0],
		['vin_framerate',0],
		['vin_mirror', 0],
		['vin_bayer',0 ],
		['vout_type', 0],
		['vout_mode', 0]
	);
	this.count = this.mem.length;
}

function EncBufParam()
{
	this.mem = new Array(
		['enc_buf0_resolution', 0],
		['enc_buf0_deintlc', 0],
		['enc_buf1_resolution', 0],
		['enc_buf1_deintlc', 0]
	);
	this.count = this.mem.length;
}

function EncParam()
{
	this.mem = new Array(
		['enc_mode', 0],
		['s0_state', 0],
		['s0_type', 0],
		['s0_enc_fps', 0],
		['s0_dptz', 0],
		['s0_resolution', 0],
		['s0_flip_rotate', 0],
		['s1_state', 0],
		['s1_type', 0],
		['s1_enc_fps', 0],
		['s1_dptz', 0],
		['s1_resolution', 0],
		['s1_flip_rotate', 0],
		['s2_state', 0],
		['s2_type', 0],
		['s2_enc_fps', 0],
		['s2_dptz', 0],
		['s2_resolution', 0],
		['s2_flip_rotate', 0],
		['s3_state', 0],
		['s3_type', 0],
		['s3_enc_fps', 0],
		['s3_dptz', 0],
		['s3_resolution', 0],
		['s3_flip_rotate', 0]
	);
	this.count = this.mem.length;
}

function PMParam()
{
	this.mem = new Array(
		['pm_left', 0],
		['pm_top', 0],
		['pm_w', 0],
		['pm_h', 0],
		['pm_color', 0],
		['pm_action', 0]
	);
	this.count = this.mem.length;
}

function OSDParam()
{
	this.mem = new Array(
		[ 's0_no_rotate', 0 ],
		[ 's0_bmp_enable', 0 ],
		[ 's0_time_enable', 0 ],
		[ 's0_text_enable', 0 ],
		[ 's0_text', 0 ],
		[ 's0_text_size', 0 ],
		[ 's0_text_outline', 0 ],
		[ 's0_text_color', 0 ],
		[ 's0_text_bold', 0 ],
		[ 's0_text_italic', 0 ],
		[ 's0_text_startx', 0 ],
		[ 's0_text_starty', 0 ],
		[ 's0_text_boxw', 0 ],
		[ 's0_text_boxh', 0 ],

		[ 's1_no_rotate', 0 ],
		[ 's1_bmp_enable', 0 ],
		[ 's1_time_enable', 0 ],
		[ 's1_text_enable', 0 ],
		[ 's1_text', 0 ],
		[ 's1_text_size', 0 ],
		[ 's1_text_outline', 0 ],
		[ 's1_text_color', 0 ],
		[ 's1_text_bold', 0 ],
		[ 's1_text_italic', 0 ],
		[ 's1_text_startx', 0 ],
		[ 's1_text_starty', 0 ],
		[ 's1_text_boxw', 0 ],
		[ 's1_text_boxh', 0 ],

		[ 's2_no_rotate', 0 ],
		[ 's2_bmp_enable', 0 ],
		[ 's2_time_enable', 0 ],
		[ 's2_text_enable', 0 ],
		[ 's2_text', 0 ],
		[ 's2_text_size', 0 ],
		[ 's2_text_outline', 0],
		[ 's2_text_color', 0 ],
		[ 's2_text_bold', 0 ],
		[ 's2_text_italic', 0 ],
		[ 's2_text_startx', 0 ],
		[ 's2_text_starty', 0 ],
		[ 's2_text_boxw', 0 ],
		[ 's2_text_boxh', 0 ],

		[ 's3_no_rotate', 0 ],
		[ 's3_bmp_enable', 0 ],
		[ 's3_time_enable', 0 ],
		[ 's3_text_enable', 0 ],
		[ 's3_text', 0 ],
		[ 's3_text_size', 0 ],
		[ 's3_text_outline', 0 ],
		[ 's3_text_color', 0 ],
		[ 's3_text_bold', 0 ],
		[ 's3_text_italic', 0 ],
		[ 's3_text_startx', 0 ],
		[ 's3_text_starty', 0 ],
		[ 's3_text_boxw', 0 ],
		[ 's3_text_boxh', 0 ]
	);
	this.count = this.mem.length;
}

function StreamParam()
{
	this.create_mem = function(streamId)
	{
		prefix = 's' + streamId + '_';
		this.mem = new Array(
			[ prefix + 'info_id', 0 ],
			[ prefix + 'state', 0 ],
			[ prefix + 'id', 0 ],
			[ prefix + 'h264_id', 0 ],
			[ prefix + 'M', 0 ],
			[ prefix + 'N', 0 ],
			[ prefix + 'idr_interval', 0 ],
			[ prefix + 'gop_model', 0 ],
			[ prefix + 'profile', 0 ],
			[ prefix + 'brc', 0 ],
			[ prefix + 'cbr_avg_bps', 0],
			[ prefix + 'vbr_min_bps', 0 ],
			[ prefix + 'vbr_max_bps', 0 ],
			[ prefix + 'quality', 0 ]
		);
		this.count = this.mem.length;
	}
}

function ImageParam()
{
	this.mem = new Array(
		['preference', 0],
		['dn_mode', 0],
		['exposure_mode', 0],
		['backlight_comp', 0],
		['local_exposure', 0],
		['dc_iris', 0],
		['dc_iris_duty', 0],
		['mctf_strength', 0],
		['shutter_min', 0],
		['shutter_max', 0],
		['max_gain', 0],
		['ae_target_ratio', 0],
		['vin_fps', 0],
		['saturation', 0],
		['brightness', 0],
		['hue', 0],
		['contrast', 0],
		['sharpness', 0],
		['wbc', 0]
	);
	this.count = this.mem.length;
}

function AFParam()
{
	this.mem = new Array(
		['lens_type', 0],
		['af_mode', 0],
		['af_tile_mode', 0],
		['zm_dist', 0],
		['fs_dist', 0],
		['fs_near', 0],
		['fs_far', 0]
	);
	this.count = this.mem.length;
}

function LiveParam()
{
	this.mem = new Array(
		['encode_type', 0],
		['encode_fps', 0],
		['encode_width',0],
		['encode_height',0],
		['brc_mode', 0],
		['cbr_avg_bps', 0],
		['vbr_min_bps', 0],
		['vbr_max_bps', 0]
	);
	this.count = this.mem.length;
}

function Pack()
{
	this.mem = '';
}

var g_VinVoutParam = new VinVoutParam();
var g_EncBufParam = new EncBufParam();
var g_EncParam = new EncParam();
var g_PMParam = new PMParam();
var g_OSDParam = new OSDParam();
var g_StreamParam = new Array(
	new StreamParam(),
	new StreamParam(),
	new StreamParam(),
	new StreamParam()
);
var g_ImageParam = new ImageParam();
var g_AFParam = new AFParam();
var g_LiveParam = new LiveParam();
var installed = false;
var version = new Array();
var highestVer = 5;
var FlySetOps = {};
	FlySetOps.Play = 1;
	FlySetOps.Stop = 2;
	FlySetOps.ForceIdr = 3;
	FlySetOps.ChangeFr = 4;
	FlySetOps.ChangeBRC = 5;
	FlySetOps.ChangeCBRAvgBps = 6;
	FlySetOps.ChangeVBRMaxBps = 7;

function NumOnly()
{
	// 0x30~0x39 is '0~9'
	if (event.keyCode < 0x30 ||event.keyCode > 0x39)
		event.returnValue = false;
}

function $(tagName)
{
	return document.getElementById(tagName);
}

function browserInfo()
{
	var sys = {};
	var browserName = navigator.userAgent.toLowerCase();
	if (/msie/i.test(browserName) && !/opera/.test(browserName)) {
		sys.name = "IE";
		sys.version = browserName.match(/msie ([\d.]+)/)[1];
	} else if (/firefox/i.test(browserName)) {
		sys.name = "Firefox";
		sys.version = browserName.match(/firefox\/([\d.]+)/)[1];
	} else if (/chrome/i.test(browserName) && /webkit/i.test(browserName) && /mozilla/i.test(browserName)) {
		sys.name = "Chrome";
		sys.version = browserName.match(/chrome\/([\d.]+)/)[1];
	} else {
		sys.name = "Others";
		sys.version = "1.0.0";
	}
	return sys;
}

function add_to_pack(pack, name, value)
{
	pack.mem += name + " = " + value + "\n";
}

function get_page_data(Param)
{
	for (var i = 0; i < Param.count; i++) {
		var obj = $(Param.mem[i][0]);
		if (obj != undefined) {
			switch (obj.type) {
			case "text" :
			case "select-one" :
				Param.mem[i][1] = obj.value;
				break;
			case "radio" :
				var radio = document.getElementsByName("n_" + Param.mem[i][0]);
				for (var j = 0; j < radio.length; j++) {
					if (radio[j].checked) {
						Param.mem[i][1] = radio[j].value;
					}
				}
				break;
			case "checkbox" :
				if (obj.checked) {
					Param.mem[i][1] = 1;
				} else {
					Param.mem[i][1] = 0;
				}
				break;
			default :
				alert("Unknown obj name : [" + Param.mem[i][0] + "], type is : ["+obj.type+"].\n");
				break;
			}
		}
	}
}

function set_page_data(Param)
{
	var i;
	for (i = 0; i < Param.count; i++) {
		var obj = $(Param.mem[i][0]);
		if (obj != undefined) {
			switch (obj.type) {
			case "text" :
			case "select-one" :
				obj.value = Param.mem[i][1];
				break;
			case "radio" :
				var radio = document.getElementsByName("n_" + Param.mem[i][0]);
				for (var j = 0; j < radio.length; j++) {
					if (Param.mem[i][1] == radio[j].value) {
						radio[j].checked = true;
					} else {
						radio[j].checked = false;
					}
				}
				break;
			case "checkbox" :
				if (Param.mem[i][1] == 1) {
					obj.checked = true;
				} else {
					obj.checked = false;
				}
				break;
			default :
				alert("Unknown type is : ["+obj.type+"].\n");
				break;
			}
		}
	}
}

function showStatus(text)
{
	if (text == undefined) {
		text = '';
	}
	var la = $("status");
	la.innerHTML = text;
	la.style.visibility = "visible";
}

function hideStatus() {
	var la = $("status");
	la.innerHTML = 'i';
	la.style.visibility = "hidden";
}

function showPage(page, postData)
{
	try {
		var url = "/cgi-bin/webdemo.cgi?page="+page;
		var ai = new AJAXInteraction(url, display);
		if (postData == undefined) 	{
			postData = ''
		}
		ai.doPost(postData);
	} catch (error) {
		alert(error);
	}

	function display(response_text) {
		$("content").innerHTML = response_text;
	}
}

function getData(pageName, detail)
{
	switch (pageName) {
		case 'csb':
			get_page_data(g_VinVoutParam);
			break;
		case 'enc':
			get_page_data(g_EncParam);
			get_page_data(g_EncBufParam);
			for (var i = 0; i < 4; ++i) {
				g_StreamParam[i].create_mem(i);
				get_page_data(g_StreamParam[i]);
			}
			break;
		case 'osd':
			get_page_data(g_OSDParam);
			break;
		case 'pm':
			get_page_data(g_PMParam);
			break;
		case 'iqb':
			get_page_data(g_ImageParam);
			break;
		case 'stream':
			g_StreamParam[detail].create_mem(detail);
			get_page_data(g_StreamParam[detail]);
			break;
		default:
			break;
	}
}


function setCamBasic()
{
	try {
		var i;
		var new_VinVoutParam = new VinVoutParam();
		get_page_data(new_VinVoutParam);
		var VinVoutPack = new Pack();
		var new_ImageParam = new ImageParam();
		get_page_data(new_ImageParam);
		var ImagePack = new Pack();
		for (i = 0; i < new_VinVoutParam.count; i++) {
			if (new_VinVoutParam.mem[i][1] != g_VinVoutParam.mem[i][1]) {
				add_to_pack(VinVoutPack, new_VinVoutParam.mem[i][0], new_VinVoutParam.mem[i][1]);
				if (new_VinVoutParam.mem[i][0] == "vin_framerate") {
					add_to_pack(ImagePack, "vin_fps", new_VinVoutParam.mem[i][1]);
				}
			}
		}

		if (VinVoutPack.mem == '') {
			alert("Nothing changed");
			return;
		}

		showStatus("Setting camera basic parameters ....");
		var url = "/cgi-bin/webdemo.cgi?page=csb";
		var ai = new AJAXInteraction(url, display);
		var req_cnt = 0;
		var postData = '';

		if (ImagePack.mem != '') {
			postData += "&sec" + req_cnt + "=IMAGE&data" + req_cnt + "=" + encodeURIComponent(ImagePack.mem);
			req_cnt++;
		}
		if (VinVoutPack.mem != '') {
			postData += "&sec" + req_cnt + "=VINVOUT&data" + req_cnt + "=" + encodeURIComponent(VinVoutPack.mem);
			req_cnt++;
		}
		if (req_cnt != 0) {
			postData = "req_cnt=" + req_cnt + postData;
			ai.doPost(postData);
		}
	} catch (error) {
		alert(error);
	}

	function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if (ret == '0') {
			alert(response_text.substring(2));
			g_VinVoutParam = new_VinVoutParam;
		} else if (ret == '1') {
			alert(response_text.substring(2));
			set_page_data(g_VinVoutParam);
		} else {
			alert("unexpected error");
		}
	}
}


function setEnc()
{
	try {
		var i, streamChanged = 0;
		var new_EncParam = new EncParam();
		get_page_data(new_EncParam);
		var EncPack = new Pack();
		var streamId;
		var strmchanged = new Array("0","0","0","0");
		var new_StreamParam = new Array(
			new StreamParam(),
			new StreamParam(),
			new StreamParam(),
			new StreamParam()
		);
		var StreamPack = new Array(
			new Pack(),
			new Pack(),
			new Pack(),
			new Pack()
		);

		for (i = 0; i < g_EncParam.count; ++i) {
			if (new_EncParam.mem[i][1] != g_EncParam.mem[i][1]) {
				if (new_EncParam.mem[i][0].substring(2) != "_resolution") {
					add_to_pack(EncPack, new_EncParam.mem[i][0], new_EncParam.mem[i][1]);
				} else {
					var width = new_EncParam.mem[i][1] >> 16;
					var height = new_EncParam.mem[i][1] & 0xFFFF;
					add_to_pack(EncPack, new_EncParam.mem[i][0].substring(0, 2)+"_width", width);
					add_to_pack(EncPack, new_EncParam.mem[i][0].substring(0, 2)+"_height", height);
				}
			}
		}
		for (streamId = 0; streamId < 4; ++streamId) {
			new_StreamParam[streamId].create_mem(streamId);
			get_page_data(new_StreamParam[streamId]);

			for (i = 0; i < new_StreamParam[0].count; i++) {
				if (new_StreamParam[streamId].mem[i][1] != g_StreamParam[streamId].mem[i][1]) {
					add_to_pack(StreamPack[streamId], new_StreamParam[streamId].mem[i][0],
						new_StreamParam[streamId].mem[i][1]);
					strmchanged[streamId] = 1;
				}
			}
		}

		showStatus("Setting encoding parameters ....");
		var url = "/cgi-bin/webdemo.cgi?page=enc";
		var ai = new AJAXInteraction(url, display);
		var req_cnt = 0;
		var postData = '';
		if (EncPack.mem != '') {
			postData += '&sec' + req_cnt + "=ENCODE&data" + req_cnt + "=" + encodeURIComponent(EncPack.mem);
			req_cnt++;
		}
		for (streamId = 0; streamId < 4; ++streamId) {
			if (strmchanged[streamId] == 1) {
				postData += "&sec"+ req_cnt +"=STREAM" + streamId + "&data"+ req_cnt +"=" +
					encodeURIComponent(StreamPack[streamId].mem);
				req_cnt ++;
			}
		}
		if (req_cnt != 0) {
			postData = "req_cnt=" + req_cnt + postData;
			ai.doPost(postData);
		} else {
			hideStatus();
			alert("Nothing changed");
			return;
		}
	} catch (error) {
		alert(error);
	}

	function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if (ret == '0') {
			alert(response_text.substring(2));
			g_EncParam = new_EncParam;
			for (streamId = 0; streamId < 4; ++streamId) {
				g_StreamParam[streamId] = new_StreamParam[streamId];
			}
		} else if (ret == '1') {
			alert(response_text.substring(2));
			set_page_data(g_EncParam);
			for (streamId = 0; streamId < 4; ++streamId) {
				set_page_data(g_StreamParam[streamId]);
			}
		} else {
			alert("unexpected error");
		}
	}
}

function addRemovePrivacyMask(add_or_remove)
{
	try {
		var new_PMParam = new PMParam();
		var PMPack = new Pack();
		get_page_data(new_PMParam);
		for (var i = 0; i < new_PMParam.count; i++) {
			if ( (new_PMParam.mem[i][1] - g_PMParam.mem[i][1]) ) {
				add_to_pack(PMPack, new_PMParam.mem[i][0], new_PMParam.mem[i][1]);
			}
			if (i == new_PMParam.count - 1) {
				add_to_pack(PMPack, new_PMParam.mem[i][0], add_or_remove);
			}
		}

		if (PMPack.mem == '') {
			alert("No new mask rectangle added");
			return;
		}

		var browser = browserInfo();
		if (browser.name != "IE") {
			addRemoveMask(new_PMParam, add_or_remove);
		}

		showStatus("Add Privacy Mask ....");
		var url = "/cgi-bin/webdemo.cgi?page=pm";
		var ai = new AJAXInteraction(url, display);
		var req_cnt = 0;
		var postData = '';
		if (PMPack.mem != '') {
			postData += "&sec" + req_cnt + "=PRIMASK&data" + req_cnt + "=" + encodeURIComponent(PMPack.mem);
			req_cnt++;
		}
		if (req_cnt != 0) {
			postData = "req_cnt=" + req_cnt + postData;
			ai.doPost(postData);
		}
	} catch (error) {
		alert(error);
	}

function addRemoveMask(new_PMParam, add_or_remove) {
		var string, w, h;
		var mask, canvas;
		var div, img;
		var left = new_PMParam.mem[0][1];
		var top = new_PMParam.mem[1][1];
		var width = new_PMParam.mem[2][1];
		var height = new_PMParam.mem[3][1];

		mask = $("pm_mask");
		canvas = $('pm_canvas');
		w = canvas.getAttribute('width');
		h = canvas.getAttribute('height');

		img = document.createElement('img');
		if (add_or_remove) {
			img.setAttribute('src', '../img/gray.gif');
		} else {
			img.setAttribute('src', '../img/cyan.gif');
		}
		img.setAttribute('style', 'border: 1px solid');
		string = width * w / 100;
		img.setAttribute('width', string);
		string = height * h / 100;
		img.setAttribute('height', string);

		div = document.createElement('div');
		div.appendChild(img);
		string = 'position:absolute; left:' + left + '%';
		string += '; top:' + top + '%';
		div.setAttribute('style', string);
		mask.appendChild(div);
	}

function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if (ret == '0') {
			alert(response_text.substring(2));
			g_PMParam = new_PMParam;
		} else if (ret == '1') {
			alert(response_text.substring(2));
			set_page_data(g_PMParam);
		} else {
			alert("unexpected error");
		}
	}
}

function clearPrivacyMask()
{
	try {
		clearAllMasks();
		var PMPack = new Pack();
		for (var i = 0; i < g_PMParam.count; i++) {
			if (g_PMParam.mem[i][0] == 'pm_action')
			g_PMParam.mem[i][1] = 3;
			else
				g_PMParam.mem[i][1] = 0;
			add_to_pack(PMPack, g_PMParam.mem[i][0], g_PMParam.mem[i][1]);
		}

		showStatus("Clear all privacy masks ....");
		var url = "/cgi-bin/webdemo.cgi?page=pm";
		var ai = new AJAXInteraction(url, display);
		var postData = '';
		postData = "req_cnt=1&sec0=PRIMASK&data0=" + encodeURIComponent(PMPack.mem);
		ai.doPost(postData);
	} catch (error) {
		alert(error);
	}

	function getLastChild(n) {
		var x = n.lastChild;
		while (!x && x.nodeType != 1) {
			x = x.previousSibling;
		}
		return x;
	}

	function clearAllMasks() {
		var mask;
		mask = $("pm_mask");
		while (mask.hasChildNodes()) {
			var node = getLastChild(mask);
			mask.removeChild(node);
		}
	}

	function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if ((ret == '0') || (ret == '1')) {
			alert(response_text.substring(2));
			set_page_data(g_PMParam);
		} else {
			alert("unexpected error");
		}
	}
}

function setOSD()
{
	try {
		var i, content;
		var new_OSDParam = new OSDParam();
		get_page_data(new_OSDParam);
		var OSDPack = new Pack();
		for (i = 0; i < new_OSDParam.count; i++) {
			if (new_OSDParam.mem[i][1] != g_OSDParam.mem[i][1]) {
				if (new_OSDParam.mem[i][0].substring(2) != "_text") {
					add_to_pack(OSDPack, new_OSDParam.mem[i][0], new_OSDParam.mem[i][1]);
				} else {
					content = '"' + new_OSDParam.mem[i][1] + '"';
					add_to_pack(OSDPack, new_OSDParam.mem[i][0], content);
				}
			}
		}

		if (OSDPack.mem == '') {
			alert("Nothing changed");
			return;
		}

		showStatus("Setting OSD parameters ....");
		var url = "/cgi-bin/webdemo.cgi?page=osd";
		var ai = new AJAXInteraction(url, display);
		var req_cnt = 0;
		var postData = '';
		if (OSDPack.mem != '') {
			postData += "&sec" + req_cnt + "=OSD&data" + req_cnt + "=" + encodeURIComponent(OSDPack.mem);
			req_cnt++;
		}
		if (req_cnt != 0) {
			postData = "req_cnt=" + req_cnt + postData;
			ai.doPost(postData);
		}
	} catch (error) {
		alert(error);
	}

	function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if (ret == '0') {
			alert(response_text.substring(2));
			g_OSDParam = new_OSDParam;
		} else if (ret == '1') {
			alert(response_text.substring(2));
			set_page_data(g_OSDParam);
		} else {
			alert("unexpected error");
		}
	}
}

function setStream(streamId)
{
	try {
		var i, brc_mode, name;
		var new_StreamParam = new StreamParam();
		new_StreamParam.create_mem(streamId);
		get_page_data(new_StreamParam);
		var StreamPack = new Pack();
		for (i = 0; i < new_StreamParam.count; i++) {
			if (new_StreamParam.mem[i][1] != g_StreamParam[streamId].mem[i][1]) {
				add_to_pack(StreamPack, new_StreamParam.mem[i][0], new_StreamParam.mem[i][1]);
			}
		}

		if (StreamPack.mem == '') {
			return 0;
		}

		showStatus("Setting stream parameters ....");
		var url = "/cgi-bin/webdemo.cgi?live_streamId=" + streamId;
		var ai = new AJAXInteraction(url, display);
		var req_cnt = 0;
		var postData = '';
		if (StreamPack.mem != '') {
			req_cnt ++;
			postData += "&sec0=STREAM" + streamId + "&data0=" + encodeURIComponent(StreamPack.mem);
		}
		if (req_cnt != 0) {
			postData = "req_cnt=" + req_cnt + postData;
			ai.doPost(postData)
		}
		return 1;
	} catch (error) {
		alert(error)
	}

	function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if (ret == '0') {
			g_StreamParam[streamId] = new_StreamParam;
		} else if (ret == '1') {
			alert(response_text.substring(2));
			set_page_data(g_StreamParam[streamId]);
		} else {
			alert("unexpected error");
		}
	}
}

function setIQBasic()
{
	try {
		var i;
		var new_ImageParam = new ImageParam();
		get_page_data(new_ImageParam);
		var ImagePack = new Pack();
		for (i = 0; i < new_ImageParam.count; i++) {
			if (new_ImageParam.mem[i][1] != g_ImageParam.mem[i][1]) {
				add_to_pack(ImagePack, new_ImageParam.mem[i][0], new_ImageParam.mem[i][1]);
			}
		}

		if (ImagePack.mem == '') {
			alert("Nothing changed");
			return;
		}

		showStatus("Setting image quality basic parameters ....");
		var url = "/cgi-bin/webdemo.cgi?page=iqb";
		var ai = new AJAXInteraction(url, display);
		var req_cnt = 0;
		var postData = '';
		if (ImagePack.mem != '') {
			postData += "&sec" + req_cnt + "=IMAGE&data" + req_cnt + "=" + encodeURIComponent(ImagePack.mem);
			req_cnt++;
		}
		if (req_cnt != 0) {
			postData = "req_cnt=" + req_cnt + postData;
			ai.doPost(postData);
		}
	} catch (error) {
		alert(error);
	}

	function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if (ret == '0') {
			alert(response_text.substring(2));
			g_ImageParam = new_ImageParam;
		} else if (ret == '1') {
			alert(response_text.substring(2));
			set_page_data(g_ImageParam);
		} else {
			alert("unexpected error");
		}
	}
}

function setIQAF()
{
	try {
		var i;
		var new_AFParam = new AFParam();
		get_page_data(new_AFParam);
		var AFPack = new Pack();
		for (i = 0; i < new_AFParam.count; i++) {
			if ( (new_AFParam.mem[i][1] - g_AFParam.mem[i][1]) ) {
				add_to_pack(AFPack, new_AFParam.mem[i][0], new_AFParam.mem[i][1]);
			}
		}
		if (AFPack.mem == '') {
			alert("Nothing changed");
			return;
		}
		showStatus("Setting image AF parameters ....");
		var url = "/cgi-bin/webdemo.cgi?page=iqaf";
		var ai = new AJAXInteraction(url, display);
		var req_cnt = 0;
		var postData = '';
		if (AFPack.mem != '') {
			postData += "&sec0=IQAF&data0=" + encodeURIComponent(AFPack.mem);
			req_cnt++;
		}
		if (req_cnt != 0) {
			postData = "req_cnt=" + req_cnt + postData;
			ai.doPost(postData);
		}
	} catch (error) {
		alert(error);
	}

	function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if (ret == '0') {
			alert(response_text.substring(2));
			g_AFParam = new_AFParam;
			set_page_data(g_AFParam);
		} else if (ret == '1') {
			alert(response_text.substring(2));
			set_page_data(g_AFParam);
		} else {
			alert("unexpected error");
		}
	}
}
function Record()
{
	if (window.ActiveXObject) {
		try {
			var obj = $("AmbaIPCmrWebPlugIn1");
			var record_status = obj.GetRecordStatus();
			var record = $("Record");
			if (record_status == false ) {
				obj.Record(true);
				record.value = "Stop Rec";
			} else {
				obj.Record(false);
				record.value = "Start Rec";
			}
		} catch (e) {
			alert(e);
		}
	}
}
function ShowStat()
{
	if (window.ActiveXObject) {
		try {
			var obj = $("AmbaIPCmrWebPlugIn1");
			var stat = $("Stat");
			if (stat.value == "Hide Statistics") {
				if (obj.ShowStat(false)) {
					stat.value = "Show Statistics";
				}
			} else {
				if (obj.ShowStat(true)) {
					stat.value = "Hide Statistics";
				}
			}
		} catch (e) {
			alert(e);
		}
	}
}

function ShowDPTZ()
{
	if (window.ActiveXObject) {
		try {
			var obj = $("AmbaIPCmrWebPlugIn1");
			var dptz = $("DPTZ");
			if (dptz.value == "Hide Digital PTZ") {
				if (obj.ShowDPTZ(false)) {
					dptz.value = "Show Digital PTZ";
				}
			} else {
				if (obj.ShowDPTZ(true)) {
					dptz.value = "Hide Digital PTZ";
				}
			}
		} catch (e) {
			alert(e);
		}
	}
}
function OnLoadActiveX(hostname, stream_id, recvType, statSize, showStat)
{
	var browser = browserInfo();
		if (browser.name != "IE") {
	if( navigator.appName.indexOf("Microsoft Internet")==-1 )
	{
		onVLCPluginReady()
	}
	else if( document.readyState == 'complete' )
	{
		onVLCPluginReady();
	}
	else
	{
		/* Explorer loads plugins asynchronously */
		document.onreadystatechange=function()
		{
			if( document.readyState == 'complete' )
			{
				onVLCPluginReady();
			}
		}
	}
	if (!getVlcVersion()) doGo("rtsp://10.0.0.2/stream"+(stream_id+1));
	}
	else
	{
	if (window.ActiveXObject) {
		try {
			var activeX = $("AmbaIPCmrWebPlugIn1");
			if (activeX.SetRecvType(recvType)) {
				activeX.SetHostname(hostname);
				activeX.SetStreamId(stream_id);
				activeX.SetStatWindowSize(statSize);
				activeX.ShowStat(showStat);
				if (!activeX.EnableDPTZ()) {
					if ($("DPTZ")) {
						$("DPTZ").style.display = "none";
					}
				} else {
					activeX.ShowDPTZ(showStat);
				}
			} else {
				alert("set wrong client type!");
				return;
			}
		} catch (e) {
			alert(e);
			window.open ('/cgi-bin/webdemo.cgi?page=help','','height=800,width=640,top=0,menubar=no,scrollbars=yes, resizable=yes,status=no');
			return;
		}
	}
	PlayVideo(stream_id);
	}
}
function getVLC(name)
{
	if (window.document[name])
	{
		return window.document[name];
	}
	if (navigator.appName.indexOf("Microsoft Internet")==-1)
	{
		if (document.embeds && document.embeds[name])
			return document.embeds[name];
	}
	else // if (navigator.appName.indexOf("Microsoft Internet")!=-1)
	{
		return document.getElementById(name);
	}
}
// VLC Plugin
function onVLCPluginReady() {
	//registerVLCEvent("MediaPlayerPlaying", handle_MediaPlayerPlaying);
}
/* actions */
function doGo(targetURL) {
	var vlc = getVLC("vlc");

	if( vlc )
	{
		vlc.playlist.items.clear();
		while( vlc.playlist.items.count > 0 )
		{
			// clear() may return before the playlist has actually been cleared
			// just wait for it to finish its job
		}
		var options = [":rtsp-tcp"];
		var itemId = vlc.playlist.add(targetURL,"",options);
		options = [];
		if( itemId != -1 )
		{
			// play MRL
			vlc.playlist.playItem(itemId);
		}
		else
		{
			alert("cannot play at the moment !");
		}
		doItemCount();
	}
}
function FlySet(streamId, op)
{
	try {
		var STREAM_ID_OFFSET = 28;
		var url = "/cgi-bin/webdemo.cgi?streamId=" + streamId;
		var ai = new AJAXInteraction(url, display);
		var req, info, data = 0;
		var value;
		switch (op) {
		case 'Play' :
			req = 'REQ_STREAM_START';
			info = (streamId << STREAM_ID_OFFSET) + FlySetOps.Play;
			break;
		case 'Stop' :
			req = 'REQ_STREAM_STOP'
			info = (streamId << STREAM_ID_OFFSET) + FlySetOps.Stop;
			break;
		case 'ForceIdr':
			req = 'REQ_SET_FORCEIDR';
			info = (streamId << STREAM_ID_OFFSET) + FlySetOps.ForceIdr;
			break;
		case 'ChangeFr':
			if (event.keyCode != 0 && event.keyCode != 13) {
				return;
			}
			req = 'REQ_CHANGE_FR';
			value = $(op).value;
			if (value == 0) {
				alert("invalid value");
				return;
			}
			info = (streamId << STREAM_ID_OFFSET) + FlySetOps.ChangeFr;
			data = parseInt(value, 10);
			break;
		case 'ChangeBRC':
			if (event.keyCode != 0 && event.keyCode != 13) {
				return;
			}
			req = 'REQ_CHANGE_BRC';
			value = $(op).value;
			info = (streamId << STREAM_ID_OFFSET) + FlySetOps.ChangeBRC;
			data = parseInt(value, 10);
			break;
		case 'ChangeCBRAvgBps':
			if (event.keyCode != 0 && event.keyCode != 13) {
				return;
			}
			req = 'REQ_CHANGE_BR';
			value = $(op).value;
			if (value == 0) {
				alert("invalid value for CBR Avg Bitrate");
				return;
			}
			info = (streamId << STREAM_ID_OFFSET) + FlySetOps.ChangeCBRAvgBps;
			data = parseInt(value, 10) * 1000;
			break;
		case 'ChangeVBRMinBps':
		case 'ChangeVBRMaxBps':
			if (event.keyCode != 0 && event.keyCode != 13) {
				return;
			}
			req = 'REQ_CHANGE_BR';
			value = $('ChangeVBRMinBps').value;
			if (value == 0) {
				alert("invalid value for VBR Min Bitrate");
				return;
			}
			info = (streamId << STREAM_ID_OFFSET) + FlySetOps.ChangeVBRMaxBps;
			value = $('ChangeVBRMaxBps').value;
			if (value == 0) {
				alert("invalid value for VBR Max Bitrate");
				return;
			}
			data = parseInt(value, 10) * 1000;
			break;
		default :
			break;
		}
		var postData = "req_cnt=1&req0=" + req + "&info0=" + info + "&data0=" + data;
		ai.doPost(postData);
	} catch (error) {
		alert(error);
	}
	function display(response_text) {
		var ret = response_text.substring(0, 1);
		if (ret == '0') {
			alert("setting " + op + " succeeded");
		} else if (ret == '1') {
			alert("setting " + op + " failed");
		} else {
			alert("unexpected error");
		}
	}
}

function ApplyFlySetting(streamId)
{
	try {
		var i;
		var new_LiveParam = new LiveParam();
		get_page_data(new_LiveParam);
		var LivePack = new Pack();
		for (i = 0; i < new_LiveParam.count; i++) {
			if (new_LiveParam.mem[i][1] != g_LiveParam.mem[i][0]) {
				add_to_pack(LivePack, new_LiveParam.mem[i][0], new_LiveParam.mem[i][1]);
			}
		}
		if (LivePack.mem == '') {
			alert("Nothing changed");
			return;
		}
		showStatus("Apply fly settings ....");
		var url = "/cgi-bin/webdemo.cgi?live_streamId=" + streamId;
		var ai = new AJAXInteraction(url, display);
		var postData = '';
		if (LivePack.mem != '') {
			postData = "req_cnt=0&sec0=LIVE&data0=" + encodeURIComponent(LivePack.mem);
			ai.doPost(postData);
		}
	} catch (error) {
		alert(error);
	}
	function display(response_text) {
		var ret = response_text.substring(0, 1);
		hideStatus();
		if (ret == '0') {
			alert(response_text.substring(2));
			g_LiveParam = new_LiveParam;
		} else if (ret == '1') {
			alert(response_text.substring(2));
			set_page_data(g_LiveParam);
		} else {
			alert("unexpected error");
		}
	}
}
function PlayVideo()
{
	var browser = browserInfo();
	if (browser.name != "IE") {
	var vlc = getVLC("vlc");
			if( vlc ){
				if( vlc.playlist.items.count > 0 ){
					vlc.playlist.play();
				}
				else{
				alert('nothing to play !');
				}
			}
	}
	else if (window.ActiveXObject) {
		try {
			$("AmbaIPCmrWebPlugIn1").Play();
		} catch (e) {
			alert(e);
		}
	}
}
function StopVideo()
{
	var browser = browserInfo();
	if (browser.name != "IE") {
		var vlc = getVLC("vlc");
			if( vlc )    vlc.playlist.stop();
	}
	else if (window.ActiveXObject) {
		try {
			$("AmbaIPCmrWebPlugIn1").Stop();
		} catch (e) {
			alert(e);
		}
	}
}
function Record()
{
	if (window.ActiveXObject) {
		try {
			var obj = $("AmbaIPCmrWebPlugIn1");
			var record_status = obj.GetRecordStatus();
			var record = $("Record");
			if (record_status == false ) {
				obj.Record(true);
				record.value = "Stop Rec";
			} else {
				obj.Record(false);
				record.value = "Start Rec";
			}
		} catch (e) {
			alert(e);
		}
	}
}
function getVlcVersion() {
	if (navigator.plugins && navigator.plugins.length) {
		for (var x = 0; x < navigator.plugins.length; x++) {
			if (navigator.plugins[x].name.indexOf("VLC")!= -1) {
				for (var ver = highestVer;ver > -1; ver--){
				if (navigator.plugins[x].description.indexOf(" "+ver+".") != -1) {
					var i = navigator.plugins[x].description.indexOf(ver+".");
					if (navigator.plugins[x].description.split("")[i] == ver)
						version[0] = ver;
					if (navigator.plugins[x].description.split("")[i+1] == ".")
						version[1] = parseFloat(navigator.plugins[x].description.split("")[i+2]);
					if (navigator.plugins[x].description.split("")[i+3] == ".")
						version[2] = parseFloat(navigator.plugins[x].description.split("")[i+4]);
					if (navigator.plugins[x].description.split("")[i+5] == ".")
						version[3] = parseFloat(navigator.plugins[x].description.split("")[i+6]);
					else version[3] = 0;
					break;
				 }
				}
				installed = true;
				break;
			}
		}
	}
	judgeVlc();
	return false;
}
function judgeVlc() {
	if(installed){
		if (version[0] < 2 )
			if(get_cookie('popped')==''){
				if(confirm("Vlc version is too low to play video stablely, you might want to update to latest version. Note:with Mozilla plugin.")){
					 window.open("http://www.videolan.org/vlc/","","");
				}
				document.cookie="popped=yes";
			}
	}
	else{
		if(confirm("Please install vlc(with Mozilla plugin) first.")){
			window.open("http://www.videolan.org/vlc/","","");
		}
	}
}
function get_cookie(Name) {
	var search = Name + "=";
	var returnvalue = "";
	if (document.cookie.length > 0) {
		offset = document.cookie.indexOf(search)
		if (offset != -1) {
			offset += search.length
			end = document.cookie.indexOf(";", offset);
			if (end == -1)	end = document.cookie.length;
		returnvalue=unescape(document.cookie.substring(offset, end))
		}
	}
	return returnvalue;
}
