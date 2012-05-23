/********************************************************************
*
*  Copyright (c) 2007, Institute of Embedded Systems at 
*                      Zurich University of Applied Sciences 
*                      (http://ines.zhaw.ch)
*
*  All rights reserved.
*
*
*  Redistribution and use in source and binary forms, with or  
*  without modification, are permitted provided that the 
*  following conditions are met:
*
*  - Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer. 
*
*  - Redistributions in binary form must reproduce the above 
*    copyright notice, this list of conditions and the following 
*    disclaimer in the documentation and/or other materials provided
*    with the distribution. 
*
*  - Neither the name of the Zurich University of Applied Sciences
*    nor the names of its contributors may be used to endorse or 
*    promote products derived from this software without specific 
*    prior written permission. 
*
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
*  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
*  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
*  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
*  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
*  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
*  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
*  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
*  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
*  POSSIBILITY OF SUCH DAMAGE.
*
*********************************************************************/


/********************************************************************
*   _____       ______  _____                                       *
*  |_   _|     |  ____|/ ____|                                      *
*    | |  _ __ | |__  | (___    Institute of Embedded Systems       *
*    | | | '_ \|  __|  \___ \   Zurich University of Applied        *
*   _| |_| | | | |____ ____) |  Sciences                            *
*  |_____|_| |_|______|_____/   8401 Winterthur, Switzerland        *
*                                                                   *
*********************************************************************
*
*  Project     : Parallel Redundancy Protocol
*
*  Version     : 1.0
*  Author      : Sven Meier
*
*********************************************************************
*  Change History
*
*  Date     | Name     | Modification
************|**********|*********************************************
*  17.12.07 | mesv     | file created, this is the core
************|**********|*********************************************
*  14.01.08 | mesv     | added some comments
************|**********|*********************************************
*  13.07.11 | itin     | integration of discard algorithm for PRP1
*********************************************************************
*  14.05.12 | walh     | prp-1 modification, bridging removed
*********************************************************************/
#include <pthread.h>
#include "PRP_Frames_T.h"
#include "PRP_LogItf_T.h"
#include "PRP_FrameAnalyser_T.h"
#include "PRP_Trailer_T.h"
#include "PRP_RedundancyControlTrailer_T.h"
#include "PRP_Environment_T.h"
#include "PRP_EnvironmentConfiguration_T.h"
#include "PRP_NetItf_T.h"


/* send seq number */
struct PRP_Frames_T_seq_Nr PRP_sN;

/**
 * @fn void PRP_Frames_T_print(PRP_Frames_T* const me, octet* data, uinteger32* length, uinteger32 level)
 * @brief Print the PRP_Frames status information
 * @param   me PRP_Frames_T this pointer
 * @param   data octet pointer to the beginning of the frame (dest mac)
 * @param   length uinteger32 pointer to the length in bytes of the frame
 * @param   level uinteger32 importance
 */
void PRP_Frames_T_print(PRP_Frames_T* const me, octet* data, uinteger32* length, uinteger32 level)
{
    uinteger32 i;

    PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);

    /* basic argument checks */
    if (me == NULL_PTR) {
        return;
    }
    if (data == NULL_PTR) {
        return;
    }
    if (length == NULL_PTR) {
        return;
    }

    PRP_PRP_LOGOUT(level, "%s\n", "====Frame============================");
    PRP_PRP_LOGOUT(level, "length: %u\n", *length);
    PRP_PRP_LOGOUT(level, "%s\n", "data");
    for (i=0; i<*length; i++) {
        PRP_PRP_LOGOUT(level, "%u: %x\n", i, data[i]);
    }
    PRP_PRP_LOGOUT(level, "%s\n", "====================================");
}

/**
 * @fn integer32 PRP_Frames_T_normal_rx(PRP_Frames_T* const me, octet* data, uinteger32* length, octet lan_id)
 * @brief Actual processing of a received frame
 * @param   me PRP_Frames_T this pointer
 * @param   data octet pointer to the beginning of the frame (dest mac)
 * @param   length uinteger32 pointer to the length in bytes of the frame
 * @param   lan_id octet on which LAN it was received
 * @retval  1 integer32 DROP
 * @retval  0 integer32 KEEP
 * @retval  <0 integer32 ERROR (code)
 */
integer32 PRP_Frames_T_normal_rx(PRP_Frames_T* const me, octet* data, uinteger32* length, octet lan_id)
{
    PRP_RedundancyControlTrailer_T* trailer;
    octet real_lan_id;
    integer32 i, ret;
    octet src_mac[6];

    PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);

    /* basic argument checks */
    if (me == NULL_PTR) {
        return(-PRP_ERROR_NULL_PTR);
    }
    if (*length < (2*PRP_ETH_ADDR_LENGTH)) {
        return(-PRP_ERROR_WRONG_VAL);
    }

    for (i=0; i<PRP_ETH_ADDR_LENGTH; i++) {
        src_mac[i] = data[(6 + i)];
    }

    PRP_PRP_LOGOUT(2, "%s\n", "received frame");
    PRP_Frames_T_print(me, data, length, 4);

    /* disable the ports by software */
    if ((me->frame_analyser_->environment_->environment_configuration_.adapter_active_A_ == FALSE) &&
        (lan_id == PRP_ID_LAN_A)) {
        return(PRP_DROP);
    }

    if ((me->frame_analyser_->environment_->environment_configuration_.adapter_active_B_ == FALSE) &&
        (lan_id == PRP_ID_LAN_B)) {
        return(PRP_DROP);
    }

    trailer = PRP_Trailer_T_get_trailer(&(me->trailer_rx_), data, length);
    if (trailer != NULL_PTR) {              /* lan id out of the trailer */
        real_lan_id = trailer->lan_id_;
    } else {
        /* lan_id from which it was received */
        real_lan_id = lan_id;
    }

    /* update counters */
    if (lan_id == PRP_ID_LAN_A) {
        me->frame_analyser_->environment_->environment_configuration_.cnt_total_received_A_++;

        /* received LAN id not the adapter received the frame */
    if ((lan_id != real_lan_id)) {
            PRP_PRP_LOGOUT(1, "%s\n", "LAN identifier of RCT different than the adapter(A) the frame was received on");
            me->frame_analyser_->environment_->environment_configuration_.cnt_total_errors_A_++;
            return(PRP_KEEP);
        }
    }
    /* PRP_ID_LAN_B */
    else {
        me->frame_analyser_->environment_->environment_configuration_.cnt_total_received_B_++;
        /* received LAN id not the adapter receieved the frame */
    if ((lan_id != real_lan_id)) {
            PRP_PRP_LOGOUT(1, "%s\n", "LAN identifier of RCT different than the adapter(B) the frame was received on");
            me->frame_analyser_->environment_->environment_configuration_.cnt_total_errors_B_++;
            return(PRP_KEEP);
        }
    }

    /* remove trailer if NOT in transparent mode */
    if (me->frame_analyser_->environment_->environment_configuration_.transparent_reception_ == FALSE) {
        PRP_PRP_LOGOUT(2, "%s\n", "removing trailer");
        PRP_Trailer_T_remove_trailer(&(me->trailer_rx_), data, length);
    }

    PRP_PRP_LOGOUT(2,"%s","SRC MAC: \n");
    for (i=0;i<6;i++) {
        PRP_PRP_LOGOUT(2,"%x",src_mac[i]);
    }
    PRP_PRP_LOGOUT(2,"\nSequ. NR: %d\n",me->trailer_rx_.redundancy_control_trailer_.seq_);

    /* The frame origin is a DAN, search drop table to check if frame has
     * been received previously */
    ret = PRP_DiscardAlgorithm_T_search_entry(&(me->frame_analyser_->environment_->discard_algorithm_),
                                                   src_mac, me->trailer_rx_.redundancy_control_trailer_.seq_octet_);

    return ret;
}

/**
 * @fn integer32 PRP_Frames_T_normal_tx(PRP_Frames_T* const me, octet* data, uinteger32* length, octet lan_id)
 * @brief Actual processing of a transmitting frame.
 * @retval  me PRP_Frames_T this pointer
 * @param   data octet pointer to the beginning of the frame (dest mac)
 * @param   length uinteger32 pointer to the length in bytes of the frame
 * @param   lan_id octet on which LAN it was received
 * @retval  0 integer32 OK
 * @retval  <0 integer32 ERROR (code)
 */
integer32 PRP_Frames_T_normal_tx(PRP_Frames_T* const me, octet* data, uinteger32* length, octet lan_id)
{
    PRP_RedundancyControlTrailer_T temp_trailer;

    PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);

    if (me == NULL_PTR) {
        return(-PRP_ERROR_NULL_PTR);
    }
    if (*length < (2*PRP_ETH_ADDR_LENGTH)) {
        return(-PRP_ERROR_WRONG_VAL);
    }

    PRP_PRP_LOGOUT(2, "%s\n", "node before transmission");


    /* always add trailer */
    /* if (((me->frame_analyser_->environment_->environment_configuration_.duplicate_discard_ == TRUE) */
    PRP_RedundancyControlTrailer_T_init(&temp_trailer);
    temp_trailer.lan_id_ = PRP_ID_LAN_A;
    temp_trailer.seq_ = PRP_sN.tx_seq_nr_;
    PRP_PRP_LOGOUT(2, "%s\n", "add trailer for LAN A");
    PRP_RedundancyControlTrailer_T_print(&temp_trailer,2);
    PRP_Trailer_T_add_trailer(&(me->trailer_tx_), data, length, &temp_trailer);

    /* send it to LAN A or not */
    if (me->frame_analyser_->environment_->environment_configuration_.adapter_active_A_ == TRUE) {
        PRP_PRP_LOGOUT(2, "%s\n", "send frame on LAN A");
        PRP_NetItf_T_transmit(data, length, PRP_ID_LAN_A);
        me->frame_analyser_->environment_->environment_configuration_.cnt_total_sent_A_++;
        PRP_Frames_T_print(me, data, length, 4);
    }

    /* add trailer again therefore remove it first*/
    PRP_Trailer_T_remove_trailer(&(me->trailer_tx_), data, length);

    /* always add trailer */
    /* if (((me->frame_analyser_->environment_->environment_configuration_.duplicate_discard_ == TRUE) */
        PRP_RedundancyControlTrailer_T_init(&temp_trailer);
        temp_trailer.lan_id_ = PRP_ID_LAN_B;
        temp_trailer.seq_ = PRP_sN.tx_seq_nr_;
        PRP_PRP_LOGOUT(2, "%s\n", "add trailer for LAN B");
        PRP_RedundancyControlTrailer_T_print(&temp_trailer,2);
        PRP_Trailer_T_add_trailer(&(me->trailer_tx_), data, length, &temp_trailer);

    /* send it to LAN B or not */
    if (me->frame_analyser_->environment_->environment_configuration_.adapter_active_B_ == TRUE) {
        PRP_PRP_LOGOUT(2, "%s\n", "send frame on LAN B");
        PRP_NetItf_T_transmit(data, length, PRP_ID_LAN_B);
        me->frame_analyser_->environment_->environment_configuration_.cnt_total_sent_B_++;
        PRP_Frames_T_print(me, data, length, 4);
    }

    if ((me->frame_analyser_->environment_->environment_configuration_.adapter_active_A_ == TRUE) ||
        (me->frame_analyser_->environment_->environment_configuration_.adapter_active_B_ == TRUE)) {
        pthread_mutex_lock(&(PRP_sN.mutex_));
        (PRP_sN.tx_seq_nr_)++;
        pthread_mutex_unlock(&(PRP_sN.mutex_));
    }

    return(0);
}

/**
 * @fn void PRP_Frames_T_init(PRP_Frames_T* const me, PRP_FrameAnalyser_T* const frame_analyser)
 * @brief Initialize the PRP_Frames interface
 * @param   me PRP_Frames_T this pointer
 * @param   frame_analyser PRP_FrameAnalyser_T pointer to the frame analyser interface
 */
void PRP_Frames_T_init(PRP_Frames_T* const me, PRP_FrameAnalyser_T* const frame_analyser)
{
    PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);

    if (me == NULL_PTR) {
        return;
    }

    if (frame_analyser == NULL_PTR) {
        return;
    }

    /* initialize send sequence number */
    PRP_sN.tx_seq_nr_ = 0;
    pthread_mutex_init(&(PRP_sN.mutex_), NULL);

    me->frame_analyser_ = frame_analyser;
    PRP_Trailer_T_init(&(me->trailer_rx_));
    PRP_Trailer_T_init(&(me->trailer_tx_));
}

/**
 * @fn void PRP_Frames_T_cleanup(PRP_Frames_T* const me)
 * @brief Clean up the PRP_frames interface
 * @param   me PRP_Frames_T this pointer
 */
void PRP_Frames_T_cleanup(PRP_Frames_T* const me)
{
    PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);

    if (me == NULL_PTR) {
        return;
    }

    me->frame_analyser_ = NULL_PTR;
    PRP_Trailer_T_cleanup(&(me->trailer_rx_));
    PRP_Trailer_T_cleanup(&(me->trailer_tx_));
}

