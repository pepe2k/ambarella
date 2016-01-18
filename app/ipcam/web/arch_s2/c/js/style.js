/////////////////////////////////////////////////////////////////////
//
// style.js
//
// History:
//	2010/07/11 - [Jian Tang] Created this file
//
// Copyright (C) 2007 - 2010, Ambarella, Inc.
//
// All rights reserved. No Part of this file may be reproduced, stored
// in a retrieval system, or transmitted, in any form, or by any means,
// electronic, mechanical, photocopying, recording, or otherwise,
// without the prior consent of Ambarella, Inc.
//
/////////////////////////////////////////////////////////////////////

var oevent = new Function('e', 'if (!e) e = window.event;return e');

function checknowleft(z, width)
{
	return ((z<0) ? 0 : ((z>width) ? width : Math.round(z)));
}

function checknowleftpercent(z, widthInPixel, widthInPercent, minPos)
{
	return Math.round(z * widthInPercent / widthInPixel + minPos);
}

function enableItem(name, enableString)
{
	var item = $(name);
	item.disabled = enableString;
}

function AddSubOneStep(tag, outBox, width, minPos, addSub)
{
	var z, newPos, newPercent;
	var SLIDERWIDTH = 350;
	var obj = $(tag);
	var temp = obj.style.left.replace("px", "") * 1;

	if (addSub)
		newPos = temp - SLIDERWIDTH / width;
	else
		newPos = temp + SLIDERWIDTH / width;

	z = checknowleft(newPos, SLIDERWIDTH);
	newPercent = checknowleftpercent(z, SLIDERWIDTH, width, minPos);
	document.onmouseup = function(e) {
		obj.style.left = z + "px";
		$(outBox).value = newPercent;
	}
}

function MoveBar(tag, outBox, width, minPos)
{
	var x, y, z, drag_ = false;
	var nowleft, nowleftpercent;
	var SLIDERWIDTH = 350;
	var obj = $(tag);

	obj.onmousedown = function(e) {
		drag_ = true;
		with(this) {
			var temp0 = style.left.replace("px", "") * 1;
			x = oevent(e).clientX;
			y = oevent(e).clientY;

			document.onmousemove = function(e) {
				if (!drag_)
					return false;
				with(this) {
					z = temp0 + oevent(e).clientX - x;
					if (z < 0) {
						z = 0;
						drag_ = false;
					}
					if (z > SLIDERWIDTH) {
						z = SLIDERWIDTH;
						drag_ = false;
					}
					style.left = z + "px";
				}
			}

			document.onmouseup = function(e) {
				drag_ = false;
				nowleft = checknowleft(z, SLIDERWIDTH);
				nowleftpercent = checknowleftpercent(nowleft, SLIDERWIDTH, width, minPos);
				style.left = nowleft + "px";
				$(outBox).value = nowleftpercent;
			}
		}
	}
}


function expand(ID, maxH)
{
	var d = $(ID);
	var h = d.offsetHeight;

	iIntervalId = setInterval(doExpand, 2);

	function doExpand() {
		h += 50;
		d.style.display = 'block';
		if (h > maxH) {
			d.style.height = maxH + 'px';
			clearInterval(iIntervalId);
		} else {
			d.style.height = h + 'px';
		}
	}
}

function fold(ID, maxH)
{
	var d = $(ID);
	var h = d.offsetHeight;

	iIntervalId = setInterval(doFold, 2);

	function doFold() {
		h -= 50;
		if (h < 0) {
			d.style.height = '0px';
			d.style.display = 'none';
			clearInterval(iIntervalId);
		} else {
			d.style.height = h + 'px';
		}
	}
}

function expandEncodeFormat(headerID, contentID, maxH, streamID)
{
	var hd = $(headerID);
	var d = $(contentID);
	var imgStr;
	if (d.style.display == 'block') {
		fold(contentID, maxH);
		imgStr = "<img src='../img/expand.gif' />";
	} else {
		getData('stream', streamID);
		expand(contentID, maxH);
		imgStr = "<img src='../img/fold.gif' />";
	}
	hd.innerHTML = imgStr;
}

function expandOSDText(headerID, contentID, maxH, streamID)
{
	var hd = $(headerID);
	var d = $(contentID);
	var imgStr;
	if (d.style.display == 'block') {
		fold(contentID, maxH);
		imgStr = "<img src='../img/expand.gif' />";
	} else {
		expand(contentID, maxH);
		imgStr = "<img src='../img/fold.gif' />";
	}
	hd.innerHTML = imgStr;
}

function removeOptions(selectObj) {
	var len = selectObj.options.length;
	for (var i = 0; i < len; ++i) {
		selectObj.options[0] = null;
	}
}

function setSelectOption(selectObj, optionList) {
	if (typeof selectObj != 'object') {
		selectObj = $(selectObj);
	}
	removeOptions(selectObj);
	var start = 0;
	var len = optionList.length;
	for (var i = 0; i < len; ++i) {
		selectObj.options[start] = new Option(optionList[i].txt, optionList[i].val);
		++start;
	}
	selectObj.options[0].selected = true;
}

function setVoutMode(voutType)
{
	var voutRes = [];
	var voutModeObj = $('vout_mode');
	var off = 0;
	var cvbs = ((0 << 16) | (1 << 0));
	var hdmi = ((0 << 16) | (1 << 2));
	voutRes[off] = [
		{txt: '480I', val: 0xFFF0},
		{txt: '576I', val: 0xFFF1}
	];
	voutRes[cvbs] = [
		{txt: '480I', val: 0xFFF0},
		{txt: '576I', val: 0xFFF1}
	];
	voutRes[hdmi] = [
		{txt: '480P', val: 0xFFF2},
		{txt: '576P', val: 0xFFF3},
		{txt: '720P', val: 0xFFF4},
		{txt: '1080I', val: 0xFFF6},
		{txt: '1080P30', val: 0xFFFD}
	];

	setSelectOption(voutModeObj, voutRes[voutType]);
	if (voutType == off)
		voutModeObj.disabled = 'disabled';
	else
		voutModeObj.disabled = null;
}

function setEncRes(streamId, encMode)
{
	var encRes = [];
	var defaultSelected = [];
	var NORMAL_MODE = 0, HIGH_MEGA_MODE = 1;
	encRes[NORMAL_MODE] = [
		{txt: '1920 x1080', val: createRes(1920, 1080)},
		{txt: '1440 x1080', val: createRes(1440, 1080)},
		{txt: '1280 x1024', val: createRes(1280, 1024)},
		{txt: '1280 x 960', val: createRes(1280, 960)},
		{txt: '1280 x 720', val: createRes(1280, 720)},
		{txt: ' 800 x 600', val: createRes(800, 600)},
		{txt: ' 720 x 576', val: createRes(720, 576)},
		{txt: ' 720 x 480', val: createRes(720, 480)},
		{txt: ' 640 x 480', val: createRes(640, 480)},
		{txt: ' 352 x 288', val: createRes(352, 288)},
		{txt: ' 352 x 240', val: createRes(352, 240)},
		{txt: ' 320 x 240', val: createRes(320, 240)},
		{txt: ' 176 x 144', val: createRes(176, 144)},
		{txt: ' 176 x 120', val: createRes(176, 120)},
		{txt: ' 160 x 120', val: createRes(160, 120)}
	];
	encRes[HIGH_MEGA_MODE] = [
		{txt: '2592x1944 (5.0M)', val: createRes(2592, 1944)},
		{txt: '2560x1440 (3.7M)', val: createRes(2560, 1440)},
		{txt: '2304x1296 (3.0M)', val: createRes(2304, 1296)},
		{txt: '2048x1536 (3.0M)', val: createRes(2048, 1536)}
	];
	// default resolution : 720p for normal mode, and 2.7M for high MP mode
	defaultSelected = [4, 3];
	var encObj = $('s' + streamId + '_resolution');

	setSelectOption(encObj, encRes[encMode]);
	encObj.options[defaultSelected[encMode]].selected = true;

	return ;

	function createRes(width, height) {
		return (width << 16) + height;
	}
}

function enableStreamFormat(streamId, enabled)
{
	var str = enabled ? null : 'disabled';
	var item, pre, name;

	pre = 's' + streamId + '_';

	name = pre + 'type';
	item = $(name);
	item.value = 0;
	enableItem(name, str);

	name = pre + 'enc_fps';
	enableItem(name, str);

	name = pre + 'dptz';
	enableItem(name, str);

	name = pre + 'resolution';
	enableItem(name, str);

	name = pre + 'flip_rotate';
	enableItem(name, str);

	name = pre + 'M';
	enableItem(name, str);

	name = pre + 'N';
	enableItem(name, str);

	name = pre + 'idr_interval';
	enableItem(name, str);

	name = pre + 'gop_model';
	enableItem(name, str);

	name = pre + 'profile';
	enableItem(name, str);

	name = pre + 'brc';
	enableItem(name, str);

	name = pre + 'cbr_avg_bps';
	enableItem(name, str);

	name = pre + 'vbr_min_bps';
	enableItem(name, str);

	name = pre + 'vbr_max_bps';
	enableItem(name, str);

	name = pre + 'quality';
	enableItem(name, str);

	return ;
}

function setEncodeMode(encMode)
{
	var mode = parseInt(encMode);
	var i, MAX_STREAM_NUM = 4;
	var dptz = $('s0_dptz'), brc;

	switch (mode) {
	case 0:		// high frame rate mode
		for (i = 1; i < MAX_STREAM_NUM; ++i) {
			enableStreamFormat(i, 1);
			brc = $('s'+i+'_brc');
			setBRCMode(brc.value, i);
		}
		dptz.disabled = null;
		setEncRes(0, 0);
		break;
	case 1:		// high mega pixel mode
		for (i = 1; i < MAX_STREAM_NUM; ++i) {
			enableStreamFormat(i, 0);
		}
		setEncRes(0, 1);
		break;
	case 2:		// low delay mode
		for (i = 1; i < MAX_STREAM_NUM; ++i) {
			enableStreamFormat(i, 0);
		}
		dptz.disabled = 'disabled';
		setEncRes(0, 0);
		break;
	default:
		alert('Invalid encode mode!\n');
		break;
	}
}

function setDPTZMode(encRes, stream)
{
	var MAX_RES = (720 << 16) + 576;
	var res;
	var dptz = $('s'+stream+'_dptz');

	res = parseInt(encRes);
	if (parseInt(stream) > 0) {
		if (res > MAX_RES) {
			dptz.disabled = 'disabled';
		} else {
			dptz.disabled = null;
		}
	}
}

function setBRCMode(brcMode, stream)
{
	var vbrMinBpsInput = $('s'+stream+'_vbr_min_bps');
	var vbrMaxBpsInput = $('s'+stream+'_vbr_max_bps');
	var cbrAvgBpsInput = $('s'+stream+'_cbr_avg_bps');

	vbrMaxBpsInput.disabled = null;
	vbrMinBpsInput.disabled = null;
	cbrAvgBpsInput.disabled = null;
	switch (brcMode) {
	case '0':		// cbr
	case '2':		// cbr with dropping frame
		vbrMaxBpsInput.disabled = 'disabled';
		vbrMinBpsInput.disabled = 'disabled';
		break;
	case '1':		// vbr
	case '3':		// vbr with dropping frames
		cbrAvgBpsInput.disabled = 'disabled';
		break;
	default:		// error
		alert("Unknown bitrate control mode : " + brcMode);
		break;
	}
}

function enableOSDText(stream, enabled)
{
	var str = enabled ? null : 'disabled';
	var pre, name;

	pre = 's' + stream + '_';

	name = pre + 'text';
	enableItem(name, str);

	name = pre + 'text_size';
	enableItem(name, str);

	name = pre + 'text_color';
	enableItem(name, str);

	name = pre + 'text_outline';
	enableItem(name, str);

	name = pre + 'text_bold';
	enableItem(name, str);

	name = pre + 'text_italic';
	enableItem(name, str);

	name = pre + 'text_startx';
	enableItem(name, str);

	name = pre + 'text_starty';
	enableItem(name, str);

	name = pre + 'text_boxw';
	enableItem(name, str);

	name = pre + 'text_boxh';
	enableItem(name, str);

	return ;
}

function addOSD(type, stream)
{
	var prefix = 's' + stream + '_';
	var bmp_enable = $(prefix + 'bmp_enable');
	var time_enable = $(prefix + 'time_enable');
	var text_enable = $(prefix + 'text_enable');
	var norotate = $(prefix + 'no_rotate');
	var enable = 0;

	switch (type) {
	//case 'no_rotate':
		//break;
	case 'bmp':
		break;
	case 'time':
		break;
	case 'text':
		if (text_enable.checked == true) {
			enable = 1;
		}
		enableOSDText(stream, enable);
		break;
	default:
		alert("Unknown OSD type : "+type);
		break;
	}
}

function setIRISDuty(dutyMode)
{
	var dutyInput = $('dc_iris_duty');

	if (dutyMode == '0')
		dutyInput.disabled = 'disabled';
	else
		dutyInput.disabled = null;
}

function setShutterLimit(shutterMin)
{
	var shutterMaxExtent = [];
	var shutterMinList = $('shutter_min');
	var shutterMaxList = $('shutter_max');
	var shutterMax, len, start, i;
	shutterMaxExtent = [
		{txt: '1/15', val: 34133333},
		{txt: '1/7.5', val: 68266667}
	];

	len = shutterMaxList.length;
	for (i = 0; i < len; ++i) {
		if (shutterMaxList.options[i].selected == true) {
			shutterMax = shutterMaxList.options[i].value;
			break;
		}
	}
	if (parseInt(shutterMax) < parseInt(shutterMin))
		shutterMax = shutterMin;
	removeOptions(shutterMaxList);

	start = 0;
	len = shutterMinList.length;
	for (i = 0; i < len; ++i) {
		if (parseInt(shutterMinList.options[i].value) >= parseInt(shutterMin)) {
			shutterMaxList.options[start] = new Option(shutterMinList.options[i].text, shutterMinList.options[i].value);
			if (shutterMaxList.options[start].value == parseInt(shutterMax)) {
				shutterMaxList.options[start].selected = true;
			}
			++start;
		}
	}

	len = shutterMaxExtent.length;
	for (i = 0; i < len; ++i) {
		shutterMaxList.options[start] = new Option(shutterMaxExtent[i].txt, shutterMaxExtent[i].val);
		++start;
	}
}


