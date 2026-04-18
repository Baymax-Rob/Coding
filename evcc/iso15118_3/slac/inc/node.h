#ifndef slac_inc_node_h
#define slac_inc_node_h
/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
#include "libmme.h"
#include "slacParameter.h"
#include <stdint.h>
#include <time.h>
#ifdef __linux__
#include <sys/queue.h>
#endif
#ifdef _WIN32
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
typedef int timer_t;
#endif

/**
* This struct is used by PEV to manage its current state
*/
typedef enum
{
    SENT_CM_SLAC_PARM_REQ,
    RECEIVED_CM_SLAC_PARM_CNF,
    SENT_CM_START_ATTEN_CHAR_IND,
    RECEIVED_CM_ATTEN_CHAR_IND,
    RECEIVED_CM_SLAC_MATCH_CNF
} pev_state;

/**
 * This struct is used by EVSE to manage each potential PEVs
 */
typedef enum
{
    RECEIVED_CM_SLAC_PARM_REQ,
    RECEIVED_CM_START_ATTEN_CHAR_IND,
    SENT_CM_ATTEN_CHAR_IND,
    RECEIVED_CM_ATTEN_CHAR_RSP,
    RECEIVED_CM_SLAC_MATCH_REQ,
    SENT_CM_SLAC_MATCH_CNF
} PEV_matching_state;

/**
 * This struct is used by PEV to manage each potential EVSEs
 */
typedef struct evse_item
{
    uint8_t mac[ETH_ALEN];
    int send_cn_atten_char_ind_flag;
    int AAG;
    struct evse_item *next;
} evse_item;

/**
 * This struct is used by EVSE to manage each potential PEVs
 */
typedef struct pev_item
{
    uint8_t run_id[8];
    uint8_t mac[ETH_ALEN];
    int matching_state;
    int receive_sounds;
    int AAG[58];
    int cm_start_atten_char_ind_timer_retry;

    /* Pointer interface_ptr is used to check if network channel was already closed. */
    int *interface_ptr;

#ifdef __linux__
    timer_t cm_start_atten_char_ind_timer_id;
    timer_t cm_slac_match_cnf_timer_id;
    iface_t *iface;
#endif // __linux__

#ifdef _WIN32
    hpav_channel_t *return_chan;

    pthread_t cm_start_atten_char_ind_timer_id[C_EV_match_retry];
    pthread_t cm_slac_match_cnf_timer_id;
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    int expire_sec[C_EV_match_retry];
    int expire_msec[C_EV_match_retry];
#endif // _WIN32

    struct pev_item *next;
} pev_item;

typedef struct pev
{
    int state;
    uint8_t run_id[8];
    uint8_t nid[7];
    uint8_t nmk[16];
    uint8_t received_nid[7];
    uint8_t received_nmk[16];
    pev_state current_state;

    int cm_slac_parm_timer_flag;
    int cm_slac_parm_req_retry;

    int cm_start_atten_char_timer_flag;
    int cm_slac_match_req_retry;
    int potential_EVSE;
    struct evse_item *evse_item_head;
#ifdef __linux__
    timer_t cm_slac_parm_timer_id;
    timer_t cm_start_atten_char_timer_id;
#endif // __linux__
#ifdef _WIN32
    pthread_t cm_slac_parm_timer_id;
    pthread_t cm_start_atten_char_timer_id;
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    int expire_sec;
    int expire_msec;
#endif // _WIN32

} pev;

typedef struct evse
{
    uint8_t nid[7];
    uint8_t nmk[16];
    struct pev_item *pev_item_head;
} evse;


/**
 * create EVSE_item for PEV to manage potential EVSE
 * \param  mac MAC Address for EVSE_item
 * \return  created EVSE_item
 */
evse_item *
create_evse_item (uint8_t *mac);

/**
 * create PEV_item for EVSE tp manage potential PEV
 * \param  run_id run ID for PEV_item
 * \param  mac MAC Address for PEV_item
 * \param  opened_interface  selected communication interface
 * \return  created PEV_item
 */
pev_item *
create_pev_item (uint8_t *run_id, uint8_t *mac, void *opened_interface);

/**
 * Update PEV_item
 * \param  pev PEV that you want to update its information
 * \param  run_id run ID for PEV_item
 * \param  mac MAC Address for PEV_item
 * \param  iface  selected communication interface
 */
void
update_pev_item (pev_item *pev, uint8_t *run_id, uint8_t *mac, void *opened_interface);

/**
 * insert EVSE_item into EVSE List
 * \param  head  head of EVSE list
 * \param  mac  MAC Address for EVSE_item
 */
void
insert_evse_item (evse_item *head, uint8_t *mac);

/**
 * insert PEV_item into PEV List
 * \param  head  head of EVSE list
 * \param  run_id run ID for PEV_item
 * \param  mac  MAC Address for EVSE_item
 * \param  opened_interface  selected communication interface
 */
void
insert_pev_item (pev_item *head,
    uint8_t *run_id, uint8_t *mac, void *opened_interface);

/**
 * print each EVSE_item in the EVSE List
 * \param  head  head of EVSE list
 */
void
print_evse_item_list (evse_item *head);

/**
 * print each PEV_item in the PEV List
 * \param  head  head of PEV list
 */
void
print_pev_item_list (pev_item *head);

/**
 * Search the EVSE with the specific MAC Address
 * \param  head  head of EVSE list
 * \param  mac  MAC Address for EVSE_item
 * \return  EVSE_item with that specific MAC Address, NULL if not found
 */
evse_item *
search_evse_item_list (evse_item *head, uint8_t *mac);

/**
 * Search the PEV with the specific MAC Address
 * \param  head  head of PEV list
 * \param  mac  MAC Address for PEV_item
 * \return  PEV_item with that specific MAC Address, NULL if not found
 */
pev_item *
search_pev_item_list (pev_item *head, uint8_t *mac);

/**
 * Select the EVSE with the min AAG from EVSE List
 * \param  head  head of EVSE list
 * \param  mac  MAC Address for PEV_item
 * \return  PEV_item with the min AAG
 */
evse_item *
select_evse_item_list (evse_item *head);
#endif /* slac_inc_node_h */
