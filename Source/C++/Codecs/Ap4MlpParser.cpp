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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4BitStream.h"
#include "Ap4MlpParser.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------+
|    AP4_MlpHeader::AP4_MlpHeader
+----------------------------------------------------------------------*/
AP4_MlpHeader::AP4_MlpHeader(const AP4_UI08* bytes,
                             AP4_UI32        num_of_substreams)
{
    m_check_nibble       = 0;
    m_access_unit_length = 0;
    m_input_timing       = 0;
    m_substreams         = 0;
    m_output_timing      = 0;
    m_is_major_sync      = false;
    AP4_BitReader bits(bytes, AP4_MLP_ACCESS_UNIT_SIZE);

    m_check_nibble       = bits.ReadBits(4);
    m_access_unit_length = bits.ReadBits(12);
    m_input_timing       = bits.ReadBits(16);

    if (bits.PeekBits(32) == AP4_MLP_MAJORSYNC_SYNCWORD) {
        m_is_major_sync = true;
        m_major_sync_info = new AP4_MlpAccessUnit::AP4_MajorSyncInfo;
        m_major_sync_info->format_sync    = bits.ReadBits(32);
        m_major_sync_info->format_info    = bits.ReadBits(32);
        m_major_sync_info->signature      = bits.ReadBits(16);
        m_major_sync_info->flags          = bits.ReadBits(16);
        bits.SkipBits(16);
        m_major_sync_info->variable_rate  = bits.ReadBit();
        m_major_sync_info->peak_data_rate = bits.ReadBits(15);
        m_major_sync_info->substreams     = bits.ReadBits(4);
        m_substreams = m_major_sync_info->substreams;
        num_of_substreams = m_substreams;
        bits.SkipBits(2);
        m_major_sync_info->extended_substream_info = bits.ReadBits(2);
        m_major_sync_info->substream_info = bits.ReadBits(8);
        // channel_meaning()
        bits.SkipBits(6);
        m_major_sync_info->ch._2ch_control_enabled = bits.ReadBit();
        m_major_sync_info->ch._6ch_control_enabled = bits.ReadBit();
        m_major_sync_info->ch._8ch_control_enabled = bits.ReadBit();
        bits.SkipBit();
        m_major_sync_info->ch.drc_start_up_gain    = bits.ReadBits(7);
        m_major_sync_info->ch._2ch_dialogue_norm   = bits.ReadBits(6);
        m_major_sync_info->ch._2ch_mix_level       = bits.ReadBits(6);
        m_major_sync_info->ch._6ch_dialogue_norm   = bits.ReadBits(5);
        m_major_sync_info->ch._6ch_mix_level       = bits.ReadBits(6);
        m_major_sync_info->ch._6ch_source_format   = bits.ReadBits(5);
        m_major_sync_info->ch._8ch_dialogue_norm   = bits.ReadBits(5);
        m_major_sync_info->ch._8ch_mix_level       = bits.ReadBits(6);
        m_major_sync_info->ch._8ch_source_format   = bits.ReadBits(6);
        bits.SkipBit();
        m_major_sync_info->ch.extra_channel_meaning_present = bits.ReadBit();

        if (m_major_sync_info->ch.extra_channel_meaning_present == true) {
            m_major_sync_info->ch.extra_channel_meaning_length = bits.ReadBits(4);
            // extra_channel_meaning_data
            if (m_major_sync_info->substream_info & 0x80) {
                // 16ch_channel_meaning
                unsigned int _16ch_bits_counter = 0;
                unsigned int _16ch_begin = bits.GetBitsPosition();
                m_major_sync_info->ch.extra_ch._16ch._16ch_dialogue_norm = bits.ReadBits(5);
                m_major_sync_info->ch.extra_ch._16ch._16ch_mix_level = bits.ReadBits(6);
                m_major_sync_info->ch.extra_ch._16ch._16ch_channel_count = bits.ReadBits(5);
                m_major_sync_info->ch.extra_ch._16ch.dyn_object_only = bits.ReadBit();
                _16ch_bits_counter += 17;
                if (m_major_sync_info->ch.extra_ch._16ch.dyn_object_only) {
                    m_major_sync_info->ch.extra_ch._16ch.lfe_present = bits.ReadBit();
                    _16ch_bits_counter++;
                }
                else {
                    m_major_sync_info->ch.extra_ch._16ch._16ch_content_description = bits.ReadBits(4);
                    _16ch_bits_counter += 4;
                    if (m_major_sync_info->ch.extra_ch._16ch._16ch_content_description & 0x1) {
                        m_major_sync_info->ch.extra_ch._16ch.chan_distribute = bits.ReadBit();
                        bits.ReadBit();
                        m_major_sync_info->ch.extra_ch._16ch.lfe_only = bits.ReadBit();
                        _16ch_bits_counter += 3;
                        if (m_major_sync_info->ch.extra_ch._16ch.lfe_only == false) {
                            bits.SkipBit();
                            m_major_sync_info->ch.extra_ch._16ch._16ch_channel_assignment = bits.ReadBits(10);
                            _16ch_bits_counter += 11;
                        }
                    }
                    if (m_major_sync_info->ch.extra_ch._16ch._16ch_content_description & 0x2) {
                        m_major_sync_info->ch.extra_ch._16ch._16ch_intermediate_spatial_format = bits.ReadBits(3);
                        _16ch_bits_counter += 3;
                    }
                    if (m_major_sync_info->ch.extra_ch._16ch._16ch_content_description & 0x4) {
                        m_major_sync_info->ch.extra_ch._16ch._16ch_dynamic_object_count = bits.ReadBits(5);
                        _16ch_bits_counter += 5;
                    }
                }
                unsigned int _16ch_length = bits.GetBitsPosition() - _16ch_begin;
                bits.SkipBits(((m_major_sync_info->ch.extra_channel_meaning_length + 1) * 16) - _16ch_length - 4);
            }
            else {
                bits.SkipBits(((m_major_sync_info->ch.extra_channel_meaning_length + 1) * 16) - 4);
            }
        }

        m_major_sync_info->major_sync_info_CRC = bits.ReadBits(16);
    } else {
        m_is_major_sync = false;
    }
        
    if (num_of_substreams > 0) {
        m_substreams_dir = new AP4_MlpAccessUnit::AP4_SubstreamDir[num_of_substreams];
        AP4_SetMemory(m_substreams_dir, 0, num_of_substreams * sizeof(m_substreams_dir[0]));

        m_substreams_seg = new AP4_MlpAccessUnit::AP4_SubstreamSeg[num_of_substreams];
        AP4_SetMemory(m_substreams_seg, 0, num_of_substreams * sizeof(m_substreams_seg[0]));
    }
    for (unsigned int substream_idx = 0; substream_idx < num_of_substreams; substream_idx++) {
        AP4_MlpAccessUnit::AP4_SubstreamDir &substream_dir = m_substreams_dir[substream_idx];
        substream_dir.extra_substream_word = bits.ReadBit();
        substream_dir.restart_nonexistent = bits.ReadBit();
        substream_dir.crc_present = bits.ReadBit();
        bits.SkipBit();
        substream_dir.substream_end_ptr = bits.ReadBits(12);
        if (substream_dir.extra_substream_word) {
            substream_dir.drc_gain_update = bits.ReadBits(9);
            substream_dir.drc_time_update = bits.ReadBits(3);
            bits.SkipBits(4);
        }
    }
    unsigned int start_seg_position = bits.GetBitsPosition();
    for (unsigned int substream_idx = 0; substream_idx < num_of_substreams; substream_idx++) {
        AP4_MlpAccessUnit::AP4_SubstreamSeg &substream_seg = m_substreams_seg[substream_idx];
        // block header exists
        if (bits.ReadBit()) {
            // restart header
            if (bits.ReadBit()) {
                AP4_UI16 restart_sync_word = bits.ReadBits(14);
                AP4_UI16 output_timing = bits.ReadBits(16);
                AP4_UI08 min_chan = bits.ReadBits(4);
                AP4_UI08 max_chan = bits.ReadBits(4);

                if (substream_idx == 0 && restart_sync_word == 0x31EA) {
                    substream_seg.output_timing = output_timing;
                    m_output_timing = output_timing;
                    substream_seg.min_chan = min_chan;
                    substream_seg.max_chan = max_chan;
                }
                else if (substream_idx == 1 && (restart_sync_word == 0x31EA || restart_sync_word == 0x31EB)) {
                    substream_seg.min_chan = min_chan;
                    substream_seg.max_chan = max_chan;
                }
                else if (substream_idx == 2 && restart_sync_word == 0x31EB) {
                    substream_seg.min_chan = min_chan;
                    substream_seg.max_chan = max_chan;
                }
                else if (substream_idx == 3 && restart_sync_word == 0x31EC) {
                    substream_seg.min_chan = min_chan;
                    substream_seg.max_chan = max_chan;
                }
            }
        }
        unsigned int bits_position = bits.GetBitsPosition() - start_seg_position;
        bits.SkipBits(m_substreams_dir[substream_idx].substream_end_ptr * 2 * 8 - bits_position);
    }
}
//}

/*----------------------------------------------------------------------+
|    AP4_MlpHeader::MatchFixed
|
|    Check that two fixed headers are the same
|
+----------------------------------------------------------------------*/
bool
AP4_MlpHeader::MatchFixed(AP4_MlpHeader frame, AP4_MlpHeader next_frame)
{
    return true;
}

/*----------------------------------------------------------------------+
|    AP4_MlpHeader::Check
+----------------------------------------------------------------------*/
AP4_Result
AP4_MlpHeader::Check()
{
//    if (m_Bsid > 8) {
//        return AP4_FAILURE;
//    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::AP4_MlpParser
+----------------------------------------------------------------------*/
AP4_MlpParser::AP4_MlpParser() :
    m_CurNSubtreams(0),
    m_AUOffest(0),
    m_FrameCount(0)
{
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::~AP4_MlpParser
+----------------------------------------------------------------------*/
AP4_MlpParser::~AP4_MlpParser()
{
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::Reset
+----------------------------------------------------------------------*/
AP4_Result
AP4_MlpParser::Reset()
{
    m_FrameCount = 0;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::Feed
+----------------------------------------------------------------------*/
AP4_Result
AP4_MlpParser::Feed(const AP4_UI08* buffer,
    AP4_Size*       buffer_size,
    AP4_Flags       flags)
{
    AP4_Size free_space;

    /* update flags */
    m_Bits.m_Flags = flags;

    /* possible shortcut */
    if (buffer == NULL ||
        buffer_size == NULL ||
        *buffer_size == 0) {
        return AP4_SUCCESS;
    }

    /* see how much data we can write */
    free_space = m_Bits.GetBytesFree();
    if (*buffer_size > free_space) *buffer_size = free_space;
    if (*buffer_size == 0) return AP4_SUCCESS;

    /* write the data */
    return m_Bits.WriteBytes(buffer, *buffer_size);
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::FindHeader
+----------------------------------------------------------------------*/
AP4_Result
AP4_MlpParser::FindHeader(AP4_UI08* header)
{
    AP4_Size available = m_Bits.GetBytesAvailable();
    AP4_Size access_unit_length;

    if (available) {
        if (GetAccessUnitOffset() == 0 && available > 24 * 8) {
            m_Bits.PeekBytes(header, 24);
            if (((header[0] << 8) | header[1]) == AP4_MLP_SMPTE_SYNCWORD) {
                m_begin_with_smpte = true;
                m_timestamp = new AP4_MlpTimeStamp;
                m_timestamp->hours = header[2] | header[3];
                m_timestamp->minutes = header[4] | header[5];
                m_timestamp->seconds = header[6] | header[7];
                m_timestamp->frames = header[8] | header[9];
                m_timestamp->samples = header[10] | header[11];
                m_timestamp->framerate = (header[13] >> 2) & 0x0f;
                m_timestamp->dropframe = header[13] & 0x01;

                access_unit_length = ((header[16] << 8) | header[17]) & 0x0fff;
                m_Bits.SkipBytes(16);      // strip timestampe() 
                m_Bits.PeekBytes(header, access_unit_length * 2);
                return AP4_SUCCESS;
            } else if (((header[4] << 24) | (header[5] << 16) | (header[6] << 8) |
                header[7]) == AP4_MLP_MAJORSYNC_SYNCWORD) {
                m_begin_with_smpte = false;
                access_unit_length = ((header[0] << 8) | header[1]) & 0x0fff;
                m_Bits.PeekBytes(header, access_unit_length * 2);
                return AP4_SUCCESS;
            }
            return AP4_ERROR_INVALID_FORMAT;
        } else if (GetAccessUnitOffset() > 0 && available > 2 * 8) {
            m_Bits.PeekBytes(header, 2);
            access_unit_length = ((header[0] << 8) | header[1]) & 0x0fff;
            m_Bits.PeekBytes(header, access_unit_length * 2);
            return AP4_SUCCESS;
        }
    }
    return AP4_ERROR_NOT_ENOUGH_DATA;
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::FindFrame
+----------------------------------------------------------------------*/
AP4_Result
AP4_MlpParser::FindFrame(AP4_MlpFrame& frame)
{
    unsigned char   raw_header[AP4_MLP_ACCESS_UNIT_SIZE];
    AP4_Result      result;

    /* align to the start of the next byte */
    m_Bits.ByteAlign();

    /* find a frame header */
    result = FindHeader(raw_header);
    if (AP4_FAILED(result)) return result;

    AP4_MlpHeader mlp_header(raw_header, m_CurNSubtreams);

    if (mlp_header.m_is_major_sync) {
        SetCurrentNumOfSubstreams(mlp_header.m_substreams);
        frame.m_Info.m_Iframe = 1;
        frame.m_Info.m_MlpStreamInfo.format_info = mlp_header.m_major_sync_info->format_info;
        frame.m_Info.m_MlpStreamInfo.peak_data_rate = mlp_header.m_major_sync_info->peak_data_rate;
        switch ((mlp_header.m_major_sync_info->format_info >> 28) & 0xff)
        {
        case 0:
            frame.m_Info.m_SampleRate = 48000;
            frame.m_Info.m_SamplesPerAU = 40;
            frame.m_Info.m_SampleDuration = 40;
            frame.m_Info.m_MediaTimeScale = 48000;
            break;
        case 1:
            frame.m_Info.m_SampleRate = 96000;
            frame.m_Info.m_SamplesPerAU = 80;
            frame.m_Info.m_SampleDuration = 80;
            frame.m_Info.m_MediaTimeScale = 96000;
            break;
        case 2:
            frame.m_Info.m_SampleRate = 192000;
            frame.m_Info.m_SamplesPerAU = 160;
            frame.m_Info.m_SampleDuration = 160;
            frame.m_Info.m_MediaTimeScale = 192000;
            break;
        case 8:
            frame.m_Info.m_SampleRate = 44100;
            frame.m_Info.m_SamplesPerAU = 40;
            frame.m_Info.m_SampleDuration = 40;
            frame.m_Info.m_MediaTimeScale = 44100;
            break;
        case 9:
            frame.m_Info.m_SampleRate = 88200;
            frame.m_Info.m_SamplesPerAU = 80;
            frame.m_Info.m_SampleDuration = 80;
            frame.m_Info.m_MediaTimeScale = 88200;
            break;
        case 10:
            frame.m_Info.m_SampleRate = 176400;
            frame.m_Info.m_SamplesPerAU = 160;
            frame.m_Info.m_SampleDuration = 160;
            frame.m_Info.m_MediaTimeScale = 176400;
            break;
        default:
            frame.m_Info.m_SampleRate = 0;
            frame.m_Info.m_SamplesPerAU = 0;
            frame.m_Info.m_SampleDuration = 0;
            frame.m_Info.m_MediaTimeScale = 0;
        }
    } else {
        frame.m_Info.m_Iframe = 0;
        frame.m_Info.m_SampleRate = 0;
        frame.m_Info.m_SamplesPerAU = 0;
    }

    frame.m_Info.m_ChannelCount = 2;
    frame.m_Info.m_FrameSize = mlp_header.m_access_unit_length * 2;
    frame.m_Source = &m_Bits;
    SetAccessUnitOffset(frame.m_Info.m_FrameSize);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::GetBytesFree
+----------------------------------------------------------------------*/
AP4_Size
AP4_MlpParser::GetBytesFree()
{
    return (m_Bits.GetBytesFree());
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::GetBytesAvailable
+----------------------------------------------------------------------*/
AP4_Size
AP4_MlpParser::GetBytesAvailable()
{
    return (m_Bits.GetBytesAvailable());
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::SetAccessUnitOffset
+----------------------------------------------------------------------*/
AP4_Result
AP4_MlpParser::SetAccessUnitOffset(AP4_UI32 access_unit_size)
{
    m_AUOffest += access_unit_size;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::GetAccessUnitOffset
+----------------------------------------------------------------------*/
AP4_Size
AP4_MlpParser::GetAccessUnitOffset()
{
    return m_AUOffest;
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::SetCurrentNumOfSubstreams
+----------------------------------------------------------------------*/
AP4_Result
AP4_MlpParser::SetCurrentNumOfSubstreams(AP4_UI32 num_of_substreams)
{
    m_CurNSubtreams = num_of_substreams;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_MlpParser::GetCurrentNumOfSubstreams
+----------------------------------------------------------------------*/
AP4_UI32
AP4_MlpParser::GetCurrentNumOfSubstreams()
{
    return m_CurNSubtreams;
}
