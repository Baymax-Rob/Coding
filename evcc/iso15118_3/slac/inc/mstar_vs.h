/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */

#include <stdio.h>
#include <string.h>
#include "libmme.h"


/**
 * Send VS_GET_STATUS.REQ
 * \param  opened_interface  selected communication interface
 * \return  error type MME_SUCCESS if success
 */
int
vs_get_status_req_send  (void *opened_interface);

/**
 * Receive VS_GET_STATUS.CNF
 * \param  opened_interface  selected communication interface
 * \return  0/1 on Link state disconnected/connected, -1 on failure
 */
int
vs_get_status_cnf_receive (void *opened_interface);

/**
 * Send VS_GET_STATUS.REQ and receive VS_GET_STATUS.CNF
 * \param  opened_interface  selected communication interface
 * \return  0/1 on Link state disconnected/connected, -1 on failure
 */
int
vs_get_status_req_send_cnf_receive (void *opened_interface);
