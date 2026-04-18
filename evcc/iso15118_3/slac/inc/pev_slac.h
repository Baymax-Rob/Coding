#ifndef slac_inc_pev_slac_h
#define slac_inc_pev_slac_h
/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
#include "libmme.h"
#include "slacParameter.h"
#include "timer.h"
#include "mstar_vs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <unistd.h>
#include <getopt.h>
#endif // __linux__


#define DISCONNECTED 0
#define UNMATCHED 1
#define MATCHED 2

/**
 * Set NID and NMK for PEV to connect to EVSE.
 * \param  opened_interface  selected communication interface
 * \param  pev  pev information
 * \param  dst  destination MAC address
 * \return  0 on success, -1 on failure
 */
int
pev_cm_set_key_req_send (void *opened_interface, pev *pev,
    const unsigned char *dst);


/**
 * Set NID and NMK for PEV to disconnect with EVSE.
 * \param  opened_interface  selected communication interface
 * \param  pev  pev information
 * \param  dst  destination MAC address
 * \return  0 on success, -1 on failure
 */
int
pev_cm_set_key_req_disconnect (void *opened_interface, pev *pev,
    const unsigned char *dst);


/**
 * Set NID and NMK for PEV to connect to EVSE.
 * \param  opened_interface  selected communication interface
 * \param  pev  pev information
 * \return  0 on success
 */
int
pev_cm_slac_parm_req_send (void *opened_interface, pev *pev);


/**
 * Receive CM_SLAC_PARM.CNF
 * \param  opened_interface  selected communication interface
 * \param  pev  pev information
 * \return  0 on success
 */
int
pev_cm_slac_parm_cnf_receive (void *opened_interface, pev *pev);


/**
 * Send CM_START_ATTEN_CHAR.IND
 * \param  opened_interface  selected communication interface
 * \return  0 on success
 */
int
pev_cm_start_atten_char_ind_send (void *opened_interface);


/**
 * Send CM_MNBC_SOUND.IND
 * \param  opened_interface  selected communication interface
 * \return  0 on success
 */
int
pev_cm_mnbc_sound_ind_send (void *opened_interface);


/**
 * Receive CM_ATTEN_CHAR.IND
 * \param  opened_interface  selected communication interface
 * \param  pev  pev information
 * \param  atten_ind  MME context where to get data
 * \param  src  Source MAC adress
 * \return  0 on success
 */
int
pev_cm_atten_char_ind_receive (void *opened_interface, pev *pev,
                             atten_char_ind *atten_ind, unsigned char *src);


/**
 * Send CM_ATTEN_CHAR.RSP
 * \param  iface  selected communication interface
 * \param  atten_ind  MME context where to get data
 * \param  dst Destination MAC adress
 * \return  0 on success
 */
int
pev_cm_atten_char_rsp_send (void *opened_interface,
                         atten_char_ind *atten_ind, unsigned char *dst);


/**
 * Send CM_SLAC_MATCH.REQ
 * \param  opened_interface  selected communication interface
 * \param  evse evse with which we want to match
 * \return  0 on success
 */
int
pev_cm_slac_match_req_send (void *opened_interface, evse_item *evse);


/**
 * Receive CM_SLAC_MATCH.CNF
 * \param  opened_interface  selected communication interface
 * \param  pev  pev information
 * \return  0 on success
 */
int
pev_cm_slac_match_cnf_receive (void *opened_interface, pev *pev);


/**
 * main program of PEV
 * \param  iname  interface name
 * \param  nid  NID that you want to set
 * \param  nmk  NMK that you want to set
 * \param  test_flag  MSTAR internal test flag
 * \param  alias_mac_compatible  indicate whether to set key with other specific alias mac address
 * \param  debug  print debug information if set this flag
 * \param  loop  execute the program constantly if set this flag
 * \param  exec_disconnection  execute disconnection after SLAC if set this flag
 * \return 0 on success, -1 on failure
 */
int
pev_main (char *iname, unsigned char nid[7], unsigned char nmk[16],
    int test_flag, int alias_mac_compatible, int debug, int loop,
    int exec_disconnection);


/**
 * print hlep message for executing pev_main
 */
void
pev_print_usage ();
#endif /* slac_inc_pev_slac_h */