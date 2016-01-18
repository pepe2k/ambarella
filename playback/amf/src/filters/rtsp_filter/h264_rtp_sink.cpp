/*
 * h264_rtp_sink.cpp
 *
 * History:
 *    2011/8/31 - [Jay Zhang] created file
 *
 * Copyright (C) 2007-2011, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

 #include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "am_types.h"

#include "helper.h"
#include "h264_rtp_sink.h"

static int findNalu_amba (NALU * nalus, AM_UINT *count, AM_U8 *bitstream, AM_UINT streamSize)
{
	int index = -1;
	AM_U8 * bs = bitstream;
	AM_UINT head;
	AM_U8 nalu_type;
	AM_U8 *last_byte_bitstream = bitstream + streamSize - 1;

	while (bs <= last_byte_bitstream) {
		
		head = (bs[3] << 24) | (bs[2] << 16) | (bs[1] << 8) | bs[0];
	//	printf("head 0x%x: 0x%x, 0x%x, 0x%x, 0x%x\n", head, bs[0], bs[1], bs[2], bs[3]);

		if (head == 0x01000000) {	// little ending
			index++;
			// we find a nalu
			bs += 4;		// jump to nalu type
			nalu_type = 0x1F & bs[0];
			nalus[index].nalu_type = nalu_type;
			nalus[index].addr = bs;

			if (index  > 0) {	// Not the first NALU in this stream
				nalus[index -1].size = nalus[index].addr - nalus[index -1].addr - 4; // cut off 4 bytes of delimiter
			}
			
			if (nalu_type == NALU_TYPE_NONIDR || nalu_type == NALU_TYPE_IDR) {
				// Calculate the size of the last NALU in this stream
				nalus[index].size =  last_byte_bitstream - bs + 1;
				break;
			}
		}
		else if (bs[3] != 0) {
			bs += 4;
		} else if (bs[2] != 0) {
			bs += 3;
		} else if (bs[1] != 0) {
			bs += 2;
		} else {
			bs += 1;
		}
	}

	*count = index + 1;
	if (*count == 0) {
		printf("No nalu found in the bitstream!\n");
		return -1;
	}
	return 0;
}

CamH264RtpSink *CamH264RtpSink::Create()
{
	CamH264RtpSink *result = new CamH264RtpSink();
	if (result != NULL && result->Construct() != ME_OK) {
		delete result;
		result = 0;
	}
	return result;
}

AM_ERR CamH264RtpSink::Construct()
{
	if (inherited::Construct("H264") != ME_OK)
	        return ME_ERROR;
	
	return ME_OK;
}

void CamH264RtpSink::FeedStreamData(AM_U8 *inputBuf, AM_U32 inputBufSize)
{
	_pCurrBuf = inputBuf;
	_currBufSize = inputBufSize;
	findNalu_amba(_nalus, &_naluCnt, inputBuf, inputBufSize);
	_currNaluIndex = 0;
//	printf("_naluCnt %d, inputBufSize %d\n", _naluCnt, inputBufSize);
}

AM_BOOL CamH264RtpSink::GetOneFrame(AM_U8 **ppFrameStart, AM_U32 *pFrameSize, AM_BOOL *pLastFragment)
{
    static AM_U8 *pCurrFragmStart;
    const AM_U32 maxPacketSize = 1448-12;	// max packet size - RTP hdr size
    static AM_U32 remainDataBytes = 0;
    static AM_U8 FU_indicator, FU_header;
    static AM_U8 firstFragBuf[maxPacketSize];
    static AM_BOOL firstFragment = AM_TRUE;
    static AM_BOOL HAS_AUD = AM_FALSE;

     *pLastFragment = AM_TRUE;		// by default

     if (remainDataBytes == 0) {
        // We have no NAL unit data currently in the buffer.  Read a new one:
        if (_currNaluIndex >= _naluCnt) {	// there's no data, return false
            return AM_FALSE;
        } else {
            while(_nalus[_currNaluIndex].nalu_type == 6) {		//filter out the SEI nalu
                AM_ASSERT(_currNaluIndex <= _naluCnt);
                _currNaluIndex++;
            }

            if (!HAS_AUD && _nalus[_currNaluIndex].nalu_type == 9)      // check if has au delimeter
                HAS_AUD = AM_TRUE;

            if ((HAS_AUD && _nalus[_currNaluIndex].nalu_type == 9) ||
            (!HAS_AUD && (_nalus[_currNaluIndex].nalu_type == 7 || _nalus[_currNaluIndex].nalu_type == 1))) {
                //AUD or sps or P slice
                _timeStamp += 3003;
            }

//          printf("type %d, size %d\n", _nalus[_currNaluIndex].nalu_type, _nalus[_currNaluIndex].size);

            pCurrFragmStart = _nalus[_currNaluIndex].addr;
            remainDataBytes = _nalus[_currNaluIndex].size;
            _currNaluIndex++;
        }
    //		 printf("	Fragmenter: read new NAL unit\n");		//jay
        firstFragment = AM_TRUE;
    }
    // We have NAL unit data in the buffer.  There are three cases to consider:
    // 1. There is a new NAL unit in the buffer, and it's small enough to deliver
    //	  to the RTP sink (as is).
    // 2. There is a new NAL unit in the buffer, but it's too large to deliver to
    //	  the RTP sink in its entirety.  Deliver the first fragment of this data,
    //	  as a FU-A packet, with one extra preceding header byte.
    // 3. There is a NAL unit in the buffer, and we've already delivered some
    //	  fragment(s) of this.	Deliver the next fragment of this data,
    //	  as a FU-A packet, with two extra preceding header bytes.

    //	printf("						  Fragmenter: deliver fragment to RTP sink\n"); 	//jay


    //	printf("		  remainDataBytes %d, fMaxSize %d\n", remainDataBytes, fMaxSize); //jay
    if (firstFragment) { // case 1 or 2	first fragment of a NALU
        if (remainDataBytes <= maxPacketSize) { // case 1
    //			printf("		  Fragmenter: small nalu\n");		//jay
            *ppFrameStart = pCurrFragmStart;
            *pFrameSize = remainDataBytes;
            remainDataBytes = 0;
        } else { // case 2

            // We need to send the NAL unit data as FU-A packets.  Deliver the first
            // packet now.	Note that we add FU indicator and FU header bytes to the front
            // of the packet (reusing the existing NAL header byte for the FU header).

            //			printf("		  Fragmenter: first fragment\n");		//jay
            // NAL unit syntax: first byte contains nal_unit_type	(h264 standard 7.4.1)
            FU_indicator = (pCurrFragmStart[0] & 0xE0) | 28; // FU indicator (28 means FU-A)
            FU_header = 0x80 | (pCurrFragmStart[0] & 0x1F); // FU header (with S bit)

            firstFragBuf[0] = FU_indicator;
            firstFragBuf[1] = FU_header;
            memcpy(firstFragBuf + 2, pCurrFragmStart + 1, maxPacketSize - 2);

            *ppFrameStart = firstFragBuf;
            *pFrameSize = maxPacketSize;

            pCurrFragmStart += (maxPacketSize - 1);
            remainDataBytes -= (maxPacketSize - 1);
            *pLastFragment = AM_FALSE;
            FU_header &= ~0x80;	// FU header (clear S bit)
            firstFragment = AM_FALSE;
        }
    } else { // case 3

        // We are sending this NAL unit data as FU-A packets.  We've already sent the
        // first packet (fragment).  Now, send the next fragment.  Note that we add
        // FU indicator and FU header bytes to the front.	(We reuse these bytes that
        // we already sent for the first fragment, but clear the S bit, and add the E
        // bit if this is the last fragment.)

    //		printf("		  Fragmenter: next fragment\n");		//jay
        AM_U32 numBytesToSend = 2 + remainDataBytes;	// 2 bytes for FU indicator and FU header
        if (numBytesToSend > maxPacketSize) {
            // We can't send all of the remaining data this time:
            numBytesToSend = maxPacketSize;
            *pLastFragment = AM_FALSE;
        } else {
    //			printf("		  Fragmenter: last fragment\n");		//jay
            // This is the last fragment:
            FU_header |= 0x40; // set the E bit in the FU header
        }
        *(pCurrFragmStart - 2) = FU_indicator; // FU indicator
        *(pCurrFragmStart - 1) = FU_header; // FU header (no S bit)

        *ppFrameStart = pCurrFragmStart - 2;	// 2 bytes for FU indicator and FU header
        *pFrameSize = numBytesToSend;
        pCurrFragmStart += (numBytesToSend - 2);
        remainDataBytes -= (numBytesToSend - 2);
    }

    return AM_TRUE;
}

// make sure outBuf is large enough to accomendate the frame
AM_U32 CamH264RtpSink::DoSpecialFrameHandling(AM_U8 *outBuf, AM_U8 *pFrameStart,
		AM_U32 frameSize, AM_BOOL lastFragment)
{
	int header_size = 12;		// rtp header

	AM_ASSERT(outBuf != NULL && pFrameStart != NULL);

	// Set up the RTP header (12 bytes):
	AM_U32 rtpHdr = 0x80000000; // RTP version 2
	int fRTPPayloadType = 96;

	rtpHdr |= (fRTPPayloadType<<16);
	rtpHdr |= _seqNo; // sequence number
	if (lastFragment)
		rtpHdr |= 0x00800000;	//set Marker Bit
	*((AM_U32 *)&outBuf[0]) = htonl(rtpHdr);
	*((AM_U32 *)&outBuf[4]) = htonl(_timeStamp);
	*((AM_U32 *)&outBuf[8]) = htonl(_ssrc);
//	printf("rtpHdr: 0x%x, 0x%x, 0x%x, 0x%x,\n", _outBuf[0], _outBuf[1], _outBuf[2], _outBuf[3]);

	memcpy(&outBuf[header_size], pFrameStart, frameSize);

//	_timeStamp += 3003;
	_seqNo++;
	return frameSize + header_size;
}

CAutoString CamH264RtpSink::GetAuxSDPLine() const
{
	return CAutoString();
}

