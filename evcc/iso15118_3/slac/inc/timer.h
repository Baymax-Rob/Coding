#ifndef slac_inc_timer_h
#define slac_inc_timer_h
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
#include "mstar_vs.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#ifdef __linux__
#include <sys/time.h>
#endif

#ifdef _WIN32
/**
 * Get System time (Windows specific).
 * \parm  sys_time  Windows system time data structure
 */
int
slac_get_sys_time(struct hpav_sys_time* sys_time);
#endif

/**
 * list of errors returned by timer functions
 */
 typedef enum
{
    TIMER_SUCCESS = 0,
    TIMER_ERROR = -1
} timer_error_t;

void
sleep_ms (int milliseconds);

/**
 * Print the current time.
 */
void
current_time ();

/**
 * Send CM_ATTEN_CHAR.IND.
 * \param  iface  selected communication interface
 * \param  pev  pev information
 */
void
cm_atten_char_ind_send (void *opened_interface, pev_item *pev);

/**
 * Set timer and call handler when time up.
 * \param  pev_status  PEV information
 * \param  expire_ms  first expire time
 * \param  interval_ms  interval time between each timer
 * \return  1 on success
 */
int
make_pev_timer
(pev *pev_status, int expire_ms, int interval_ms);

/**
 * Set timer and call handler when time up.
 * \param  pev  PEV_item information
 * \param  expire_ms  first expire time
 * \param  interval_ms  interval time between each timer
 * \return  1 on success
 */
int
make_evse_timer
(pev_item *pev, int expire_ms, int interval_ms);
#endif /* slac_inc_timer_h */
