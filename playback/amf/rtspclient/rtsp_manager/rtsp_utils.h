#ifndef RTSP_UTILS_H_
#define RTSP_UTILS_H_

#if NEW_RTSP_CLIENT

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
extern "C"{
#include "libavformat/avformat.h"
#include "libavformat/rtpdec.h"
#include "libavutil/opt.h"
};
#include "get_h264_info.h"

/**
 * Packet profile of the data that we will be receiving. Real servers
 * commonly send RDT (although they can sometimes send RTP as well),
 * whereas most others will send RTP.
 */
enum RTSPTransport {
    RTSP_TRANSPORT_RTP, /**< Standards-compliant RTP */
    RTSP_TRANSPORT_RDT, /**< Realmedia Data Transport */
    RTSP_TRANSPORT_NB
};

/**
 * Transport mode for the RTSP data. This may be plain, or
 * tunneled, which is done over HTTP.
 */

#define RTSP_DEFAULT_NB_AUDIO_CHANNELS 1
#define RTSP_DEFAULT_AUDIO_SAMPLERATE 44100
#define RTSP_RTP_PORT_MIN 5000
#define RTSP_RTP_PORT_MAX 65000

/**
 * Identify particular servers that require special handling, such as
 * standards-incompliant "Transport:" lines in the SETUP request.
 */
enum RTSPServerType {
    RTSP_SERVER_RTP,  /**< Standards-compliant RTP-server */
    RTSP_SERVER_REAL, /**< Realmedia-style server */
    RTSP_SERVER_WMS,  /**< Windows Media server */
    RTSP_SERVER_NB
};

/**
 * Private data for the RTSP demuxer.
 *
 * @todo Use AVIOContext instead of URLContext
 */
typedef struct RTSPState {
    /** number of items in the 'rtsp_streams' variable */
    int nb_rtsp_streams;

    struct RTSPStream **rtsp_streams; /**< streams in this session */

    /** the seek value requested when calling av_seek_frame(). This value
     * is subsequently used as part of the "Range" parameter when emitting
     * the RTSP PLAY command. If we are currently playing, this command is
     * called instantly. If we are currently paused, this command is called
     * whenever we resume playback. Either way, the value is only used once,
     * see rtsp_read_play() and rtsp_read_seek(). */
    int64_t seek_timestamp;

    /** brand of server that we're talking to; e.g. WMS, REAL or other.
     * Detected based on the value of RTSPMessageHeader->server or the presence
     * of RTSPMessageHeader->real_challenge */
    enum RTSPServerType server_type;


    /** some MS RTSP streams contain a URL in the SDP that we need to use
     * for all subsequent RTSP requests, rather than the input URI; in
     * other cases, this is a copy of AVFormatContext->filename. */
    char control_uri[1024];

    /* Number of RTCP BYE packets the RTSP session has received.
     * An EOF is propagated back if nb_byes == nb_streams.
     * This is reset after a seek. */
    int nb_byes;
} RTSPState;

/**
 * Describes a single stream, as identified by a single m= line block in the
 * SDP content. In the case of RDT, one RTSPStream can represent multiple
 * AVStreams. In this case, each AVStream in this set has similar content
 * (but different codec/bitrate).
 */
typedef struct RTSPStream {
    void *transport_priv;

    /** corresponding stream index, if any. -1 if none (MPEG2TS case) */
    int stream_index;

    /** interleave IDs; copies of RTSPTransportField->interleaved_min/max
     * for the selected transport. Only used for TCP. */
    int interleaved_min, interleaved_max;

    char control_url[1024];   /**< url for this stream (from SDP) */

    /** The following are used only in SDP, not RTSP */
    //@{
    int sdp_port;             /**< port (from SDP content) */
    struct sockaddr_storage sdp_ip; /**< IP address (from SDP content) */
    int sdp_ttl;              /**< IP Time-To-Live (from SDP content) */
    int sdp_payload_type;     /**< payload type */
    //@}

    /** The following are used for dynamic protocols (rtp_*.c/rdt.c) */
    //@{
    /** handler structure */
    RTPDynamicProtocolHandler *dynamic_handler;

    /** private data associated with the dynamic protocol */
    PayloadContext *dynamic_protocol_context;
    //@}
} RTSPStream;


/**
 * Parse a SDP description of streams by populating an RTSPState struct
 * within the AVFormatContext.
 */
int rtsp_sdp_parse(AVFormatContext *s, const char *content);

#endif//NEW_RTSP_CLIENT

#endif /* RTSP_UTILS_H_ */

