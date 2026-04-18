/*  plc hle utils project {{{
 *
 * Copyright (C) 2018 MStar Semiconductor
 *
 * <<<Licence>>>
 *
 * }}} */

#include "timer.h"

const int attenuation_group = 58;
#ifdef _WIN32
#define SIGRTMIN 1
typedef int siginfo_t;
#endif

void
sleep_ms (int milliseconds)
{
#ifdef __linux__
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep (&ts, NULL);
#endif
#ifdef _WIN32
    Sleep (milliseconds);
#endif // _WIN32
}


void
current_time ()
{
#ifdef __linux__
    struct timeval  tv;
    struct timezone tz;
    struct tm *tm;

    gettimeofday (&tv, &tz);
    tm = localtime (&tv.tv_sec);

    // tv_usec is microsecond
    LOG_PRINT (LOG_VERBOSE, " %d:%02d:%02d %ld %ld \n", tm->tm_hour, tm->tm_min,
        tm->tm_sec, tv.tv_usec / 1000, tv.tv_usec % 1000);
#endif // __linux__
}


void
cm_atten_char_ind_send (void *opened_interface, pev_item *pev)
{
    mme_ctx_t ind_ctx;
    unsigned char send_buff[ETH_DATA_LEN];
    unsigned int buff_len = 0;
    mme_error_t result;
    int i;

    mme_init (&ind_ctx, CM_ATTEN_CHAR_IND, send_buff, ETH_DATA_LEN);

    atten_char_ind atten_ind =
    {
        .application_type = 0,
        .security_type = 0,
        .ACVarField.num_sounds = pev->receive_sounds,
        .ACVarField.ATTEN_PROFILE.num_groups = attenuation_group
    };
    memcpy (atten_ind.ACVarField.source_address,
                                    pev->mac, sizeof (pev->mac));
    memcpy (atten_ind.ACVarField.run_id, pev->run_id, sizeof (pev->run_id));
    for (i = 0; i < attenuation_group; i++)
    {
        atten_ind.ACVarField.ATTEN_PROFILE.AAG[i] =
            pev->AAG[i] / pev->receive_sounds;
    }

    mme_put (&ind_ctx, &atten_ind, sizeof (atten_ind), &buff_len);

    LOG_PRINT (LOG_INFO, "[EVSE] sends CM_ATTEN_CHAR.IND\n");
    result =
        mme_send (opened_interface, &ind_ctx, MME_SEND_IND_RSP, pev->mac);
    if (result != MME_SUCCESS)
    {
        LOG_PRINT (LOG_VERBOSE, "mme_send() error, result = %d\n", result);
    }
}



/**
 * The following handler are called by PEV.
 */
#ifdef __linux__
static void
sent_cm_slac_parm_req_handler (int sig, siginfo_t *si, void *uc)
{

    pev *pev = si->si_value.sival_ptr;
    if (DEBUG_DISPLAY)
    {
        LOG_PRINT (LOG_VERBOSE, "CM_SLAC_PARM_REQ Timer stop at ");
        current_time ();
    }
    pev->cm_slac_parm_timer_flag = 0;
}
#endif

#ifdef _WIN32

int
gettimeofday (struct timeval * tp)
{
    /**
     * Note: some broken versions only have 8 trailing zero's,
     * the correct epoch has 9 trailing zero's.
     * This magic number is the number of 100 nanosecond intervals
     * since January 1, 1601 (UTC) until 00:00:00 January 1, 1970
     */
    static const uint64_t EPOCH = ( (uint64_t)116444736000000000ULL);

    SYSTEMTIME system_time;
    FILETIME file_time;
    uint64_t time;

    GetSystemTime (&system_time);
    SystemTimeToFileTime (&system_time, &file_time);
    time = ( (uint64_t)file_time.dwLowDateTime);
    time += ( (uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long) ( (time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}


void
*cm_slac_parm_req_handler (void *arg)
{
    pev *pev = (struct pev *) arg;
    struct hpav_sys_time start_time, end_time;

    struct timeval now;
    struct timespec outtime;

    get_sys_time (&start_time);
    gettimeofday (&now, NULL);

    pthread_cleanup_push (pthread_mutex_unlock, &pev->mutex);
    pthread_mutex_lock (&pev->mutex);

    outtime.tv_sec = now.tv_sec + pev->expire_sec;
    outtime.tv_nsec = (now.tv_usec + (1000UL * pev->expire_msec)) * 1000UL;
    outtime.tv_sec += outtime.tv_nsec / 1000000000UL;
    outtime.tv_nsec = outtime.tv_nsec % 1000000000UL;

    pthread_cond_timedwait (&pev->cond, &pev->mutex, &outtime);

    pev->cm_slac_parm_timer_flag = 0;

    get_sys_time (&end_time);
    int wait_time = get_elapsed_time_ms (&start_time, &end_time);

    pthread_mutex_unlock (&pev->mutex);
    pthread_cleanup_pop (0);

    return 0;
}


void
*cm_atten_char_ind_handler (void* arg)
{
    pev *pev = (struct pev *) arg;
    struct hpav_sys_time start_time, end_time;

    struct timeval now;
    struct timespec outtime;

    get_sys_time (&start_time);
    gettimeofday (&now, NULL);

    pthread_cleanup_push (pthread_mutex_unlock, &pev->mutex);
    pthread_mutex_lock (&pev->mutex);

    outtime.tv_sec = now.tv_sec + pev->expire_sec;
    outtime.tv_nsec = (now.tv_usec + (1000UL * pev->expire_msec)) * 1000UL;
    outtime.tv_sec += outtime.tv_nsec / 1000000000UL;
    outtime.tv_nsec = outtime.tv_nsec % 1000000000UL;

    pthread_cond_timedwait (&pev->cond, &pev->mutex, &outtime);

    pev->cm_start_atten_char_timer_flag = 0;

    get_sys_time (&end_time);
    int wait_time = get_elapsed_time_ms (&start_time, &end_time);

    pthread_mutex_unlock (&pev->mutex);
    pthread_cleanup_pop (0);

    return 0;
}
#endif // _WIN32


#ifdef __linux__
static void
received_cm_atten_char_ind_handler (int sig, siginfo_t *si, void *uc)
{
    pev *pev = si->si_value.sival_ptr;
    if (DEBUG_DISPLAY)
    {
        LOG_PRINT (LOG_VERBOSE, "CM_START_ATTEN_CHAR.IND Timer stop at ");
        current_time ();
    }
    pev->cm_start_atten_char_timer_flag = 0;
}
#endif

/**
 * The following handler are called by EVSE.
 */
#ifdef __linux__
static void
received_cm_start_atten_char_ind_handler (int sig, siginfo_t *si, void *uc)
{
    pev_item *pev = si->si_value.sival_ptr;
    if (DEBUG_DISPLAY)
    {
        LOG_PRINT (LOG_VERBOSE, "received_CM_START_ATTEN_CHAR_IND_Handler \
 stop at ");
        current_time ();
    }

    if (pev->matching_state == RECEIVED_CM_START_ATTEN_CHAR_IND)
    {
        pev->matching_state = SENT_CM_ATTEN_CHAR_IND;
        // Need to send CM_ATTEN_CHAR.IND
        cm_atten_char_ind_send (pev->iface, pev);
    }

    pev->cm_start_atten_char_ind_timer_retry--;
    if (pev->cm_start_atten_char_ind_timer_retry<=0)
    {
        if (timer_delete (pev->cm_start_atten_char_ind_timer_id) == 0)
        {
            LOG_PRINT (LOG_VERBOSE,
                "Delete CM_START_ATTEN_CHAR_IND_TIMER successfully.\n");
        }
        else
        {
            LOG_PRINT (LOG_VERBOSE,
                "CM_START_ATTEN_CHAR_IND_TIMER was already deleted.\n");
        }
    }
}

static void
sent_cm_slac_match_cnf_handler (int sig, siginfo_t *si, void *uc)
{

    pev_item *pev = si->si_value.sival_ptr;

    LOG_PRINT (LOG_VERBOSE, "received_cm_slac_match_cnf_handler \
stop at ");
    current_time ();

    vs_get_status_req_send(pev->iface);
}
#endif

#ifdef _WIN32
void
*cm_start_atten_char_ind_handler (void *arg)
{
    pev_item *pev = (struct pev_item *) arg;
    int retry = C_EV_match_retry - pev->cm_start_atten_char_ind_timer_retry;
    pev->cm_start_atten_char_ind_timer_retry--;

    struct hpav_sys_time start_time, end_time;
    struct timeval now;
    struct timespec expire_time;

    get_sys_time (&start_time);
    gettimeofday (&now, NULL);

    pthread_cleanup_push (pthread_mutex_unlock, &pev->mutex);
    pthread_mutex_lock (&pev->mutex);

    expire_time.tv_sec = now.tv_sec + pev->expire_sec[retry];
    expire_time.tv_nsec =
        (now.tv_usec + (1000UL * pev->expire_msec[retry])) * 1000UL;
    expire_time.tv_sec += expire_time.tv_nsec / 1000000000UL;
    expire_time.tv_nsec = expire_time.tv_nsec % 1000000000UL;

    pthread_cond_timedwait (&pev->cond, &pev->mutex, &expire_time);
    if (pev->matching_state == RECEIVED_CM_START_ATTEN_CHAR_IND)
    {
        pev->matching_state = SENT_CM_ATTEN_CHAR_IND;
        cm_atten_char_ind_send (pev->return_chan, pev);
    }
    else
    {
        LOG_PRINT (LOG_VERBOSE,
        "cm_start_atten_char_ind_handler didn't send CM_ATTEN_CHAR.IND\n");
    }

    get_sys_time (&end_time);
    int wait_time = get_elapsed_time_ms (&start_time, &end_time);

    pthread_mutex_unlock (&pev->mutex);
    pthread_cleanup_pop (0);

    return 0;
}

void
*cm_slac_match_cnf_handler (volatile void *arg)
{
    pev_item *pev = (struct pev_item *) arg;
    struct hpav_sys_time start_time, end_time;

    struct timeval now;
    struct timespec outtime;

    get_sys_time (&start_time);

    gettimeofday (&now, NULL);

    pthread_cleanup_push (pthread_mutex_unlock, &pev->mutex);
    pthread_mutex_lock (&pev->mutex);

    outtime.tv_sec = now.tv_sec + pev->expire_sec[0];
    outtime.tv_nsec =
        (now.tv_usec + (1000UL * pev->expire_msec[0])) * 1000UL;
    outtime.tv_sec += outtime.tv_nsec / 1000000000UL;
    outtime.tv_nsec = outtime.tv_nsec % 1000000000UL;

    pthread_cond_timedwait (&pev->cond, &pev->mutex, &outtime);

    /* interface_ptr = 0 means that network channel was already closed. */
    if (*pev->interface_ptr == 0)
    {
        LOG_PRINT (LOG_VERBOSE, "Network channel was already closed \
so we won't send VS_GET_STATUS.REQ.\n");
        return 0;
    }
    vs_get_status_req_send (pev->return_chan);

    get_sys_time (&end_time);
    int wait_time = get_elapsed_time_ms (&start_time, &end_time);

    pthread_mutex_unlock (&pev->mutex);
    pthread_cleanup_pop (0);

    return 0;
}
#endif // _WIN32


int
make_pev_timer
(pev *pev_status, int expire_ms, int interval_ms)
{
    // first expire time
    // [Notice: tv_nsec cannot exceed one second]
    int expire_sec = expire_ms / 1000;
    int expire_msec = expire_ms % 1000;

    // the period of timer
    int interval_sec = interval_ms / 1000;
    int interval_msec = interval_ms % 1000;

#ifdef _WIN32

    void(*pthread_handler)(void *) = NULL;

    pev_status->expire_sec = expire_sec;
    pev_status->expire_msec = expire_msec;

    pthread_mutex_init (&pev_status->mutex, NULL);
    pthread_cond_init (&pev_status->cond, NULL);

    pthread_t *timer_id;
    // set the corresponding handler according to the pev.current_state
    switch (pev_status->current_state)
    {
    case 0:
        timer_id = &pev_status->cm_slac_parm_timer_id;
        pthread_handler = cm_slac_parm_req_handler;
        break;
    // this case start from send CM_START_ATTEN_CHAR.IND
    case 1:
        timer_id = &pev_status->cm_start_atten_char_timer_id;
        pthread_handler = cm_atten_char_ind_handler;
        break;
    default:
        LOG_PRINT (LOG_VERBOSE, "Unknown current_state\n");
        return TIMER_ERROR;
    }

    if (pthread_create (timer_id, NULL, *pthread_handler, (void *)pev_status) != 0 )
    {
        LOG_PRINT (LOG_VERBOSE, "error when create pthread: %d\n", errno);
        return TIMER_ERROR;
    }

#endif
#ifdef __linux__
    timer_t *timer_id;
    struct sigevent te;
    struct itimerspec its;
    struct sigaction sa;
    int sig_no = SIGRTMIN;


    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO;

    // set the corresponding handler according to the pev.current_state
    switch (pev_status->current_state)
    {
    case 0:
        timer_id = &pev_status->cm_slac_parm_timer_id;
        sa.sa_sigaction = sent_cm_slac_parm_req_handler;
        break;
    // this case start from send CM_START_ATTEN_CHAR.IND
    case 1:
        timer_id = &pev_status->cm_start_atten_char_timer_id;
        sa.sa_sigaction = received_cm_atten_char_ind_handler;
        break;
    default:
        LOG_PRINT (LOG_VERBOSE, "Unknown current_state\n");
        return TIMER_ERROR;
    }

    sigemptyset (&sa.sa_mask);
    if (sigaction (sig_no, &sa, NULL) == -1)
    {
        LOG_PRINT (LOG_DEBUG, "Error sigaction\n");
        return TIMER_ERROR;
    }

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sig_no;
    te.sigev_value.sival_ptr = pev_status;
    if (timer_create(CLOCK_REALTIME, &te, timer_id) )
    {
        LOG_PRINT (LOG_DEBUG, "timer_create() error\n");
        return TIMER_ERROR;
    }


    its.it_value.tv_sec = expire_sec;
    its.it_value.tv_nsec = expire_msec * 1000000;


    its.it_interval.tv_sec = interval_sec;
    its.it_interval.tv_nsec = interval_msec * 1000000;

    if (timer_settime (*timer_id, 0, &its, NULL))
    {
        LOG_PRINT (LOG_DEBUG, "timer_settime() error\n");
        return TIMER_ERROR;
    }
    if (DEBUG_DISPLAY)
    {
        LOG_PRINT (LOG_VERBOSE, "Set the timer %d at ", (int)*timer_id);
        current_time ();
    }
#endif
    return TIMER_SUCCESS;
}


int
make_evse_timer
(pev_item *pev, int expire_ms, int interval_ms)
{
    // first expire time
    // [Notice: tv_nsec cannot exceed one second]
    int expire_sec = expire_ms / 1000;
    int expire_msec = expire_ms % 1000;

    // the period of timer
    int interval_sec = interval_ms / 1000;
    int interval_msec = interval_ms % 1000;

#ifdef _WIN32
    int i;

    void(*pthread_handler)(void *) = NULL;

    pev->expire_sec[0] = expire_sec;
    pev->expire_msec[0] = expire_msec;

    pthread_mutex_init (&pev->mutex, NULL);
    pthread_cond_init (&pev->cond, NULL);

    pthread_t *timer_id;
    // set the corresponding handler according to the pev.current_state
    switch (pev->matching_state)
    {
    // this case start from receive CM_START_ATTEN_CHAR.IND.
    case 1:
        timer_id = &pev->cm_start_atten_char_ind_timer_id[0];
        pthread_handler = cm_start_atten_char_ind_handler;
        break;
    case 5:
        timer_id = &pev->cm_slac_match_cnf_timer_id;
        pthread_handler = cm_slac_match_cnf_handler;
        break;
    default:
        LOG_PRINT (LOG_VERBOSE, "Unknown current_state\n");
        return TIMER_ERROR;
    }

    if (pthread_create (timer_id, NULL, *pthread_handler, (void *)pev) != 0)
    {
        LOG_PRINT (LOG_DEBUG, "error when create pthread: %d\n", errno);
        return -1;
    }

    /**
     * The following code only happen when sending CM_ATTEN_CHAR_IND.
     */
    if (interval_ms > 0)
    {
        for (i = 1; i < C_EV_match_retry; i++)
        {
            /**
             * For each retry thread, the waiting time should be
             * expire_time plus interval_time.
             */
            int interval_total_ms = (interval_ms * i) + expire_ms;
            pev->expire_sec[i] = interval_total_ms / 1000;
            pev->expire_msec[i] = interval_total_ms % 1000;

            timer_id = &pev->cm_start_atten_char_ind_timer_id[i];
            pthread_handler = cm_start_atten_char_ind_handler;
            if (pthread_create
                (timer_id, NULL, *pthread_handler, (void *)pev) != 0)
            {
                LOG_PRINT (LOG_DEBUG,
                    "error when create pthread: %d\n", errno);
                return -1;
            }
        }
    }

#endif

#ifdef __linux__
    timer_t* timer_id;
    struct sigevent te;
    struct itimerspec its;
    struct sigaction sa;
    int sig_no = SIGRTMIN + 1;

    /* Set up signal handler. */
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    // set the corresponding handler according to the pev.current state
    switch (pev->matching_state)
    {
    case 1:
        timer_id = &pev->cm_start_atten_char_ind_timer_id;
        sa.sa_sigaction = received_cm_start_atten_char_ind_handler;
        break;
    case 5:
        timer_id = &pev->cm_slac_match_cnf_timer_id;
        sa.sa_sigaction = sent_cm_slac_match_cnf_handler;
        break;
    default:
        LOG_PRINT (LOG_VERBOSE, "Unknown current state\n");
        return TIMER_SUCCESS;
    }

    sigemptyset (&sa.sa_mask);
    if (sigaction (sig_no, &sa, NULL) == -1)
    {
        LOG_PRINT (LOG_DEBUG, "Error sigaction\n");
        return TIMER_ERROR;
    }

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = sig_no;
    te.sigev_value.sival_ptr = pev;

    if (timer_create (CLOCK_REALTIME, &te, timer_id) != 0)
    {
        LOG_PRINT (LOG_DEBUG, "timer_create error\n");
        return TIMER_ERROR;
    }

    its.it_value.tv_sec = expire_sec;
    its.it_value.tv_nsec = expire_msec * 1000000;

    its.it_interval.tv_sec = interval_sec;
    its.it_interval.tv_nsec = interval_msec * 1000000;

    if (timer_settime (*timer_id, 0, &its, NULL) != 0)
    {
        LOG_PRINT (LOG_DEBUG, "timer_settime() error\n");
        return TIMER_ERROR;
    }
    if (DEBUG_DISPLAY)
    {
        LOG_PRINT (LOG_VERBOSE, "Set the timer %d at ", (int)*timer_id);
        current_time ();
    }
#endif
    return TIMER_SUCCESS;
}
