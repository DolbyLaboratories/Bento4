/*****************************************************************
|
|    AP4 - MLP Sync Frame Parser
|
|    Copyright 2002-2020 Axiomatic Systems, LLC
|
|
|    This file is part of Bento4/AP4 (MP4 Atom Processing Library).
|
|    Unless you have obtained Bento4 under a difference license,
|    this version of Bento4 is Bento4|GPL.
|    Bento4|GPL is free software; you can redistribute it and/or modify
|    it under the terms of the GNU General Public License as published by
|    the Free Software Foundation; either version 2, or (at your option)
|    any later version.
|
|    Bento4|GPL is distributed in the hope that it will be useful,
|    but WITHOUT ANY WARRANTY; without even the implied warranty of
|    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
|    GNU General Public License for more details.
|
|    You should have received a copy of the GNU General Public License
|    along with Bento4|GPL; see the file COPYING.  If not, write to the
|    Free Software Foundation, 59 Temple Place - Suite 330, Boston, MA
|    02111-1307, USA.
|
****************************************************************/

#ifndef _AP4_MLP_PARSER_H_
#define _AP4_MLP_PARSER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4BitStream.h"
#include "Ap4DmlpAtom.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/

#define AP4_MLP_ACCESS_UNIT_SIZE                 65536
#define AP4_MLP_MAJORSYNC_SYNCWORD               0xF8726FBA
#define AP4_MLP_SMPTE_SYNCWORD                   0x0110



/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/

typedef struct {
    AP4_UI32  hours;
    AP4_UI32  minutes;
    AP4_UI32  seconds;
    AP4_UI32  frames;
    AP4_UI32  samples;
    AP4_UI32  framerate;
    AP4_UI32  dropframe;
} AP4_MlpTimeStamp;

typedef struct {
    struct AP4_MajorSyncInfo {
        AP4_UI32  format_sync;
        AP4_UI32  format_info;
        AP4_UI32  signature;
        AP4_UI32  flags;
        AP4_UI32  variable_rate;
        AP4_UI32  peak_data_rate;
        AP4_UI32  substreams;
        AP4_UI32  extended_substream_info;
        AP4_UI32  substream_info;
        AP4_UI32  major_sync_info_CRC;
        struct MLPChannelMeaning
        {
            AP4_UI32  _2ch_control_enabled;
            AP4_UI32  _6ch_control_enabled;
            AP4_UI32  _8ch_control_enabled;
            AP4_UI32  drc_start_up_gain;
            AP4_UI32  _2ch_dialogue_norm;
            AP4_UI32  _2ch_mix_level;
            AP4_UI32  _6ch_dialogue_norm;
            AP4_UI32  _6ch_mix_level;
            AP4_UI32  _6ch_source_format;
            AP4_UI32  _8ch_dialogue_norm;
            AP4_UI32  _8ch_mix_level;
            AP4_UI32  _8ch_source_format;
            AP4_UI32  extra_channel_meaning_present;
            AP4_UI32  extra_channel_meaning_length;
            struct Extra_Channel_Meaning_Data
            {
                struct SixteenCh_Channel_Meaning
                {
                    AP4_UI32  _16ch_dialogue_norm;
                    AP4_UI32  _16ch_mix_level;
                    AP4_UI32  _16ch_channel_count;
                    AP4_UI32  dyn_object_only;
                    AP4_UI32  lfe_present;
                    AP4_UI32  _16ch_content_description;
                    AP4_UI32  chan_distribute;
                    AP4_UI32  lfe_only;
                    AP4_UI32  _16ch_channel_assignment;
                    AP4_UI32  _16ch_intermediate_spatial_format;
                    AP4_UI32  _16ch_dynamic_object_count;
                } _16ch;
            } extra_ch;
        } ch;
    };
    struct AP4_SubstreamDir {
        AP4_UI32  extra_substream_word;
        AP4_UI32  restart_nonexistent;
        AP4_UI32  crc_present;
        AP4_UI32  substream_end_ptr;
        AP4_UI32  drc_gain_update;
        AP4_UI32  drc_time_update;
    };
    struct AP4_SubstreamSeg {
        AP4_UI32 output_timing;
        AP4_UI32 min_chan;
        AP4_UI32 max_chan;
    };
} AP4_MlpAccessUnit;


class AP4_MlpHeader {
public:
    // constructor
    AP4_MlpHeader(const AP4_UI08* bytes,
                  AP4_UI32        num_of_substreams = 0);

    // methods
    AP4_Result Check();
    
    // timestamp()
    // AP4_MlpTimeStamp *m_timestamp;

    // mlp_sync
    AP4_UI32 m_check_nibble;
    AP4_UI32 m_access_unit_length;
    AP4_UI32 m_input_timing;
    AP4_UI32 m_substreams;
    AP4_UI32 m_output_timing;
    bool m_is_major_sync;
 
    // major_sync_info
    AP4_MlpAccessUnit::AP4_MajorSyncInfo *m_major_sync_info;

    // substream_directory
    AP4_MlpAccessUnit::AP4_SubstreamDir *m_substreams_dir;

    // substream directory
    AP4_MlpAccessUnit::AP4_SubstreamSeg *m_substreams_seg;
    
// class methods
    static bool MatchFixed(AP4_MlpHeader frame, AP4_MlpHeader next_frame);

};

typedef struct {
    AP4_UI32  m_FrameSize;
    AP4_UI32  m_ChannelCount;
    AP4_UI32  m_SampleDuration;
    AP4_UI32  m_MediaTimeScale;
    AP4_UI32  m_Iframe;
    AP4_UI32  m_SampleRate;
    AP4_UI32  m_SamplesPerAU;
    AP4_UI32  m_NumOfSubStreams;
    AP4_DmlpAtom::StreamInfo m_MlpStreamInfo;
} AP4_MlpFrameInfo;

typedef struct {
    AP4_BitStream*   m_Source;
    AP4_MlpFrameInfo m_Info;
//    AP4_MlpAccessUnit m_AccessUnit;
} AP4_MlpFrame;



class AP4_MlpParser {
public:
    // constructor and destructor
    AP4_MlpParser();
    virtual ~AP4_MlpParser();

    // methods
    AP4_Result Reset();
    AP4_Result Feed(const AP4_UI08* buffer,
        AP4_Size*       buffer_size,
        AP4_Flags       flags = 0);
    AP4_Result FindFrame(AP4_MlpFrame& frame);
    AP4_Result Skip(AP4_Size size);
    AP4_Size   GetBytesFree();
    AP4_Size   GetBytesAvailable();
    AP4_Result SetAccessUnitOffset(AP4_UI32 access_unit_size);
    AP4_UI32   GetAccessUnitOffset();

    AP4_Result SetCurrentNumOfSubstreams(AP4_UI32 num_of_substreams);
    AP4_UI32   GetCurrentNumOfSubstreams();

private:
    // methods
    AP4_Result FindHeader(AP4_UI08* header);

    // members
    AP4_MlpTimeStamp *m_timestamp;
    AP4_UI32         m_CurNSubtreams;
    AP4_UI32         m_AUOffest;
    AP4_BitStream    m_Bits;
    AP4_Cardinal     m_FrameCount;
    bool             m_begin_with_smpte;
};

#endif // _AP4_MLP_PARSER_H_
