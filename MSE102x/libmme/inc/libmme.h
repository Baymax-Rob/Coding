#ifndef libmme_inc_libmme_h
#define libmme_inc_libmme_h
/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
/**
 * \file    application/libmme/inc/libmme.h
 * \brief   structures and function prototypes of libmme
 * \defgroup libmme libmme : MME management library
 *
 */
#ifdef _WIN32
#include <windows.h>
#define ETH_ALEN 6
#define ETH_DATA_LEN 1500
typedef unsigned int uint;
#elif __linux__
#include <linux/if_ether.h>
#else
#error "Unknown platform - Not supported"
#endif

#include "mmtypes.h"
#include "ethernet.h"
#include <time.h>   // for nanosleep
#include <stdint.h>

#define LOG_VERBOSE (2)
#define LOG_DEBUG (1)
#define LOG_INFO (0)


#ifndef __func__
#define __func__ __FUNCTION__
#endif

#define LOG_PRINT(level,  ...) do{ \
                                    if (LOG_VERBOSE == DEBUG_DISPLAY) {\
                                        printf("%s: ", __func__);\
                                    }\
                                    if (level <= DEBUG_DISPLAY)\
                                    {\
                                        printf(__VA_ARGS__);\
                                    }\
                                } while (0)

#define MME_TYPE        0x88e1      /* MME Ethertype (MStar MME Protocol)   */
#define MME_VERSION     0x01
#define MME_HEADER_SIZE 5 /* MMV - MMTYPE - FMI */
#define MME_MAX_SIZE    (ETH_PACKET_MAX_PAYLOAD_SIZE - MME_HEADER_SIZE)        /* maximum size of MME payload */
#define MME_MIN_SIZE    (ETH_PACKET_MIN_PAYLOAD_SIZE - MME_HEADER_SIZE)       /* minimum size of MME payload */
#define MME_TOUT        5           /* communication timeout in seconds */
#define MME_IFACE_LOOPBACK "lo"
#define MME_IFACE_BRIDGE   "eth1"

#define OUI_MSTAR     "\x00\x13\xd7"
#define MSTAR_OUI_SIZE 3

#define SET_MME_PAYLOAD_MAX_SIZE(type, val) val = MMTYPE_IS_VS(type) ? MME_MAX_SIZE - MSTAR_OUI_SIZE : MME_MAX_SIZE
#define SET_MME_PAYLOAD_MIN_SIZE(type, val) val = MMTYPE_IS_VS(type) ? MME_MIN_SIZE - MSTAR_OUI_SIZE : MME_MIN_SIZE
#define SET_MME_HEADER_SIZE(type, val) val = MMTYPE_IS_VS(type) ? sizeof(MME_t) + MSTAR_OUI_SIZE : sizeof(MME_t)

#define MME_TYPE_MASK 0x0003
#define CHARGING_TIME 3000

int DEBUG_DISPLAY;      // flag for dislay debug information

extern const unsigned char mac_broadcast[ETH_ALEN];
extern const unsigned char mac_link_local[ETH_ALEN];
extern unsigned char other_mac_link_local[ETH_ALEN];
extern const int attenuation_group;

#pragma pack(1)
typedef struct
{
    unsigned char   mme_dest[ETH_ALEN];     /* Destination node                         */
    unsigned char   mme_src[ETH_ALEN];      /* Source node                              */
    /* unsigned int        vlan_tag; */     /* ieee 802.1q VLAN tag (optional) [0x8100] */
    unsigned short      mtype;              /* 0x88e1 (iee assigned Ethertype)          */
    unsigned char       mmv;                /* Management Message Version               */
    unsigned short      mmtype;             /* Management Message Type                  */
    unsigned short      fmi;                /* Fragmentation Management Info            */
    //unsigned char       buffer[41];
} MME_t;
#pragma pack()

typedef struct
{
    unsigned char       mme_dest[ETH_ALEN]; /* Destination node                         */
    unsigned char       mme_src[ETH_ALEN];  /* Source node                              */
    unsigned int        vlan_tag;           /* ieee 802.1q VLAN tag (optional) [0x8100]	*/
    unsigned short      mtype;              /* 0x88e1 (iee assigned Ethertype)          */
    unsigned char       mmv;                /* Management Message Version               */
    unsigned short      mmtype;             /* Management Message Type                  */
    unsigned short      fmi;                /* Fragmentation Management Info            */
} MME_vlan_t;

typedef enum
{
    IFACE_SUCCESS = 0,
    IFACE_FAIL = -1
} iface_error_t;

typedef struct
{
    int fid;
    unsigned char mac[6];
} iface_t;

#ifdef _WIN32
// Interface to send/receive Ethernet to/from on Windows
struct hpav_iface
{
    // Next interface in the list, NULL if last interface in the list
    struct hpav_iface* next;
    // Interface name to be passed to hpav_open_channel
    char* name;
    // If not NULL, human readable description of the interface
    char* description;
    // MAC address of the interface (all zero if it could not be determined)
    unsigned char mac_addr[ETH_MAC_ADDRESS_SIZE];
};
typedef struct hpav_iface hpav_iface_t;

typedef struct pcap pcap_t;

// Descriptor for a channel (open interface)
typedef struct hpav_channel
{
    // PCAP channel
    pcap_t* pcap_chan;
    // MAC address of the interface
    // (all zero if it could not be determined) (copied from hpav_if)
    unsigned char mac_addr[ETH_MAC_ADDRESS_SIZE];
} hpav_channel_t;


typedef int timer_t;

/**
* Get System time (Windows specific).
* \parm  sys_time  Windows system time data structure
*/
int
get_sys_time(struct hpav_sys_time* sys_time);


/**
 * Compute time difference in ms (Windows specific).
 * \parm  p_start_sys_time start system time
 * \parm  p_end_sys_time end system time
 */
int
get_elapsed_time_ms
(struct hpav_sys_time *p_start_sys_time, struct hpav_sys_time *p_end_sys_time);

/**
 * Populate MAC address (Windows specific).
 * \param  interface_list list of network interface
 * \return  0 if success, -1 if interfaces_list is null
 */
int
populate_mac_addr (hpav_iface_t** interfaces_list);

/**
 * Get the list of interfaces compatible with libpcap (Windows specific).
 * \param  interface_list list of network interface
 * \return  0 if success, -1 if failure
 */
int
get_eth_interfaces (hpav_iface_t** interface_list);

/**
 * Free interfaces allocated by get_eth_interfaces (Windows specific).
 * \param  interface_list list of network interface
 */
int
free_eth_interface_list (hpav_iface_t* interface_list);

/**
 * Get interface by specifying interface number (Windows specific).
 * \param  interface_list list of network interface
 * \param  if_num number of network interface
 * \return  network interface that you get
 */
struct hpav_iface *
hpav_get_interface_by_index_num (struct hpav_iface* interfaces, unsigned int if_num);

/**
 * Opens a channel on given interface (Windows specific).
 * \parm  interface to open the network interface you want to open
 * \return  Return NULL in this case to indicate an error occured
 */
hpav_channel_t *
open_channel (hpav_iface_t *interface_to_open);


typedef struct callback_data
{
    unsigned char mac[ETH_MAC_ADDRESS_SIZE];
    unsigned char *data;
    /* Indicates whether packets are received */
    unsigned short int stop;
    unsigned int packet_len;
} callback_data_t;


struct hpav_sys_time {
    // Windows system time data structure
    SYSTEMTIME win_sys_time;
};
#endif //_Win32

/**
 * Send MME to network interface and wait for confirm MME;
 * this is a synchronous transmit and receive transaction
 * (function waits for answer MME before returning).
 * If no answer packet is received before MME_CONFIRM_TIMEOUT,
 * error is returned.<br>
 * As MME are level 2 and IP routing does not apply,
 * the network interface where to send data packet must be selected inside
 * iface paramater. For instance : "eth0", "br0", ...<br>
 * If an MME needs to be fragmented,
 * the sending process is in charge to manage the fragmentation.
 * The same as reassembling the fragmented MME confirm.<br>
 * 3 types of communication can be done :
 * <ul><li>MME_SEND_REQ_CNF : request and confirm transaction
 * <li> MME_SEND_CNF : confirm only (no answer expected)
 * <li>MME_SEND_IND_RSP : indicate and response transaction
 * <li>MME_SEND_IND : indicate only (no answer expected)
 * </ul>
 *
 * \param  ctx MME context to send
 * \param  type transaction type
 * \param  iface selected communication interface; if NULL,
  *            "br0" if remote destination, "lo" if local
 * \param  dest destination MAC address in binary format (6 bytes)
 * \param  confirm_ctx MME context given to put the received MME
 *               answer packet. A adapted buffer must have been provided.
 * \return error type (MME_SUCCESS if success)
 * \return MME_ERROR_NOT_INIT: context not initialized
 */
int
open_interface (const char* iname, void *opened_interface);

/**
 * list of errors returned by libmme functions
 */
typedef enum
{
    MME_SUCCESS = 0,
    /** context not initialized */
    MME_ERROR_NOT_INIT = -1,
    /** not enough available space */
    MME_ERROR_SPACE = -2,
    /** not enough available data */
    MME_ERROR_ENOUGH = -3,
    /** timeout on waiting on response */
    MME_ERROR_TIMEOUT = -4,
    /** socket send/receive error */
    MME_ERROR_TXRX = -5,
    /** general error */
    MME_ERROR_GEN = -6,
    /** not expected result */
    MME_ERROR_RESULT = -7
} mme_error_t;

/**
 * list of link state
 */
 typedef enum
{
    CONNECT_LINK = 1,
    DISCONNECT_LINK = 0,
    ERROR_LINK = -1
} link_state_t;

/**
 * list of loop test state
 */
typedef enum
{
    EXEC_LOOP_TEST = 1,
    NOT_EXEC_LOOP_TEST = 0
} loop_test_state_t;

/**
 * status of MME context for initialization
 */
typedef enum
{
    MME_STATUS_INIT = 0,
    MME_STATUS_OK = 0xabcdefab,
} mme_status_t;

/** MME transaction type */
typedef enum
{
    /** send a .REQ and wait for a .CNF */
    MME_SEND_REQ_CNF = 0,
    /** send a .CNF only */
    MME_SEND_CNF,
    /** send a .IND and wait for a .RSP */
    MME_SEND_IND_RSP,
    /** send a .IND only */
    MME_SEND_IND,
    /** receive a .IND and wait to send a .RSP**/
    MME_RECEIVE_IND_RSP,
    /** receive a .REQ and wait to send a .CNF **/
    MME_RECEIVE_REQ_CNF,
    /** receive a .IND only **/
    MME_RECEIVE_IND,
    /** receive a .RSP only **/
    MME_RECEIVE_RSP,
    /** receive a .CNF only **/
    MME_RECEIVE_CNF
} mme_send_type_t;


/** Type of OUI */
typedef enum
{
    MME_OUI_NOT_PRESENT = 0,
    MME_OUI_MSTAR,
    MME_OUI_NOT_MSTAR
} mme_oui_type_t;
/**
 * Context structure of an MME message
 * \struct mme_ctx_t
 */

typedef struct
{
    /** context current status */
    mme_status_t status;
    /** management message type */
    unsigned short mmtype;
    /** buffer  where to store MME data */
    unsigned char *buffer;
    /** total length of buffer */
    unsigned int length;
    /** start of MME payload */
    unsigned int head;
    /** end of MME payload */
    unsigned int tail;
    /** OUI retrieved from MME */
    mme_oui_type_t oui;
} mme_ctx_t;

//CM_NW_STATS.CNF
typedef struct
{
    unsigned char mac_bin[ETH_ALEN];
    unsigned char tx_phy_data_rate;
    unsigned char rx_phy_data_rate;
} nw_stats_t;

// CM_SET_KEY.REQ
#pragma pack(1)
typedef struct
{
    uint8_t key_type;
    uint8_t my_nonce[4];
    uint8_t your_nonce[4];
    uint8_t pid;
    uint8_t prn[2];
    uint8_t pmn;
    uint8_t cco_capability;
    uint8_t nid[7];
    uint8_t new_eks;
    uint8_t new_key[16];
} set_key_req;

// CM_SET_KEY.CNF
typedef struct
{
    uint8_t result;
    uint8_t my_nonce[4];
    uint8_t your_nonce[4];
    uint8_t pid;
    char prn[2];
    uint8_t pmn;
    uint8_t cco_capability;
} set_key_cnf;

// CM_SLAC_PARM.REQ
typedef struct
{
    uint8_t application_type;
    uint8_t security_type;
    uint8_t run_id[8];
    uint8_t ciphersuitesetsize;
    char ciphersuite[2];
} slac_parm_req;

// CM_SLAC_PARM.CNF
typedef struct
{
    uint8_t m_sound_target[6];
    uint8_t num_sounds;
    uint8_t time_out;         // TT_EVSE_match_MNBC
    uint8_t resp_type;
    uint8_t forwarding_sta[6];
    uint8_t application_type;
    uint8_t security_type;
    uint8_t run_id[8];
    char ciphersuite[2];
} slac_parm_cnf;

// CM_START_ATTEN_CHAR.IND
typedef struct
{
    uint8_t application_type;
    uint8_t security_type;
    struct
    {
        uint8_t num_sounds;    // C_EV_match_MNBC
        uint8_t time_out;         // TT_EVSE_match_MNBC
        uint8_t resp_type;
        uint8_t forwarding_sta[6];
        uint8_t run_id[8];
    } ACVarField;
} start_atten_char_ind;

// CM_MNBC_SOUND.IND
typedef struct
{
    uint8_t application_type;
    uint8_t security_type;
    struct
    {
        char sender_ID[17];
        uint8_t cnt;
        uint8_t run_id[8];
        char rsvd[8];             //other vendor didn't include this var
        char rnd[16];
    } MSVarField;
} mnbc_sound_ind;

// CM_MNBC_SOUND.IND for other vendor
typedef struct
{
    uint8_t application_type;
    uint8_t security_type;
    struct
    {
        char sender_ID[17];
        uint8_t cnt;
        uint8_t run_id[8];
        //char rsvd[8];             //other vendor didn't include this var
        char rnd[16];
    } MSVarField;
} mnbc_sound_ind_q;

// CM_ATTEN_CHAR.IND
typedef struct
{
    uint8_t application_type;
    uint8_t security_type;
    struct
    {
        uint8_t source_address[6];
        uint8_t run_id[8];
        uint8_t source_ID[17];
        uint8_t resp_ID[17];
        uint8_t num_sounds;
        struct
        {
            uint8_t num_groups;              // numGroups comes from CM_ATTEN_PROFILE.IND's NumGroups
            uint8_t AAG[58];                    // "58"  comes from CM_ATTEN_PROFILE.IND's NumGroups
        } ATTEN_PROFILE;
    } ACVarField;
} atten_char_ind;


// CM_ATTEN_PROFILE.IND
typedef struct
{
    uint8_t pev_mac[6];
    uint8_t num_groups[1];
    uint8_t rsvd[1];
    uint8_t AAG[58];
} atten_profile_ind;

// CM_ATTEN_CHAR.RSP
typedef struct
{
    uint8_t application_type;
    uint8_t security_type;
    struct
    {
        uint8_t source_address[6];
        uint8_t run_id[8];
        uint8_t source_ID[17];
        uint8_t resp_ID[17];
        uint8_t result;
    } ACVarField;
} atten_char_rsp;


// CM_SLAC_MATCH.REQ
typedef struct
{
    uint8_t application_type;
    uint8_t security_type;
    uint8_t mvflength[2];
    struct
    {
        uint8_t pev_id[17];
        uint8_t pev_mac[6];
        uint8_t evse_id[17];
        uint8_t evse_mac[6];
        uint8_t run_id[8];
        uint8_t rsvd[8];
    } MatchVarField;
} slac_match_req;

// CM_SLAC_MATCH.CNF
typedef struct
{
    uint8_t application_type;
    uint8_t security_type;
    uint8_t mvflength[2];
    struct
    {
        uint8_t pev_id[17];
        uint8_t pev_mac[6];
        uint8_t evse_id[17];
        uint8_t evse_mac[6];
        uint8_t run_id[8];
        uint8_t rsvd[8];
        uint8_t nid[7];
        uint8_t rsvd2;
        uint8_t nmk[16];
    } MatchVarField;
} slac_match_cnf;
#pragma pack()

// VS_GET_STATUS_CNF
typedef struct
{
    uint8_t result;
    uint8_t status;
    uint8_t cco;
    uint8_t preferred_cco;
    uint8_t backup_cco;
    uint8_t proxy_cco;
    uint8_t simple_connect;
    uint8_t link_state;
} vs_get_status_cnf;


/**
 * Callback use give the received MME after a mme_listen()
 *
 * \param  ctx context containing the received packet
 * \param  iface the network interface where packet has been received
 * \param  source source MAC address of received packet
 * \return error type (MME_SUCCESS if success)
 */
typedef mme_error_t (*mme_listen_cb_t) (mme_ctx_t *ctx, char *iface, unsigned char *source);


/* function prototypes */
/**
 * Create a MME message context.
 * This context is used to build the MME message step by step.<br>
 * The provided buffer must be enough large to contain all the final payload.<br>
 * The MME payload can be bigger than an ethernet payload,
 * as the fragmentation is managed by the 'send' function.
 *
 * \param  ctx MME context to fill with init value
 * \param  mmtype type of MME message (must be in official type list)
 * \param  buffer the buffer to put the payload
 * \param  length  the buffer length
 * \return error type (MME_SUCCESS if success)
 */
mme_error_t
mme_init (mme_ctx_t *ctx, const mmtype_t mmtype,
    unsigned char *buffer, const unsigned int length);

/** Get the current MME payload length
 *
 * \param ctx MME context to get the payload length
 * \param length the returned length
 * \return error type (MME_SUCCESS if success)
 * \return MME_ERROR_NOT_INIT: context not initialized
 */
mme_error_t
mme_get_length (mme_ctx_t *ctx, unsigned int *length);
/** Get the remaining free space to add payload
 *
 * \param ctx MME context to get the remaining space
 * \param length the remaining space
 * \return error type (MME_SUCCESS if success)
 * \return MME_ERROR_NOT_INIT: context not initialized
 */
mme_error_t
mme_get_free_space (mme_ctx_t *ctx, unsigned int *length);

/**
 * Push data at the beginning of the MME payload.
 * Data are inserted between the start of buffer
 * and the current payload data.<br>
 * MME data tail and length are updated.<br>
 * If there is not enough free place to push data,
 * an error is returned and the remaining free space length is returned.
 *
 * \param  ctx MME context where to push data
 * \param  data data to be pushed into payload
 * \param  length length of data to push
 * \param  result_length length of data really pushed
 * \return error type (MME_SUCCESS if success)
 * \return MME_ERROR_NOT_INIT: context not initialized
 * \return MME_ERROR_SPACE: not enough available space
 */

mme_error_t
mme_push (mme_ctx_t *ctx, const void *data,
        unsigned int length, unsigned int *result_length);
/**
 * Put data at the end of MME payload. MME data tail and length are updated<br>
 * If there is not enough free place to put data,
 * an error is returned and the remaining free space length is returned.
 *
 * \param  ctx MME context where to put data
 * \param  data data to put at the end of MME payload
 * \param  length length of data to put
 * \param  result_length length of data really put
 * \return error type (MME_SUCCESS if success)
 * \return MME_ERROR_NOT_INIT: context not initialized
 * \return MME_ERROR_SPACE: not enough available space
 */
mme_error_t
mme_put (mme_ctx_t *ctx, const void *data,
        unsigned int length, unsigned int *result_length);
/**
 * Pull (get) data from beginning of MME payload.
 * MME data head and length are updated<br>
 * If there is not enough data to pull,
 * an error is returned and the remaining payload length is returned.
 *
 * \param  ctx MME context where to get data
 * \param  data buffer where to get data from the beginning of MME payload
 * \param  length length of data to pull
 * \param result_length length of data pulled;
 * if there is not enough data into the MME to fit the length,
 * the remaining data length is returned
 * \return error type (MME_SUCCESS if success)
 * \return MME_ERROR_NOT_INIT: context not initialized
 */
mme_error_t
mme_pull (mme_ctx_t *ctx, void *data,
        unsigned int length, unsigned int *result_length);
/**
 * Pop (get) data from the end of MME payload.
 * MME data tail and length are updated<br>
 * If there is not enough data to pull,
 * an error is returned and the remaining payload length is returned.
 *
 * \param  ctx MME context where to get data
 * \param  data buffer where to get data from the end of MME payload
 * \param  length length of data to pull
 * \param result_length length of data gotten;
 * if there is not enough data into the MME to fit the length,
 * the remaining data length is returned
 * \return error type (MME_SUCCESS if success)
 * \return MME_ERROR_NOT_INIT: context not initialized
 * \return MME_ERROR_SPACE: not enough available space
 */
mme_error_t
mme_pop (mme_ctx_t *ctx, void *data,
        unsigned int length, unsigned int *result_length);
/**
* Build the MME header. It calculates the number of packet and returns it.
* \param mh MME header, filled by this function
* \param ctx MME context to send
* \param src_mac MAC Address of the source
* \param dest MAC Address of the destination
* \return int The number of packets required to send the MME
*/
int
mme_header_prepare (MME_t *mh, mme_ctx_t *ctx,
        unsigned char *src_mac, const unsigned char *dest);
/**
* Build a packet from the context. This function will be called by mme_send
* before sending each fragment of the packet. The function will update the
* fragment offset.
* This function is for internal use, and should not be used outside the
* library.
* \param mh MME header, will be updated with the correct fragment number
* \param ctx MME context to send
* \param fo Fragment offset, from which data must be sent
* \param index Fragment number
* \param pkt Pointer to the head of the data buffer
* \return int Size of the packet (including header)
*/
int
mme_prepare (MME_t *mh, mme_ctx_t *ctx,
                        int *fo, int index, unsigned char *pkt);

/**
 * Send MME to destination MAC address
 * \param  iface interface where to listen to the MME; if NULL,
 *                          iface is guessed according to source
 * \param  ctx context to receive the MME data
 * \parm type type of MME
 * \parm dest destination MAC address
*/
mme_error_t
mme_send (void *opened_interface, mme_ctx_t *ctx,
            mme_send_type_t type, const unsigned char *dest);

/**
 * Wait for an MME request and call an user function to build
 * and send the answer.<br>
 * As MME are level 2 and IP routing does not apply, the network interface
 * where to listen to data packet must be selected inside
 * iface paramater. For instance : "eth0", "br0", ...<br>
 * If a received MME is fragmented, the receive process re-assemble
 * it before delivering.<br>
 * When received MME has been processed and put into a MME context,
 * the user function is called with the context in parameter.
 *
 * \param  ctx context to receive the MME data
 * \param  iface interface where to listen to the MME; if NULL,
 *                          iface is guessed according to source
 * \param  source buffer where is provided
 *                          the source MAC address of received MME
 * \param  listen_cb callback function used when MME packet is received.
 * \param  tout timeout in seconds after which listening expires.
 * \return error type (MME_SUCCESS if success)
 * \return MME_ERROR_NOT_INIT: context not initialized
 */
mme_error_t
mme_listen (mme_ctx_t *ctx, char *iface,
    unsigned char *source, mme_listen_cb_t listen_cb, unsigned int tout);

/**
 * Receive message from a socket
 * \param  opened_interface  selected communication interface
 * \param  recv_pkt  received buffer
 * \param  pkt_len  data length of receive message
 * \param  WaitSec  select() timeout in second
 * \param  WaitmSec  select() timeout in millisecond
 * \return  error type MME_SUCCESS if success
 */
mme_error_t
mme_receive_timeout (void *opened_interface, unsigned char *recv_pkt,
        unsigned int *pkt_len, int WaitSec, int WaitmSec);

/**
 * Remove MME Header from the raw packet
 *
 *
 * \param  mme_type   mmtype of the raw packet
 * \param  recv_pkt  raw packet that you want to remove header
 * \param  pkt_len  data length of raw packet
 * \param  ctx  the packet without mme header
 */
void
mme_remove_header (unsigned short mme_type, unsigned char *recv_pkt,
                        int pkt_len, mme_ctx_t *ctx);

/**
 * Compare the MAC address
 * \param  dest_mac   MAC Address of received MME
 * \param  mac  MAC Address of host
 * \return 0 if match, -1 if not match
 */
int
mme_check_dest_mac (unsigned char *dest_mac, uint8_t *mac);


/**
 * Close the channel
 * \param  opened_interface  pointer to selected communication interface
 */
void
close_interface (void **opened_interface);
#endif /* libmme_inc_libmme_h */
