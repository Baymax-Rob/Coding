/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
/**
 * \file    application/libmme/src/mme.c
 * \brief   MME management library
 * \ingroup libmme
 *
 */
#ifdef _WIN32
#include "pcap.h"
#endif // _WIN32

#include "libmme.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef __linux__
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <inttypes.h>
#endif

#ifdef _WIN32
#include <Iphlpapi.h>
int fds;
#endif

const unsigned char mac_broadcast[ETH_ALEN] =
                    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const unsigned char mac_link_local[ETH_ALEN] =
                    {0x00, 0x13, 0xD7, 0x00, 0x00, 0x01};
unsigned char other_mac_link_local[ETH_ALEN] =
                    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static void
mme_check_oui (mme_ctx_t *ctx, MME_t *mh, unsigned char *pkt)
{
    /* Fill OUI information */
    if (MMTYPE_IS_VS(mh->mmtype))
    {
        /* Set MME_OUI_SPIDCOM only if it was not set before */
        if ( (memcmp (OUI_MSTAR, pkt + sizeof (MME_t), 3) == 0)
            && (ctx->oui != MME_OUI_NOT_MSTAR))
        {
            ctx->oui = MME_OUI_MSTAR;
        }
        else /* the OUI check failed */
        {
            /* One fragment of the MME (if fmi_fmsn != 0) did not
            ** contain MStar OUI, so the whole MME will be
            ** marked as a foreign VS MME */
            ctx->oui = MME_OUI_NOT_MSTAR;
        }
    }
}

/**
 * Construct the FMI field of the MME header.
 * \param  nf_mi  number of fragments
 * \param  fn_mi  fragment number
 * \param  fmsn  fragmentation message sequence number
 * \return  the FMI field.
 */

static inline unsigned short
mme_fmi_set (uint nf_mi, uint fn_mi, uint fmsn)
{
    return (fmsn << 8) | (fn_mi << 4) | nf_mi;
}

/**
 * Get the FMI field uncompressed from the MME header.
 * \param  fmi  the compressed FMI
 * \param  nf_mi  number of fragments
 * \param  fn_mi  fragment number
 * \param  fmsn  fragmentation message sequence number
*/

static inline void
mme_fmi_get (const unsigned short fmi, uint *nf_mi,
             uint *fn_mi, uint *fmsn)
{
    *nf_mi = fmi & 0xF;
    *fn_mi = (fmi >> 4) & 0xF;
    *fmsn = (fmi >> 8) & 0xFF;
}


mme_error_t
mme_init (mme_ctx_t *ctx, const mmtype_t mmtype,
unsigned char *buffer, const unsigned int length)
{
    /* protect from null pointers */
    if (ctx == NULL || buffer == NULL)
        return MME_ERROR_GEN;

    ctx->buffer     =   buffer;
    ctx->mmtype     =   mmtype;
    ctx->length     =   length;
    ctx->head       =   0;
    ctx->tail       =   0;

    /* By default the MME is not VS */
    ctx->oui        =   MME_OUI_NOT_PRESENT;
    ctx->status     =   MME_STATUS_OK;

    return MME_SUCCESS;
}


mme_error_t
mme_get_length (mme_ctx_t *ctx, unsigned int *length)
{
    /* protect from null pointers */
    if (ctx == NULL || length == NULL)
        return MME_ERROR_GEN;
    /* check if ctx has been inititalized */
    if (ctx->status == MME_STATUS_INIT)
        return MME_ERROR_NOT_INIT;

    *length = ctx->tail - ctx->head;
    return MME_SUCCESS;
}


mme_error_t
mme_get_free_space (mme_ctx_t *ctx, unsigned int *length)
{
    /* protect from null pointers */
    if (ctx == NULL || length == NULL)
        return MME_ERROR_GEN;
    /* check if ctx has been inititalized */
    if (ctx->status == MME_STATUS_INIT)
        return MME_ERROR_NOT_INIT;

    *length = ctx->length - (ctx->tail - ctx->head);
    return MME_SUCCESS;
}


mme_error_t
mme_push (mme_ctx_t *ctx, const void *data,
        unsigned int length, unsigned int *result_length)
{
    unsigned int free = 0;
    int delta = 0;

    /* protect from null pointers */
    if (ctx == NULL || data == NULL || result_length == NULL)
        return MME_ERROR_GEN;
    /* check if ctx has been inititalized */
    if (ctx->status == MME_STATUS_INIT)
        return MME_ERROR_NOT_INIT;

    free = ctx->length - (ctx->tail - ctx->head);
    if (length > free)
    {
        *result_length = free;
        return MME_ERROR_SPACE;
    }

    *result_length = length;

    /* make place in front, if needed */
    if (length > ctx->head)
    {
        /*
         *             *length
         *  .-----------^-----------.
         *  |---------|xxxxxxxxxxxxx|--------------------|---------------------|
         * buff      head                              tail                 ctx->length
         *             \_______________  _______________/
         *                             \/
         *                           pyload
         *
         * we have to shift right our payload for this difference delta marked with 'x'
         * in order for *length bytes to fit in from beginning of the buffer
         */
        delta = length - ctx->head;
        memmove (ctx->buffer + ctx->head + delta,
            ctx->buffer + ctx->head, ctx->tail - ctx->head);

        /* update head and tail pointers (offsets) */
        ctx->head += delta;
        ctx->tail += delta;
    }

    /* place head pointer to where copy chould start from */
    ctx->head -= length;
    memcpy (ctx->buffer + ctx->head, data, length);

    return MME_SUCCESS;
}


mme_error_t
mme_put (mme_ctx_t *ctx, const void *data,
    unsigned int length, unsigned int *result_length)
{
    unsigned int free = 0;
    int delta = 0;

    /* protect from null pointers */
    if (ctx == NULL || data == NULL || result_length == NULL)
        return MME_ERROR_GEN;
    /* check if ctx has been inititalized */
    if (ctx->status == MME_STATUS_INIT)
        return MME_ERROR_NOT_INIT;

    free = ctx->length - (ctx->tail - ctx->head);
    if (length > free)
    {
        *result_length = free;
        return MME_ERROR_SPACE;
    }

    *result_length = length;

    /* make place after payload, if needed */
    if (length > ctx->length - ctx->tail)
    {
        /*
         *                                       *length
         *                           .---------------^-----------------.
         *  |-----------|------------|xxxxxxx|-------------------------|
         * buff        head                 tail                    ctx->length
         *               \________  ________/
         *                        \/
         *                      payload
         *
         * we have to shift left our payload for this difference delta marked with 'x'
         * in order for *length bytes to fit in from beginning of the buffer
         */
        delta = length - (ctx->length - ctx->tail);
        memmove ((unsigned char *) (ctx->buffer + ctx->head - delta),
        (unsigned char *) (ctx->buffer + ctx->head), ctx->tail - ctx->head);

        /* update head and tail pointers (offsets) */
        ctx->head -= delta;
        ctx->tail -= delta;
    }

    memcpy ((unsigned char *) (ctx->buffer + ctx->tail),
        (unsigned char *) data, length);
    ctx->tail += length;

    return MME_SUCCESS;
}


mme_error_t
mme_pull (mme_ctx_t *ctx, void *data,
    unsigned int length, unsigned int *result_length)
{
    /* protect from null pointers */
    if (ctx == NULL || data == NULL || result_length == NULL)
        return MME_ERROR_GEN;
    /* check if ctx has been inititalized */
    if (ctx->status == MME_STATUS_INIT)
        return MME_ERROR_NOT_INIT;

    /* check if it is demanded more data than we have in payload */
    if (length > ctx->tail - ctx->head)
    {
        *result_length = ctx->tail - ctx->head;
        printf("mme_pull() error  [MME_ERROR_ENOUGH]\n");
        printf("%s: length= %d, ctx->tail= %d, ctx->head= %d\n",
            __func__, length, ctx->tail, ctx->head );
        return MME_ERROR_ENOUGH;
    }

    *result_length = length;

    memcpy (data, (unsigned char *) (ctx->buffer + ctx->head), length);
    ctx->head += length;

    return MME_SUCCESS;
}


mme_error_t
mme_pop (mme_ctx_t *ctx, void *data,
    unsigned int length, unsigned int *result_length)
{
    /* protect from null pointers */
    if (ctx == NULL || data == NULL || result_length == NULL)
        return MME_ERROR_GEN;
    /* check if ctx has been inititalized */
    if (ctx->status == MME_STATUS_INIT)
        return MME_ERROR_NOT_INIT;

    /* check if it is demanded more data than we have in payload */
    if (length > ctx->tail - ctx->head)
    {
        *result_length = ctx->tail - ctx->head;
        return MME_ERROR_ENOUGH;
    }

    *result_length = length;

    memcpy (data,
        (unsigned char *) (ctx->buffer + ctx->tail - length), length);
    ctx->tail -= length;

    return MME_SUCCESS;
}


#ifdef __linux__
static int
_get_iface_mac_addr (int sock, const char *iface, unsigned char *mac_addr)
{
    struct ifreq ifr;

    memset (&ifr, 0x0, sizeof (ifr));

    /* Get the interface Index  */
    strncpy (ifr.ifr_name, iface, IFNAMSIZ);
    if ((ioctl (sock, SIOCGIFINDEX, &ifr)) < 0)
    {
        return -1;
    }
    if (ioctl (sock, SIOCGIFHWADDR, &ifr) < 0)
    {
        return -1;
    }
    memcpy (mac_addr, ifr.ifr_hwaddr.sa_data, ETH_ALEN);
    return 0;
}
#endif


int
mme_header_prepare (MME_t *mh, mme_ctx_t *ctx,
                unsigned char *src_mac, const unsigned char *dest)
{
    int   nbpkt = 0;


    uint  fmi_nf_mi = 0; /* Number of fragments */
    uint  fmi_fmsn = 0; /* SSN of the fragmented mme */
    uint  payload_size;
    /*
     * --- Create MME header ---
     */

    /* copy the Src mac addr */
    memcpy (mh->mme_src, (void *)src_mac, 6);
    /* copy the Dst mac addr */
    memcpy (mh->mme_dest, (void *)dest, 6);
    /* copy the protocol */
    mh->mtype = htons (MME_TYPE);
    /* set default mme version */
    mh->mmv = MME_VERSION;
    mh->mmtype = ctx->mmtype;

    /* Set the max payload size */
    SET_MME_PAYLOAD_MAX_SIZE(mh->mmtype, payload_size);

    /* calculate number of mme packets needed */
    nbpkt = (ctx->tail - ctx->head) / payload_size;
    if (nbpkt == 0 || ((ctx->tail - ctx->head) % payload_size) > 0)
    {
        nbpkt++;
    }

    fmi_nf_mi  = nbpkt - 1;
    /* Set the sequence number to 1 if the message is fragmented.
     * This number correspond to a session number, all fragmented MME
     *  of this session must have the same fmsn number. */
    fmi_fmsn = fmi_nf_mi ? 1 : 0;
    mh->fmi = mme_fmi_set (fmi_nf_mi, 0, fmi_fmsn);

    return nbpkt;
}


int
mme_prepare
    (MME_t *mh, mme_ctx_t *ctx, int *fo, int index, unsigned char *pkt)
{
    uint  fs = 0;
    uint  fmi_nf_mi = 0; /* Number of fragments */
    uint  fmi_fn_mi = 0; /* Fragment number */
    uint  fmi_fmsn = 0; /* SSN of the fragmented mme */
    uint  payload_size, payload_min_size;

    SET_MME_PAYLOAD_MAX_SIZE (mh->mmtype, payload_size);
    SET_MME_PAYLOAD_MIN_SIZE (mh->mmtype, payload_min_size);

    /*
     * --- Set per-message fragmentation info, current fragment number ---
     */
    mme_fmi_get (mh->fmi, &fmi_nf_mi, &fmi_fn_mi, &fmi_fmsn);
    mh->fmi = mme_fmi_set (fmi_nf_mi, index, fmi_fmsn);
    /*
     * --- Append payload ---
     */

    /* In case of VS MME, append the OUI first */
    if (MMTYPE_IS_VS(mh->mmtype))
    {
        memcpy (pkt, OUI_MSTAR, MSTAR_OUI_SIZE);
        pkt += MSTAR_OUI_SIZE;
    }

    /* calculate the size of the current fragment */
    fs =
    ( (ctx->tail - *fo) < payload_size) ? (ctx->tail - *fo) : payload_size;
    /* copy the content into packet buffer */
    memcpy (pkt, ctx->buffer + *fo, fs);
    /* update fo */
    *fo += fs;

    /* zero-pad the rest if last packet fs < MME_MIN_SIZE,
     * which is minimum size for mme packet */
    if (fs < payload_min_size)
    {
        memset (pkt + fs, 0x0, payload_min_size - fs);
        fs = payload_min_size;
    }

    /* In case of VS MME, add SPIDCOM_OUI_SIZE to the payload */
    if (MMTYPE_IS_VS (mh->mmtype))
    {
        fs += MSTAR_OUI_SIZE;
    }

    return (fs + sizeof (MME_t));
}


#ifdef _WIN32
int
populate_mac_addr (hpav_iface_t **interfaces_list)
{
    hpav_iface_t *first_if = NULL;

    // Code exttracted from MSDN documentation
    PIP_ADAPTER_INFO pAdapterInfo = NULL;
    PIP_ADAPTER_INFO pAdapter = NULL;
    ULONG ulOutBufLen = sizeof (IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO *)malloc (sizeof (IP_ADAPTER_INFO));

    if (interfaces_list != NULL)
    {
        first_if = *interfaces_list;
    }
    else
    {
        return -1;
    }

    // Make an initial call to GetAdaptersInfo to get
    // the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo (pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
    {
        free (pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc (ulOutBufLen);
    }

    if (GetAdaptersInfo (pAdapterInfo, &ulOutBufLen) == NO_ERROR)
    {
        /* We know have the list of adapters. Loop on the interfaces
        and for each of them find the corresponding adapter if possible */
        while (first_if != NULL)
        {
            /* The interface description contains a prefix
                that we need to skip to find { */
            char* p_name = first_if->name;
            while (*p_name != '{' && *p_name != '\0')
            {
                p_name++;
            }
            if (p_name != NULL)
            {
                pAdapter = pAdapterInfo;
                while (pAdapter)
                {
                    if (strcmp (p_name, pAdapter->AdapterName) == 0
                        && pAdapter->AddressLength == ETH_MAC_ADDRESS_SIZE)
                    {
                        memcpy (first_if->mac_addr,
                            pAdapter->Address, ETH_MAC_ADDRESS_SIZE);
                    }
                    pAdapter = pAdapter->Next;
                }
            }
            first_if = first_if->next;
        }
    }
    if (pAdapterInfo != NULL)
    {
        free (pAdapterInfo);
    }
    return 0;
}


int
get_eth_interfaces (hpav_iface_t **interface_list)
{
    // Local variables declaration
    pcap_if_t *pcap_interfaces = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];
    hpav_iface_t *prev_if = NULL;
    hpav_iface_t *new_if = NULL;
    hpav_iface_t *first_if = NULL;
    int result = -1, i = 0;

    // NULL is failure
    *interface_list = NULL;

    // Calls libpcap
    result = pcap_findalldevs (&pcap_interfaces, errbuf);
    if (result == -1)
    {
        return -1;
    }

    pcap_if_t *d;
    // Print the list
    for (d = pcap_interfaces; d != NULL; d = d->next)
    {
        LOG_PRINT (LOG_VERBOSE, "%d. %s", i++, d->name);
        if (d->description)
            LOG_PRINT (LOG_VERBOSE, " (%s)\n", d->description);
        else
            LOG_PRINT (LOG_VERBOSE, " (No description available)\n");
    }
    // Copy data from libpcap into hpav data structures
    while (pcap_interfaces != NULL)
    {
        /* On Windows, skip generic dialup interface.
         Winpcap cannot sendpacket to this interface */
        if (strcmp
        ("\\Device\\NPF_GenericDialupAdapter", pcap_interfaces->name) != 0)
        {
            // New hpav_iface
            new_if = malloc (sizeof (hpav_iface_t));
            // Record the first interface to send it back to the caller
            if (first_if == NULL)
            {
                first_if = new_if;
            }
            // Initialise interface MAC address
            memset (new_if->mac_addr, 0, ETH_MAC_ADDRESS_SIZE);
            // Copy interface name
            new_if->name = malloc (strlen (pcap_interfaces->name) + 1);
            strcpy (new_if->name, pcap_interfaces->name);
            // Copy interface description (can be NULL)
            if (pcap_interfaces->description != NULL)
            {
                new_if->description =
                    malloc (strlen (pcap_interfaces->description) + 1);
                strcpy (new_if->description, pcap_interfaces->description);
            }
            else
            {
                new_if->description = NULL;
            }
            new_if->next = NULL;
            // If there is a previous inteface, chain the new one to it
            if (prev_if != NULL)
            {
                prev_if->next = new_if;
            }
            prev_if = new_if;
        }
        pcap_interfaces = pcap_interfaces->next;
    }

    // We don't need pcap interfaces anymore : free them
    pcap_freealldevs (pcap_interfaces);

    // Return a pointer to the first interface found
    *interface_list = first_if;

    // Populate MAC address (platform specific)
    populate_mac_addr (interface_list);

    return 0; // Success
}


int
free_eth_interface_list (hpav_iface_t *interface_list)
{
    while (interface_list != NULL)
    {
        struct hpav_iface *current_interface = interface_list;
        free (interface_list->name);
        if (interface_list->description != NULL)
        {
            free (interface_list->description);
        }
        interface_list = interface_list->next;
        free (current_interface);
    }
    return 0;
}


struct hpav_iface*
hpav_get_interface_by_index_num (struct hpav_iface *interfaces, unsigned int if_num)
{
    struct hpav_iface *current_interface = NULL;
    struct hpav_iface *interface_result = NULL;
    unsigned int interface_num = 0;

    current_interface = interfaces;
    while (current_interface != NULL)
    {
        if (interface_num == if_num)
        {
            interface_result = current_interface;
        }
        current_interface = current_interface->next;
        interface_num++;
    }
    return interface_result;
}


hpav_channel_t*
open_channel (hpav_iface_t *interface_to_open)
{
    // Local variables
    hpav_channel_t *return_chan = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcap_chan = NULL;
    u_int net = 0;  // network IP address
    u_int mask = 0;	 // network address mask

    /**
     * Open interface with libpcap
     * 100 ms is to make sure we can frequently update timeout
     * and check exit conditions while sniffing.
     */
    pcap_chan = pcap_open_live (interface_to_open->name, 65535,
        PCAP_OPENFLAG_PROMISCUOUS | PCAP_OPENFLAG_NOCAPTURE_LOCAL,
        100, errbuf);

    if (pcap_chan != NULL)
    {
        /**
         * sets a flag that will force pcap_dispatch() or pcap_loop()
         * to return rather than looping.
         */
        pcap_breakloop (pcap_chan);
        if (0 != pcap_setnonblock (pcap_chan, 1, errbuf))
        {
            LOG_PRINT (LOG_DEBUG,
                "pcap_setnonblock() return -1, PCAP errbuf: %s", errbuf);
            return NULL;
        }

        // Built returned structure
        return_chan = (hpav_channel_t *)malloc (sizeof (hpav_channel_t));
        return_chan->pcap_chan = pcap_chan;
        memcpy (return_chan->mac_addr,
            interface_to_open->mac_addr, ETH_MAC_ADDRESS_SIZE);

        /* Compile the program with a filter - non-optimized */
        /* Hold compiled program */
        struct bpf_program fp;
        /* Result */
        int result = -1;

        /* [ISSUE] filter "0x88e1" will dicard any broadast packet */
        if (pcap_compile (return_chan->pcap_chan, &fp,
            "ether proto 0x88E1 or ether broadcast", 0, 0) == -1)
        {
            char buffer[PCAP_ERRBUF_SIZE + 128];
            sprintf (buffer, "PCAP error code : %d, errbuf : %s",
                result, pcap_geterr (return_chan->pcap_chan));
            return NULL;
        }

        /* Set the compiled program as the filter */
        if (pcap_setfilter (return_chan->pcap_chan, &fp) == -1)
        {
            char buffer[PCAP_ERRBUF_SIZE + 128];
            sprintf (buffer, "PCAP error code : %d, errbuf : %s",
                result, pcap_geterr (return_chan->pcap_chan));
            return NULL;
        }
        return return_chan;
    }
    else
    {
        LOG_PRINT (LOG_DEBUG,
            "pcap_open_live() return NULL, errbuf: %s", errbuf);
        return NULL;
    }
}
#endif


int
open_interface (const char *iname, void *opened_interface)
{
    int raw;
#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
    struct sockaddr_ll sll;
    struct ifreq ifr;
    unsigned char mac[6] = {0};     /* MAC of chosen interface */

    if (iface == NULL)
    {
        LOG_PRINT (LOG_DEBUG, "Error: interface id NULL");
        return IFACE_FAIL;
    }

    /*
    * --- Create the raw socket ---
    */
    if ( (raw = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_HPAV))) == -1)
    {
        LOG_PRINT (LOG_DEBUG,
            "Error creating raw socket: %s\n", strerror (errno));
        return IFACE_FAIL;
    }

    memset (&sll, 0x0, sizeof (sll));
    memset (&ifr, 0x0, sizeof (ifr));

    _get_iface_mac_addr (raw, iname, mac);

    /* Get index of needed interface  */
    strncpy (ifr.ifr_name, iname, IFNAMSIZ);
    if ( (ioctl (raw, SIOCGIFINDEX, &ifr)) == -1)
    {
        LOG_PRINT (LOG_DEBUG, "Error getting %s index !\n", iname);
        close (raw);
        return IFACE_FAIL;
    }

    /* Bind our raw socket to this interface */
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons (ETH_P_HPAV);
    if ((bind (raw, (struct sockaddr *)&sll, sizeof (sll)))== -1)
    {
        LOG_PRINT (LOG_DEBUG,
            "Error binding raw socket to interface: %s\n", strerror(errno));
        close (raw);
        return IFACE_FAIL;
    }
    iface->fid = raw;
    memcpy (iface->mac, mac, ETH_MAC_ADDRESS_SIZE);
#endif

#ifdef _WIN32

    struct hpav_iface *interfaces = NULL;
    int interface_num_to_open = atoi (iname);
    hpav_channel_t *current_chan = NULL;
    hpav_channel_t *return_chan = (hpav_channel_t *)opened_interface;
    get_eth_interfaces (&interfaces);

    if (interfaces != NULL)
    {
        struct hpav_iface *interface_to_open = NULL;
        // Get interface
        interface_to_open =
            hpav_get_interface_by_index_num (interfaces, interface_num_to_open);

        if (interface_to_open != NULL)
        {
            // Open the interface
            LOG_PRINT (LOG_DEBUG, "Opening interface %d : %s (%s)\n",
                interface_num_to_open, interface_to_open->name,
                (interface_to_open->description != NULL
                    ? interface_to_open->description : "no description"));

            current_chan = open_channel (interface_to_open);
            if (current_chan == NULL)
            {
                return IFACE_FAIL;
            }
            LOG_PRINT (LOG_VERBOSE, "Interface successfully opened\n");
        }

        /**
         * Copy current_chan to return_chan so that Caller
         * can access the descriptor of channel
         */
        return_chan->pcap_chan = current_chan->pcap_chan;
        memcpy (return_chan->mac_addr,
            current_chan->mac_addr, ETH_MAC_ADDRESS_SIZE);
        free_eth_interface_list (interfaces);
    }
    else
    {
        LOG_PRINT (LOG_DEBUG, "No interface available\n");
        return IFACE_FAIL;
    }
#endif
    return IFACE_SUCCESS;
}


void
close_interface (void **opened_interface)
{
#ifdef _WIN32
    hpav_channel_t *channel = (hpav_channel_t *)*opened_interface;

    // Close underlying pcap pointer
    pcap_close (channel->pcap_chan);

    free (channel);
    /**
     * Assign opened_interface to 0 so that later
     * we can check this value to decide whether we can send mme.
     */
    *opened_interface = 0;
#endif // _WIN32
#ifdef __linux__
    iface_t *iface = (iface_t *)*opened_interface;
    close(iface->fid);
#endif // __linux__
}


mme_error_t
mme_send (void *opened_interface, mme_ctx_t *ctx,
                    mme_send_type_t type, const unsigned char *dest)
{
#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
    unsigned char *src = iface->mac;
#endif // __linux__
#ifdef _WIN32
    hpav_channel_t *return_chan = (hpav_channel_t *)opened_interface;
    unsigned char *src = return_chan->mac_addr;
#endif // _WIN32

    MME_t *mh;
    unsigned char *pkt;
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = { 0 };
    int pkt_len;
    int sent = -1;
    unsigned short nbpkt = 0;
    int fo;      /* current fragment offset */

    uint fmi_nf_mi = 0; /* Number of fragments */
    uint fmi_fn_mi = 0; /* Fragment number */
    uint fmi_fmsn = 0; /* SSN of the fragmented mme */
    int i;


    /* protect from null pointers */
    if (ctx == NULL || dest == NULL)
        return MME_ERROR_GEN;
    /* check if ctx has been inititalized */
    if (ctx->status == MME_STATUS_INIT)
        return MME_ERROR_NOT_INIT;

    /*
     * #####################
     * #    SEND
     * #####################
     */
    mh = (MME_t *)pkt_holder;

    /* nbpkt = the number of packet */
    nbpkt = mme_header_prepare (mh, ctx, src, dest);
    mme_fmi_get (mh->fmi, &fmi_nf_mi, &fmi_fn_mi, &fmi_fmsn);

    /* Security check, if nbpkt is more than 15 don't send anything
     * and inform the user. */
    if (fmi_nf_mi > 0xf)
    {
        printf ("The MME to send is too long and cannot be fragmented\n");
        return MME_ERROR_SPACE;
    }

    /* initialize fragment offset to the begining of the payload*/
    fo = ctx->head;

    /* prepare and send out packets */
    for (i = 0; i < nbpkt; i++)
    {
        /*
         * --- Append payload ---
         */
        /* skip the MME header */
        pkt = pkt_holder + sizeof (MME_t);

        /* determine the size of whole packet (header + data) */
        pkt_len = mme_prepare (mh, ctx, &fo, i, pkt);

        /*
         * --- Send raw packet ---
         */
        /* initialize pkt pointer to the start of the send buffer */
        pkt = pkt_holder;

#ifdef __linux__

        int raw = iface->fid;
        if ( (sent = write (raw, pkt, pkt_len)) != pkt_len)
        {
            perror ("string error\n");
            printf ("Could only send %d bytes of packet of length %d\n",
                sent, pkt_len);
            return MME_ERROR_TXRX;
        }
#endif // __linux__
#ifdef _WIN32
        if ( (sent = pcap_sendpacket
            (return_chan->pcap_chan, pkt, pkt_len)) != 0)
        {
            char buffer[PCAP_ERRBUF_SIZE + 128];
            sprintf (buffer, "PCAP error code : %d, errbuf : %s",
                    sent, pcap_geterr(return_chan->pcap_chan));
        }
#endif // _WIN32

    } /* for */

    return MME_SUCCESS;
}


mme_error_t
mme_listen (mme_ctx_t *ctx, char *iface, unsigned char *source,
                        mme_listen_cb_t listen_cb, unsigned int tout)
{
#ifdef __linux__
    int sock_iface, sock_lo;
    struct sockaddr_ll sll;
    struct ifreq ifr;
    MME_t *mh;
    unsigned char *pkt;
    unsigned char pkt_holder[ETH_PACKET_MAX_NOVLAN_SIZE] = {0};
    fd_set fds;
    struct sockaddr_ll pkt_info;
    int last_pkt = 0;
    uint fmi_nf_mi = 0; /* Number of fragments */
    uint fmi_fn_mi = 0; /* Fragment number */
    uint fmi_fmsn = 0; /* SSN of the fragmented mme */
    struct timeval timeout; /* When we should time out */
    struct timeval now;
    struct timeval waittime;
    int res;
    int len;
    uint mme_h_size; /* Size of MME header */
    unsigned int result_len;
    int pkt_nfo_sz = sizeof (pkt_info);
    mme_error_t ret;
    uint pkt_cnt = 0;            /* to indicate if we recive packets in
                                 * correct order */

    /* protect from null pointers */
    if (ctx == NULL || source == NULL)
        return MME_ERROR_GEN;
    /* check if ctx has been inititalized */
    if (ctx->status == MME_STATUS_INIT)
        return MME_ERROR_NOT_INIT;

    /* Set the MME Header size */
    SET_MME_HEADER_SIZE(ctx->mmtype, mme_h_size);

    /*
     * --- Create the iface socket ---
     */
    if ((sock_iface = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_HPAV))) < 0)
    {
        perror ("Error creating sock_iface socket: ");
        return MME_ERROR_GEN;
    }

    memset (&sll, 0x0, sizeof (sll));
    memset (&ifr, 0x0, sizeof (ifr));

    if (NULL == iface)
    {
        strncpy (ifr.ifr_name, MME_IFACE_BRIDGE, IFNAMSIZ);
    }
    else
    {
        strncpy (ifr.ifr_name, iface, IFNAMSIZ);
    }

    /* get iface index */
    if ((ioctl (sock_iface, SIOCGIFINDEX, &ifr)) == -1)
    {
        printf ("%s: Error getting %s index\n", __FUNCTION__, iface);
        close (sock_iface);
        return MME_ERROR_GEN;
    }

    /* Bind socket to this interface */
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons (ETH_P_HPAV);

    if ( (bind (sock_iface, (struct sockaddr *)&sll, sizeof (sll))) == -1)
    {
        perror ("Error binding iface to interface: ");
        close (sock_iface);
        return MME_ERROR_GEN;
    }

    /* create loopback socket */
    if ( (sock_lo = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_HPAV))) < 0)
    {
        perror ("Error creating loopback socket: ");
        close (sock_iface);
        return MME_ERROR_GEN;
    }

    memset (&sll, 0x0, sizeof (sll));
    memset (&ifr, 0x0, sizeof (ifr));

    if (NULL == iface)
    {
        /* Get the loopback Interface Index  */
        strncpy (ifr.ifr_name, MME_IFACE_LOOPBACK, IFNAMSIZ);
        /* get iface index */
        if ( (ioctl (sock_lo, SIOCGIFINDEX, &ifr)) == -1)
        {
            perror ("Error getting loopback index: ");
            close (sock_iface);
            close (sock_lo);
            return MME_ERROR_GEN;
        }

        /* Bind socket to this interface */
        sll.sll_family = AF_PACKET;
        sll.sll_ifindex = ifr.ifr_ifindex;
        sll.sll_protocol = htons (ETH_P_HPAV);

        if ( (bind (sock_lo, (struct sockaddr *)&sll, sizeof (sll))) == -1)
        {
            perror ("Error binding loopback socket to interface\n");
            close (sock_iface);
            close (sock_lo);
            return MME_ERROR_GEN;
        }
    }

    /* Calculate into 'timeout' when we should time out */
    gettimeofday (&timeout, 0);
    timeout.tv_sec += (tout != 0) ? tout : MME_TOUT;

    /* initialize packet counter */
    pkt_cnt = 0;

    do  /* receive the payload packages */
    {
        /* rewind pkt pointer to begining of the packet holder */
        pkt = pkt_holder;

        for (;;)  /* listen until we get package from our proper source */
        {
            /* Force timeout if we ran out of time */
            gettimeofday (&now, 0);
            if (timercmp (&now, &timeout, >) != 0)
            {
                printf ("Server indication timeout.\n");
                close (sock_iface);
                close (sock_lo);
                return MME_ERROR_TIMEOUT;
            }

            /* Calculate into 'waittime' how long to wait */
            timersub (&timeout, &now, &waittime); /* wait = timeout - now */

            /* Init fds with 'sock' as the only fd */
            FD_ZERO (&fds);
            FD_SET (sock_iface, &fds);
            if (NULL == iface)
            {
                FD_SET (sock_lo, &fds);
            }

            res = select (sock_iface + 2, &fds, NULL, NULL, &waittime);
            if (res < 0)
            {
                perror ("select() failed");
                close (sock_iface);
                close (sock_lo);
                return MME_ERROR_GEN;
            }
            else if (res == 0)  /* timeout */
            {
                printf ("Server indication timeout.\n");
                close (sock_iface);
                close (sock_lo);
                return MME_ERROR_TIMEOUT;
            }

            if (FD_ISSET (sock_iface, &fds))
            {
                if ( (len = recvfrom (sock_iface, pkt,
                        ETH_PACKET_MAX_NOVLAN_SIZE, 0,
                        (struct sockaddr*)&pkt_info,
                        (socklen_t *)&pkt_nfo_sz)) < 0)
                {
                    perror ("Recv from returned error: ");
                    close (sock_iface);
                    close (sock_lo);
                    return MME_ERROR_TXRX;
                }
            }
            else
            {
                if ( (len = recvfrom
                    (sock_lo, pkt, ETH_PACKET_MAX_NOVLAN_SIZE,
                        0, (struct sockaddr*)&pkt_info,
                        (socklen_t *)&pkt_nfo_sz)) < 0)
                {
                    perror ("Recv from loopback returned error: ");
                    close (sock_iface);
                    close (sock_lo);
                    return MME_ERROR_TXRX;
                }
            }


            mh = (MME_t *)pkt;
            mme_fmi_get (mh->fmi, &fmi_nf_mi, &fmi_fn_mi, &fmi_fmsn);

            /* check if indication came from the good source,
             * and if it is, check if cooresponds to our prepared response
             * (REQ/CNF or IND/RSP); by HP AV standard difference is in LSB,
             * i.e. if REQ LSBs are ...00, CNF will be ...01,
             * IND ...10 and RSP ...11 REQ is always CNF - 1,
             * IND is RSP - 1 */
            /* we received packet from correct source */
            if (memcmp (pkt_info.sll_addr, source, ETH_ALEN) == 0
            && (mh->mmtype == ctx->mmtype)
            && fmi_fn_mi == pkt_cnt)
            {
                mme_check_oui (ctx, mh, pkt);
                break;  /* good answer - go out and process the package */
            }
        } /* for */

        /* check if the packet we received is the last one */
        if (fmi_fn_mi == fmi_nf_mi)
            last_pkt = 1;

        /* skip the MME header */
        pkt += mme_h_size;
        len -= mme_h_size;

        /* copy the packet into the receive buffer */
        ret = mme_put (ctx, pkt, len, &result_len);
        if (ret != MME_SUCCESS)
        {
            close (sock_iface);
            close (sock_lo);
            return ret;
        }

        pkt_cnt++;

    } while (!last_pkt);

    /* and now whe we assembled the message, call the processing function */
    if (listen_cb != NULL)
    {
        ret = listen_cb (ctx, iface, source);
        if (ret != MME_SUCCESS)
        {
            close (sock_iface);
            close (sock_lo);
            return ret;
        }
    }

    close (sock_iface);
    close (sock_lo);
    #endif // __linux__
    return MME_SUCCESS;
}

#ifdef _WIN32

int
get_sys_time (struct hpav_sys_time *sys_time)
{
    GetSystemTime (&sys_time->win_sys_time);
    // GetSytemeTime doesn't return any error code. Always return 0 here.
    return 0;
}

int
get_elapsed_time_ms
(struct hpav_sys_time *p_start_sys_time, struct hpav_sys_time *p_end_sys_time)
{
    // This is the recommended way on Windows
    FILETIME start_file_time;
    FILETIME end_file_time;
    LARGE_INTEGER start_i64;
    LARGE_INTEGER end_i64;

    // This Win32 call can fail, but MSDN doesn't say how...
    SystemTimeToFileTime (&p_start_sys_time->win_sys_time, &start_file_time);
    SystemTimeToFileTime (&p_end_sys_time->win_sys_time, &end_file_time);

    start_i64.LowPart = start_file_time.dwLowDateTime;
    start_i64.HighPart = start_file_time.dwHighDateTime;
    end_i64.LowPart = end_file_time.dwLowDateTime;
    end_i64.HighPart = end_file_time.dwHighDateTime;
    // File time has 100ns accuracy. Divide by 10000 to get ms.
    return (int) ( ( (__int64)end_i64.QuadPart - (__int64)start_i64.QuadPart)
                                                        / (__int64)10000);
}


static inline
void
rx_callback (u_char *user,
    const struct pcap_pkthdr *packet_header,
    const u_char *packet_data)
{
    /**
     * user: pointer that you send
     * packet_data: received packet data
     */
    callback_data_t *user_data = (callback_data_t *)user;
    MME_t *eth_frame = (MME_t *)packet_data;

    if ( (memcmp (user_data->mac, eth_frame->mme_dest, ETH_MAC_ADDRESS_SIZE) == 0)
    || (memcmp (mac_broadcast, eth_frame->mme_dest, ETH_MAC_ADDRESS_SIZE) == 0))
    {
        if ( (packet_data[12] == 0x88) && (packet_data[13] == 0xe1))
        {
            user_data->stop = 1;
            user_data->data = (unsigned char *)malloc (packet_header->caplen);
            memcpy (user_data->data, packet_data, packet_header->caplen);
            user_data->packet_len = packet_header->caplen;
        }
    }
}
#endif // _WIN32


mme_error_t
mme_receive_timeout (void *opened_interface, unsigned char *recv_pkt,
                    unsigned int *pkt_len, int wait_sec, int wait_msec)
{
#ifdef _WIN32
    hpav_channel_t *return_chan = (hpav_channel_t *)opened_interface;
    struct hpav_sys_time start_time, end_time;
    int result = 0;
    /* User data passed to pcap_dispatch() callback */
    callback_data_t cb_data;
    /* Init callback user data */
    memset (&cb_data, 0, sizeof (callback_data_t));
    memcpy (cb_data.mac, return_chan->mac_addr, ETH_MAC_ADDRESS_SIZE);
    cb_data.data = NULL;
    get_sys_time (&start_time);
    int wait_all_ms = wait_sec * 1000 + wait_msec;

    /* We loop until we have collected all the packets
       with the right Ethertype and MME type */
    do {
        result = pcap_dispatch
        (return_chan->pcap_chan, -1, rx_callback, (u_char *)&cb_data);

        if (result > 0)
        {
            /* PCAP: number of  packets  processed  on  success */
        }
        else if (result == 0)
        {
            /* PCAP: no packets to read */
        }
        else if (result == -1)
        {
            /* PCAP: error occured */
            char buffer[PCAP_ERRBUF_SIZE + 128];
            sprintf (buffer, "PCAP error code : %d, errbuf : %s",
                result, pcap_geterr (return_chan->pcap_chan));
            return MME_ERROR_TXRX;
        }
        else /* (result == -2) */
        {
            /* PCAP: the loop terminated due to a call to pcap_breakloop()
                before any packets were processed. */
        }

        /* Compute elapsed time */
        get_sys_time (&end_time);

    /* Exit loop when caller timeout is reached */
    } while ( (get_elapsed_time_ms (&start_time, &end_time)
                        <= (int)wait_all_ms) && !cb_data.stop);

    if (result < 0)
    {
        return MME_ERROR_TIMEOUT;
    }
    else if ( (result == 0) && (cb_data.packet_len == 0))
    {
        return MME_ERROR_TIMEOUT;
    }

    int i = 0;
    for (i = 0; i < cb_data.packet_len; i++)
    {
        LOG_PRINT (LOG_VERBOSE, "%02x ", cb_data.data[i]);
    }
    LOG_PRINT (LOG_VERBOSE, "\n\n");

    memcpy (recv_pkt, cb_data.data, cb_data.packet_len);

    free (cb_data.data);
    cb_data.data = NULL;

    *pkt_len = cb_data.packet_len;
#endif // _WIN32

#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;

    int raw = iface->fid;
    fd_set fds;
    struct sockaddr_ll pkt_info;
    struct timeval timeout; /* When we should time out */
    struct timeval now;
    struct timeval waittime;
    int res;
    int pkt_nfo_sz = sizeof (pkt_info);

    /* Calculate into 'timeout' when we should time out */
    gettimeofday (&timeout, 0);
    timeout.tv_sec += wait_sec;
    timeout.tv_usec += wait_msec * 1000;

    gettimeofday (&now, 0);

    if (timercmp (&now, &timeout, >) != 0)
    {
        LOG_PRINT (LOG_VERBOSE, "Server response timeout.\n");
        return MME_ERROR_TIMEOUT;
    }

    /* Calculate into 'waittime' how long to wait */
    timersub (&timeout, &now, &waittime); /* wait = timeout - now */

    /*Init fds with 'sock' as the only fd */
    FD_ZERO (&fds);
    FD_SET (raw, &fds);

    /* fds fd_set is containing only our socket, */
    /* so no need to check with FD_ISSET() */
    res = select (raw + 1, &fds, NULL, NULL, &waittime);

    if (res < 0)
    {
        LOG_PRINT (LOG_VERBOSE, "select() failed\n");
        return MME_ERROR_GEN;
    }
    else if (res == 0)  /* timeout */
    {
        //printf ("%s: Server response timeout2.\n", __func__);
        return MME_ERROR_TIMEOUT;
    }
    if ( (*pkt_len = recvfrom (raw, recv_pkt,
            ETH_PACKET_MAX_NOVLAN_SIZE, 0, (struct sockaddr*)&pkt_info,
            (socklen_t *)&pkt_nfo_sz)) < 0)
    {
        perror ("Recv from returned error: ");
        return MME_ERROR_TXRX;
    }
#endif // __linux__
    return MME_SUCCESS;
}


void
mme_remove_header (unsigned short mme_type, unsigned char *recv_pkt,
                                            int pkt_len, mme_ctx_t *ctx)
{
    unsigned int result_len;
    mme_error_t ret;
    /* Get the MME Header size */
    uint mme_h_size; /* Size of MME header */
    SET_MME_HEADER_SIZE (mme_type, mme_h_size);

    recv_pkt += mme_h_size;
    pkt_len -= mme_h_size;

    /* copy the packet into the receive buffer */
    ret = mme_put (ctx, recv_pkt, pkt_len, &result_len);

    if (ret != MME_SUCCESS)
    {
        LOG_PRINT (LOG_VERBOSE, "mme_put() error. ret = %d\n", ret);
    }
}


int
mme_check_dest_mac (unsigned char *dest_mac, uint8_t *mac)
{
    if (memcmp (dest_mac, mac_broadcast, sizeof (mac_broadcast)) == 0)
    {
        LOG_PRINT (LOG_VERBOSE, "your MAC address is broadcast address\n");
        return MME_SUCCESS;
    }
    if (memcmp (dest_mac, mac, 6 * sizeof (*mac)) != 0)
    {
        LOG_PRINT (LOG_VERBOSE,
            "MAC address in received MME is not same as my own!\n");
        return MME_ERROR_RESULT;
    }
    else
    {
        return MME_SUCCESS;
    }
}
