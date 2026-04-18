/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */

#include "node.h"
#include "slacParameter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <linux/if_ether.h>
#endif

pev_item *
create_pev_item (uint8_t *run_id, uint8_t *mac, void *opened_interface)
{
#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
#endif // __linux__
#ifdef _WIN32
    hpav_channel_t *return_chan = (hpav_channel_t *)opened_interface;
#endif // _WIN32

    pev_item *new_node = (pev_item *)malloc (sizeof (pev_item));
    memcpy (new_node->run_id, run_id, 8 * sizeof (*run_id));
    memcpy (new_node->mac, mac, 6 * sizeof (*mac));
    int i;
    for (i = 0; i < 58; i++)
    {
        new_node->AAG[i] = 0;
    }
    new_node->cm_start_atten_char_ind_timer_retry = C_EV_match_retry;
    new_node->matching_state = 0;
    new_node->receive_sounds = 0;
#ifdef _WIN32
    new_node->return_chan = return_chan;
#endif // _WIN32
#ifdef __linux__
    new_node->iface = iface;
#endif // __linux__
    new_node->next = NULL;
    return new_node;
}

evse_item *
create_evse_item (uint8_t *mac)
{
    evse_item *new_node = (evse_item *)malloc (sizeof (evse_item));
    memcpy (new_node->mac, mac, 6 * sizeof (*mac));
    new_node->send_cn_atten_char_ind_flag = 0;
    /**
     * Assign a initial value in case that
     * this EVSE didn't response Attenuation value within CM_ATTEN_CHAR.IND.
     */
    new_node->AAG = 100;
    new_node->next = NULL;
    return new_node;
}

void
insert_pev_item (struct pev_item *head, uint8_t *run_id,
                            uint8_t *mac, void *opened_interface)
{
    pev_item *tmp = create_pev_item (run_id, mac, opened_interface);
    pev_item *current = head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = tmp;
}

void
insert_evse_item (struct evse_item *head, uint8_t *mac)
{
    evse_item *tmp = create_evse_item (mac);
    evse_item *current = head;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = tmp;
}

void
print_pev_item_list (struct pev_item *head)
{
    int i, num=0;

    pev_item *current = head;
    LOG_PRINT (LOG_VERBOSE, "print_pev_item_list below: \n");
    while (current->next != NULL)
    {
        current = current->next;

        LOG_PRINT (LOG_VERBOSE, "pev[%d]->MAC addr= ", num);
        for (i = 0; i < 6; i++)
        {
            LOG_PRINT (LOG_VERBOSE, "0x%02x ", current->mac[i]);
        }
        LOG_PRINT (LOG_VERBOSE, "\n");

        LOG_PRINT (LOG_VERBOSE, "pev[%d]->run_id= ", num++);
        for (i = 0; i < 8; i++)
        {
            LOG_PRINT (LOG_VERBOSE, "%02x ", current->run_id[i]);
        }
        LOG_PRINT (LOG_VERBOSE, "\n");
    }
}

void
print_evse_item_list (struct evse_item *head)
{
    int i, num = 0;
    evse_item *current = head;

    while (current->next != NULL)
    {
        current = current->next;

        LOG_PRINT (LOG_VERBOSE, "evse[%d]->MAC addr = ", num++);
        for (i = 0; i < 6; i++)
        {
            LOG_PRINT (LOG_VERBOSE, "0x%02x ", current->mac[i]);
        }
        LOG_PRINT (LOG_VERBOSE, "\n");
    }
}

void
update_pev_item (pev_item *pev, uint8_t *run_id, uint8_t *mac, void *opened_interface)
{
#ifdef __linux__
    iface_t *iface = (iface_t *)opened_interface;
#endif // __linux__
#ifdef _WIN32
    hpav_channel_t *return_chan = (hpav_channel_t *)opened_interface;
#endif // _WIN32

    memcpy (pev->run_id, run_id, 8 * sizeof (*run_id));
    memcpy (pev->mac, mac, 6 * sizeof (*mac));
    int i;
    for (i = 0; i < 58; i++)
    {
        pev->AAG[i] = 0;
    }
    pev->cm_start_atten_char_ind_timer_retry = C_EV_match_retry;
    pev->matching_state = 0;
    pev->receive_sounds = 0;
#ifdef _WIN32
    pev->return_chan = return_chan;
#endif // _WIN32
#ifdef __linux__
    pev->iface = iface;
#endif // __linux__
}

pev_item *
search_pev_item_list (struct pev_item *head, uint8_t *mac)
{
    pev_item *current = head;
    while (current->next != NULL)
    {
        current = current->next;

        if (memcmp (current->mac, mac, 6 * sizeof (*mac)) == 0)
        {
            LOG_PRINT (LOG_VERBOSE, "PEV node was found in the list\n");
            return current;
        }
    }
    LOG_PRINT (LOG_VERBOSE, "PEV node was not found!!\n");
    return NULL;
}

evse_item *
search_evse_item_list (struct evse_item *head, uint8_t *mac)
{
    evse_item *current = head;
    while (current->next != NULL)
    {
        current = current->next;
        if (memcmp (current->mac, mac, 6 * sizeof (*mac)) == 0)
        {
            LOG_PRINT (LOG_VERBOSE, "EVSE found in the list\n");
            return current;
        }
    }
    LOG_PRINT (LOG_VERBOSE, "EVSE not found!!\n");
    return NULL;
}

evse_item *
select_evse_item_list (struct evse_item *head)
{
    evse_item *current = head, *target = NULL;
    int min_AAG = 100;

    while (current->next != NULL)
    {
        current = current->next;
        if (current->AAG < min_AAG)
        {
            min_AAG = current->AAG;
            target = current;
        }
    }
    return target;
}