#ifndef slac_inc_evse_slac_h
#define slac_inc_evse_slac_h
/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */
#include "libmme.h"
#include "slacParameter.h"
#include "node.h"
#include "timer.h"
#include "mstar_vs.h"
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#include <BaseTsd.h>
#include <Winuser.h>
#endif
#ifdef __linux__
#include <unistd.h>
#include <getopt.h>
#endif

/**
 * Set NID and NMK for EVSE.
 * \param  opened_interface  selected communication interface
 * \param  evse  evse information
 * \param  dst  destination MAC address
 * \return  0 on success, -1 on failure
 */
int
evse_cm_set_key_req_cnf (void *opened_interface, evse *evse, const unsigned char *dst);


/**
 * Send CM_SLAC_PARM.REQ and receive CM_SLAC_PARM.CNF.
 * \param  opened_interface  selected communication interface
 * \param  evse  evse information
 * \param  req_ctx  MME context where to get data
 * \param  dst Destination MAC adress
 * \return  0 on success, -1 on failure
 */
int
evse_cm_slac_parm_req_cnf (void *opened_interface, evse *evse,
                   mme_ctx_t *req_ctx, unsigned char *dst);


/**
 * Receive CM_START_ATTEN_CHAR.IND.
 * \param  ind_ctx  MME context where to get data
 * \param  dst Destination MAC adress
 * \return  0 on success, -1 on failure
 */
int
evse_cm_start_atten_char_ind_receive
                         (mme_ctx_t *ind_ctx, unsigned char *dst);


/**
 * Receive a MNBC SOUND.
 * \param  ind_ctx  MME context where to get data
 * \return  1 if cnounter = 0, 0 on success, -1 on failure
 */
int
evse_cm_mnbc_sound_ind_receive (mme_ctx_t *ind_ctx);


/**
 * Receive CM_ATTEN_PROFILE.IND.
 * \param  evse  evse information
 * \param  ind_ctx  MME context where to get data
 * \return  0 on success, -1 on failure
 */
int
evse_cm_atten_profile_ind_receive (evse *evse, mme_ctx_t *ind_ctx);


/**
 * Receive CM_ATTEN_CHAR.RSP.
 * \param  evse  evse information
 * \param  req_ctx  MME context where to get data
 * \return  0 on success, -1 on failure
 */
int
evse_cm_atten_char_rsp_receive (evse *evse, mme_ctx_t *ind_ctx);


/**
 * Send CM_SLAC_MATCH.REQ and receive CM_SLAC_MATCH.CNF.
 * \param  opened_interface  selected communication interface
 * \param  req_ctx  MME context where to get data
 * \param  evse  evse information
 * \param  pev  pev_item information
 * \return  0 on success, -1 on failure
 */
int
evse_cm_slac_match_req_receive_cnf_send (void *opened_interface,
                                 mme_ctx_t *req_ctx, evse *evse, pev_item *pev);


/**
 * Print test result.
 * \param  evse  evse information
 * \param  error_flag  indicate if the test is successful
 */
void
test_result_display (evse *evse, int error_flag);


/**
 * main program of EVSE
 * \parm  iname  interface name
 * \parm  nid  NID that you want to set
 * \parm  nmk  NMK that you want to set
 * \parm  alias_mac_compatible  indicate whether to set key with other specific alias mac address
 * \parm  debug  print debug information if set this flag
 * \parm  loop  execute the program constantly if set this flag
 * \parm  exec_disconnection  execute disconnection after SLAC if set this flag
 * \return 0 on success, -1 on failure
 */
int
evse_main (char *iname, unsigned char nid[7], unsigned char nmk[16],
    int alias_mac_compatible, int debug, int loop, int exec_disconnection);


/**
 * print hlep message for executing pev_main
 */
void
evse_print_usage ();
#endif /* slac_inc_evse_slac_h */