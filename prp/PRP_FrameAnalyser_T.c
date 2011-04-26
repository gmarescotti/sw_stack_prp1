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
*  17.12.07 | mesv     | file created
************|**********|*********************************************
*  14.01.08 | mesv     | added some comments
*********************************************************************/

#include "PRP_FrameAnalyser_T.h"
#include "PRP_LogItf_T.h"
#include "PRP_Environment_T.h"

/************************************************************/
/*       PRP_FrameAnalyser_T_analyse_rx                     */
/************************************************************/
integer32 PRP_FrameAnalyser_T_analyse_rx(PRP_FrameAnalyser_T* const me, octet* data, uinteger32* length, octet lan_id)
{
	PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);
	
	if(me == NULL_PTR)
	{
		return(-PRP_ERROR_NULL_PTR);
	}
	
	/* basic checks */
	if(data == NULL_PTR)
	{
		return(-PRP_ERROR_NULL_PTR);
	}

	if(length == NULL_PTR)
	{
		return(-PRP_ERROR_NULL_PTR);
	}
	
	if((lan_id != PRP_ID_LAN_A) && (lan_id != PRP_ID_LAN_B))
	{
		return(-PRP_ERROR_WRONG_VAL);
	}
	
	if(*length > 18)
	{
		if((data[12] == 0x88) && (data[13] == 0xFB)) /* if Supervision frame */
		{
			return(PRP_Supervision_T_supervision_rx(&(me->environment_->supervision_), data, length, lan_id));
		}
		else if((data[12] == 0x81) && (data[13] == 0x00) &&
		        (data[16] == 0x88) && (data[17] == 0xFB)) /* if VLAN tagged Supervision frame */
		{
			return(PRP_Supervision_T_supervision_rx(&(me->environment_->supervision_), data, length, lan_id));
		}
		if((data[12] == 0x00) && (data[13] == 0x27) &&
		   (data[14] == 0x42) && (data[15] == 0x42) &&
		   (data[16] == 0x03)) /* if RSTP */
		{
			return(PRP_Bridging_T_bridging_rx(&(me->environment_->bridging_), data, length, lan_id));
		}
		else
		{
			return(PRP_Frames_T_normal_rx(&(me->frames_), data, length, lan_id));
		}		
	}
	else
	{
		return(PRP_Frames_T_normal_rx(&(me->frames_), data, length, lan_id));
	}			
	
}

/************************************************************/
/*       PRP_FrameAnalyser_T_analyse_tx                     */
/************************************************************/
integer32 PRP_FrameAnalyser_T_analyse_tx(PRP_FrameAnalyser_T* const me, octet* data, uinteger32* length, octet lan_id)
{
	PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);
	
	if(me == NULL_PTR)
	{
		return(-PRP_ERROR_NULL_PTR);
	}
	
	if(data == NULL_PTR)
	{
		return(-PRP_ERROR_NULL_PTR);
	}

	if(length == NULL_PTR)
	{
		return(-PRP_ERROR_NULL_PTR);
	}
	
	return(PRP_Frames_T_normal_tx(&(me->frames_), data, length, lan_id));
	
}


/************************************************************/
/*       PRP_FrameAnalyser_T_init                           */
/************************************************************/
void PRP_FrameAnalyser_T_init(PRP_FrameAnalyser_T* const me, PRP_Environment_T* const environment)
{
	PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);
	
	if(me == NULL_PTR)
	{
		return;
	}

	if(environment == NULL_PTR)
	{
		return;
	}

	me->environment_ = environment; /* connect to the environment */
	PRP_Frames_T_init(&(me->frames_), me);
}

/************************************************************/
/*       PRP_FrameAnalyser_T_cleanup                        */
/************************************************************/
void PRP_FrameAnalyser_T_cleanup(PRP_FrameAnalyser_T* const me)
{
	PRP_PRP_LOGOUT(3, "[%s] entering \n", __FUNCTION__);
	
	if(me == NULL_PTR)
	{
		return;
	}
	
	me->environment_ = NULL_PTR;
	PRP_Frames_T_cleanup(&(me->frames_));
}

