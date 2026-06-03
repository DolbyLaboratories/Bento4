/*****************************************************************
|
|    AP4 - AC4 Sync Frame Parser
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
#include "Ap4Ac4Parser.h"
#include <cstdlib>

bool AP4_Ac4Header::m_DeprecatedV0 = true;
/*----------------------------------------------------------------------+
|    AP4_Ac4Header::AP4_Ac4Header
+----------------------------------------------------------------------*/
AP4_Ac4Header::AP4_Ac4Header(const AP4_UI08* bytes, unsigned int size, bool header_exists)
{
    /* Read the entire the frame */
    AP4_BitReader bits(bytes, size);

    /* Sync word and frame_size()*/
    if (header_exists) {
        m_SyncWord = bits.ReadBits(16);
        m_HeaderSize = 2;
        if (m_SyncWord == AP4_AC4_SYNC_WORD_CRC){
            m_CrcSize = 2;
        }else {
            m_CrcSize = 0;
        }
        m_FrameSize = bits.ReadBits(16);
        m_HeaderSize += 2;
        if (m_FrameSize == 0xFFFF){
            m_FrameSize = bits.ReadBits(24);
            m_HeaderSize += 3;
        }
    }

    /* Begin to parse TOC */
    m_TocSize = bits.GetBitsPosition() / 8; // toc size initialized

    m_BitstreamVersion = bits.ReadBits(2);
    if (m_BitstreamVersion == 3) {
        m_BitstreamVersion = AP4_Ac4VariableBits(bits, 2);
    }
    m_SequenceCounter  = bits.ReadBits(10);
    m_BWaitFrames      = bits.ReadBit();
    if (m_BWaitFrames) {
        m_WaitFrames = bits.ReadBits(3);
        if (m_WaitFrames > 0){ m_BrCode = bits.ReadBits(2);}
    }else {
        m_WaitFrames = -1;
    }

    m_FsIndex               = bits.ReadBit();
    m_FrameRateIndex        = bits.ReadBits(4);
    m_BIframeGlobal         = bits.ReadBit();
    m_BSinglePresentation   = bits.ReadBit();
    if (m_BSinglePresentation == 1){
        m_NPresentations = 1;
        m_BMorePresentations = 0;
    }else{
        m_BMorePresentations = bits.ReadBit();
        if (m_BMorePresentations == 1){
            m_NPresentations = AP4_Ac4VariableBits(bits, 2) + 2;
        }else{
            m_NPresentations = 0;
        }
    }
    m_PayloadBase = 0;
    m_BPayloadBase = bits.ReadBit();
    if (m_BPayloadBase == 1){
        m_PayloadBase = bits.ReadBits(5)  + 1;
        if (m_PayloadBase == 0x20){
            m_PayloadBase += AP4_Ac4VariableBits(bits, 3);
        }
    }
    if (m_BitstreamVersion <= 1){
        if (m_DeprecatedV0){
            m_DeprecatedV0 = false;
            printf("Warning: Bitstream version 0 is deprecated\n");
        }
    }else{
        m_BProgramId = bits.ReadBit();
        if (m_BProgramId == 1){
            m_ShortProgramId = bits.ReadBits(16);
            m_BProgramUuidPresent = bits.ReadBit();
            if (m_BProgramUuidPresent == 1){
                for (int cnt = 0; cnt < 16; cnt++){
                    m_ProgramUuid[cnt] = bits.ReadBits(8);
                }
            }else {
                memcpy(m_ProgramUuid,"ONDEL TEAM UUID\0", 16);
            }
        }else {
            m_ShortProgramId = 0;
            m_BProgramUuidPresent = 0;
            memcpy(m_ProgramUuid, "ONDEL TEAM UUID\0", 16);
        }
        unsigned int maxGroupIndex = 0;
        unsigned int *firstPresentationSubstreamGroupIndexes = NULL;
        unsigned int firstPresentationNSubstreamGroups = 0;

        if (m_NPresentations > 0){
            m_PresentationV1 = new AP4_Dac4Atom::Ac4Dsi::PresentationV1[m_NPresentations];
            AP4_SetMemory(m_PresentationV1, 0, m_NPresentations * sizeof(m_PresentationV1[0]));
        } else {
            m_PresentationV1 = NULL;
        }
        // ac4_presentation_v1_info()
        for (unsigned int pres_idx = 0; pres_idx < m_NPresentations; pres_idx++){
            AP4_Dac4Atom::Ac4Dsi::PresentationV1& presentation = m_PresentationV1[pres_idx];
            presentation.ParsePresentationV1Info(bits,
                                                 m_BitstreamVersion,
                                                 m_FrameRateIndex,
                                                 pres_idx,
                                                 maxGroupIndex,
                                                 &firstPresentationSubstreamGroupIndexes,
                                                 firstPresentationNSubstreamGroups);            
        }
        unsigned int bObjorAjoc = 0;
        unsigned int channelCount = 0;
        unsigned int speakerGroupIndexMask = 0;
        AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 *substream_groups = new AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1[maxGroupIndex + 1];
        AP4_SetMemory(substream_groups, 0, (maxGroupIndex + 1) * sizeof(substream_groups[0]));
        for (unsigned int pres_idx = 0; pres_idx < m_NPresentations; pres_idx++){
            m_PresentationV1[pres_idx].d.v1.n_substreams_in_presentation = 0;
        }
        // ac4_substream_group_info();
        for (unsigned int sg = 0 ; (sg < (maxGroupIndex + 1)) && (m_NPresentations > 0); sg ++) {
            AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1 &substream_group = substream_groups[sg];
            AP4_Result pres_index = GetPresentationIndexBySGIndex(sg);
            if (pres_index == AP4_FAILURE) {
                break;
            }
            unsigned int localChannelCount = 0;
            unsigned int frame_rate_factor = (m_PresentationV1[pres_index].d.v1.dsi_frame_rate_multiply_info == 0)? 1: (m_PresentationV1[pres_index].d.v1.dsi_frame_rate_multiply_info * 2);
            substream_group.ParseSubstreamGroupInfo(bits,
                                                    m_BitstreamVersion,
                                                    GetPresentationVersionBySGIndex(sg),
                                                    Ap4_Ac4SubstreamGroupPartOfDefaultPresentation(sg, firstPresentationSubstreamGroupIndexes, firstPresentationNSubstreamGroups),
                                                    frame_rate_factor,
                                                    m_FsIndex,
                                                    localChannelCount,
                                                    speakerGroupIndexMask,
                                                    bObjorAjoc);
            if (channelCount < localChannelCount) { channelCount = localChannelCount;}
            m_PresentationV1[pres_index].d.v1.n_substreams_in_presentation += substream_group.d.v1.n_substreams;
        }

        for (unsigned int pres_idx = 0; pres_idx < m_NPresentations; pres_idx++){
            m_PresentationV1[pres_idx].d.v1.substream_groups = new AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1[m_PresentationV1[pres_idx].d.v1.n_substream_groups];
            for (unsigned int sg = 0; sg < m_PresentationV1[pres_idx].d.v1.n_substream_groups; sg++){
                // TODO: Default Copy Construct Function
                m_PresentationV1[pres_idx].d.v1.substream_groups[sg]   = substream_groups[m_PresentationV1[pres_idx].d.v1.substream_group_indexs[sg]];
                m_PresentationV1[pres_idx].d.v1.immersive_audio_indicator |= m_PresentationV1[pres_idx].d.v1.substream_groups[sg].d.v1.immersive_audio_indicator;
            }
        }
        delete[] substream_groups;

        if (bObjorAjoc == 0){
            m_ChannelCount = AP4_Ac4ChannelCountFromSpeakerGroupIndexMask(speakerGroupIndexMask);
        }else {
            m_ChannelCount = 2;
        }
    }

    // substream_index_table()
    AP4_UI32 n_substreams = bits.ReadBits(2);
    if (n_substreams == 0) {
        n_substreams = AP4_Ac4VariableBits(bits, 2) + 4;
    }
    AP4_UI08 b_size_present;
    if (n_substreams == 1) {
        b_size_present = bits.ReadBit();
    } else {
        b_size_present = 1;
    }
    if (b_size_present) {
        for (AP4_UI32 s = 0; s < n_substreams; s++) {
            AP4_UI08 b_more_bits = bits.ReadBit();
            AP4_UI32 substream_size = bits.ReadBits(10);
            if (b_more_bits) {
                substream_size += (AP4_Ac4VariableBits(bits, 2) << 10);
            }
            // indicates the substream size in bytes and is used to determine the size of each substream in the payload. 
            m_SubstreamSize.push_back(substream_size);
        }
    }
    if (bits.GetBitsPosition() % 8) {
        bits.SkipBits(8 - (bits.GetBitsPosition() % 8));
    }
    m_TocSize = bits.GetBitsPosition() / 8 - m_TocSize;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Header::MatchFixed
|
|    Check that two fixed headers are the same
|
+----------------------------------------------------------------------*/
bool
AP4_Ac4Header::MatchFixed(AP4_Ac4Header& frame, AP4_Ac4Header& next_frame)
{
    // Some parameter shall be const which defined in AC-4 in ISO-BMFF specs
    // TODO: More constraints will be added
    if ((frame.m_FsIndex == next_frame.m_FsIndex) &&
        (frame.m_FrameRateIndex == next_frame.m_FrameRateIndex)){
        return true;
    }else {
        return false;
    }
    
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Header::Check
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Header::Check()
{
    // Bitstream version
    if (m_BitstreamVersion != 2) {
        return AP4_FAILURE;
    }
    if ((m_FsIndex == 0  && m_FrameRateIndex != 13) || (m_FsIndex == 1 &&  m_FrameRateIndex >13)){
        return AP4_FAILURE;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Header::GetPresentationVersionBySGIndex
+----------------------------------------------------------------------*/
AP4_Result 
AP4_Ac4Header::GetPresentationVersionBySGIndex(unsigned int substream_group_index) 
{
    for (unsigned int idx = 0; idx < m_NPresentations; idx++){
        for (unsigned int sg = 0; sg < m_PresentationV1[idx].d.v1.n_substream_groups; sg++){
            if (substream_group_index ==  m_PresentationV1[idx].d.v1.substream_group_indexs[sg]) {
                return m_PresentationV1[idx].presentation_version;
            }
        }
    }
    return AP4_FAILURE;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Header::GetPresentationIndexBySGIndex
+----------------------------------------------------------------------*/
AP4_Result 
AP4_Ac4Header::GetPresentationIndexBySGIndex(unsigned int substream_group_index) 
{
    for (unsigned int idx = 0; idx < m_NPresentations; idx++){
        for (unsigned int sg = 0; sg < m_PresentationV1[idx].d.v1.n_substream_groups; sg++){
            if (substream_group_index ==  m_PresentationV1[idx].d.v1.substream_group_indexs[sg]) {
                return idx;
            }
        }
    }
    return AP4_FAILURE;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::AP4_Ac4Parser
+----------------------------------------------------------------------*/
AP4_Ac4Parser::AP4_Ac4Parser() :
    m_FrameCount(0),
    de_method(0),
    de_channel_config(0),
    de_nr_channels(1),
    de_nr_bands(8)
{
    AP4_SetMemory(de_par_prev, 0, sizeof(de_par_prev));
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::~AP4_Ac4Parser
+----------------------------------------------------------------------*/
AP4_Ac4Parser::~AP4_Ac4Parser()
{
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::Reset
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Parser::Reset()
{
    m_FrameCount = 0;

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::Feed
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Parser::Feed(const AP4_UI08* buffer, 
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
|    AP4_Ac4Parser::FindHeader
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Parser::FindHeader(AP4_UI08* header)
{
    AP4_Size available = m_Bits.GetBytesAvailable();

    /* look for the sync pattern */
    while (available-- >= AP4_AC4_HEADER_SIZE) {
        m_Bits.PeekBytes(header, 2);

        if ((((header[0] << 8) | header[1]) == AP4_AC4_SYNC_WORD_CRC) || (((header[0] << 8) | header[1]) == AP4_AC4_SYNC_WORD)) {
            /* found a sync pattern, read the entire the header */
            m_Bits.PeekBytes(header, AP4_AC4_HEADER_SIZE);
            
           return AP4_SUCCESS;
        } else {
            m_Bits.SkipBytes(1); 
        }
    }

    return AP4_ERROR_NOT_ENOUGH_DATA;
}
/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::GetSyncFrameSize
+----------------------------------------------------------------------*/
AP4_UI32
AP4_Ac4Parser::GetSyncFrameSize(AP4_BitReader &bits)
{
    unsigned int sync_word  = bits.ReadBits(16);
    unsigned int frame_size = bits.ReadBits(16);
    unsigned int head_size  = 4;
    if (frame_size == 0xFFFF){
        frame_size = bits.ReadBits(24);
        head_size += 3; 
    }
    if (sync_word == AP4_AC4_SYNC_WORD_CRC) {
        head_size += 2;
    }
    return (head_size + frame_size); 
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::FindFrame
+----------------------------------------------------------------------*/
AP4_Result
AP4_Ac4Parser::FindFrame(AP4_Ac4Frame& frame)
{
    unsigned int   available;
    unsigned char  *raw_header = new unsigned char[AP4_AC4_HEADER_SIZE];
    AP4_Result     result;

    /* align to the start of the next byte */
    m_Bits.ByteAlign();
    
    /* find a frame header by searching sync words */
    result = FindHeader(raw_header);
    if (AP4_FAILED(result)) return result;

    // duplicated work, just to get the frame size
    AP4_BitReader tmp_bits(raw_header, AP4_AC4_HEADER_SIZE);
    unsigned int sync_frame_size = GetSyncFrameSize(tmp_bits);
    if (sync_frame_size > (AP4_BITSTREAM_BUFFER_SIZE - 1)) {
        return AP4_ERROR_NOT_ENOUGH_DATA;
    }

    delete[] raw_header;
    raw_header = new unsigned char[sync_frame_size];
    /*
     * Error handling to skip the 'fake' sync word. 
     * - the maximum sync frame size is about (AP4_BITSTREAM_BUFFER_SIZE - 1) bytes.
     */
    if (m_Bits.GetBytesAvailable() < sync_frame_size ) {
        if (m_Bits.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE - 1)) {
            // skip the sync word, assume it's 'fake' sync word
            m_Bits.SkipBytes(2);
        }
        return AP4_ERROR_NOT_ENOUGH_DATA;
    }
    // copy the whole frame because toc size is unknown
    m_Bits.PeekBytes(raw_header, sync_frame_size);
    /* parse the header */
    AP4_Ac4Header ac4_header(raw_header, sync_frame_size);
    if (ac4_header.m_BitstreamVersion > 1) {
        ParseAc4SubstreamData(ac4_header, raw_header, sync_frame_size);
    }

    // Place before goto statement to resolve Xcode compiler issue
    unsigned int bit_rate_mode = 0;

    /* check the header */
    result = ac4_header.Check();
    if (AP4_FAILED(result)) {
        m_Bits.SkipBytes(sync_frame_size);
        goto fail;
    }
    
    /* check if we have enough data to peek at the next header */
    available = m_Bits.GetBytesAvailable();
    // TODO: find the proper AP4_AC4_MAX_TOC_SIZE or just parse what this step need ?
    if (available >= ac4_header.m_FrameSize + ac4_header.m_HeaderSize + ac4_header.m_CrcSize + AP4_AC4_HEADER_SIZE + AP4_AC4_MAX_TOC_SIZE) {
        // enough to peek at the header of the next frame
        unsigned char *peek_raw_header = new unsigned char[AP4_AC4_HEADER_SIZE];

        m_Bits.SkipBytes(ac4_header.m_FrameSize + ac4_header.m_HeaderSize + ac4_header.m_CrcSize);
        m_Bits.PeekBytes(peek_raw_header, AP4_AC4_HEADER_SIZE);

        // duplicated work, just to get the frame size
        AP4_BitReader peak_tmp_bits(peek_raw_header, AP4_AC4_HEADER_SIZE);
        unsigned int peak_sync_frame_size = GetSyncFrameSize(peak_tmp_bits);

        delete[] peek_raw_header;
        peek_raw_header = new unsigned char[peak_sync_frame_size];
        // copy the whole frame because toc size is unknown
        if (m_Bits.GetBytesAvailable() < (peak_sync_frame_size)) {
            peak_sync_frame_size = m_Bits.GetBytesAvailable();
        }
        m_Bits.PeekBytes(peek_raw_header, peak_sync_frame_size);

        m_Bits.SkipBytes(-((int)(ac4_header.m_FrameSize + ac4_header.m_HeaderSize + ac4_header.m_CrcSize)));

        /* check the header */
        AP4_Ac4Header peek_ac4_header(peek_raw_header, peak_sync_frame_size, true);
        result = peek_ac4_header.Check();
        if (AP4_FAILED(result)) {
            // TODO: need to reserve current sync frame ?
            m_Bits.SkipBytes(sync_frame_size + peak_sync_frame_size);
            goto fail;
        }

        /* check that the fixed part of this header is the same as the */
        /* fixed part of the previous header                           */
        if (!AP4_Ac4Header::MatchFixed(ac4_header, peek_ac4_header)) {
            // TODO: need to reserve current sync frame ?
            m_Bits.SkipBytes(sync_frame_size + peak_sync_frame_size);
            goto fail;
        }
    } else if (available < (ac4_header.m_FrameSize + ac4_header.m_HeaderSize + ac4_header.m_CrcSize) || (m_Bits.m_Flags & AP4_BITSTREAM_FLAG_EOS) == 0) {
        // not enough for a frame, or not at the end (in which case we'll want to peek at the next header)
        return AP4_ERROR_NOT_ENOUGH_DATA;
    }

    m_Bits.SkipBytes(ac4_header.m_HeaderSize);

    /* fill in the frame info */
    frame.m_Info.m_HeaderSize     = ac4_header.m_HeaderSize;
    frame.m_Info.m_FrameSize      = ac4_header.m_FrameSize;
    frame.m_Info.m_CRCSize        = ac4_header.m_CrcSize;
    frame.m_Info.m_ChannelCount   = ac4_header.m_ChannelCount; 
    frame.m_Info.m_SampleDuration = (ac4_header.m_FsIndex == 0)? 2048 : AP4_Ac4SampleDeltaTable   [ac4_header.m_FrameRateIndex];
    frame.m_Info.m_MediaTimeScale = (ac4_header.m_FsIndex == 0)? 44100: AP4_Ac4MediaTimeScaleTable[ac4_header.m_FrameRateIndex];
    frame.m_Info.m_Iframe         = ac4_header.m_BIframeGlobal;
  
    /* fill the AC4 DSI info */
    frame.m_Info.m_Ac4Dsi.ac4_dsi_version        = 1;
    frame.m_Info.m_Ac4Dsi.d.v1.bitstream_version = ac4_header.m_BitstreamVersion;
    frame.m_Info.m_Ac4Dsi.d.v1.fs_index          = ac4_header.m_FsIndex;
    frame.m_Info.m_Ac4Dsi.d.v1.fs                = AP4_Ac4SamplingFrequencyTable[frame.m_Info.m_Ac4Dsi.d.v1.fs_index];
    frame.m_Info.m_Ac4Dsi.d.v1.frame_rate_index  = ac4_header.m_FrameRateIndex;
    frame.m_Info.m_Ac4Dsi.d.v1.b_program_id      = ac4_header.m_BProgramId;
    frame.m_Info.m_Ac4Dsi.d.v1.short_program_id  = ac4_header.m_ShortProgramId;
    frame.m_Info.m_Ac4Dsi.d.v1.b_uuid            = ac4_header.m_BProgramUuidPresent;
    AP4_CopyMemory(frame.m_Info.m_Ac4Dsi.d.v1.program_uuid, ac4_header.m_ProgramUuid, 16);

    // Calcuate the bit rate mode according to ETSI TS 103 190-2 V1.2.1 Annex B 
    if (ac4_header.m_WaitFrames      == 0)                                 { bit_rate_mode = 1;}
    else if (ac4_header.m_WaitFrames >= 1 && ac4_header.m_WaitFrames <= 6) { bit_rate_mode = 2;}
    else if (ac4_header.m_WaitFrames >  6)                                 { bit_rate_mode = 3;}

    frame.m_Info.m_Ac4Dsi.d.v1.ac4_bitrate_dsi.bit_rate_mode = bit_rate_mode;
    frame.m_Info.m_Ac4Dsi.d.v1.ac4_bitrate_dsi.bit_rate = 0;                    // unknown, fixed value now
    frame.m_Info.m_Ac4Dsi.d.v1.ac4_bitrate_dsi.bit_rate_precision = 0xffffffff; // unknown, fixed value now
    frame.m_Info.m_Ac4Dsi.d.v1.n_presentations = ac4_header.m_NPresentations;
    frame.m_Info.m_Ac4Dsi.d.v1.presentations   = ac4_header.m_PresentationV1;

    /* set the frame source */
    frame.m_Source = &m_Bits;
    return AP4_SUCCESS;

fail:
    /* skip the header and return (only skip the first byte in  */
    /* case this was a false header that hides one just after)  */
    return AP4_ERROR_CORRUPTED_BITSTREAM;
} 

/*
*  Find the position of a substream in AC-4 sync frame 
*
*  | TOC                   |               | substream 0           | substream X    |
*             | substream  | payload_base  |
*             | index      |               |
*             | table      |               |
*                                          | substream_offset X    |
*  
*/
int AP4_Ac4Parser::AP4_Ac4FindSubstreamPositionInBytes(AP4_Ac4Header& header, AP4_SI32 substream_index) {
    if (substream_index < 0) {
        return -1;
    }
    AP4_Size position = header.m_HeaderSize + header.m_TocSize + header.m_PayloadBase;
    for (unsigned s = 0; s < (unsigned)substream_index; ++s) {
        if (substream_index >= header.m_SubstreamSize.size()) {
            printf("Invalid substream_index %d\n", substream_index);
            return -1;
        }
        AP4_UI32 substream_offset = header.m_SubstreamSize[s];
        position += substream_offset;    
    }
    return position;
}

bool AP4_Ac4Parser::ParseAc4SubstreamData(AP4_Ac4Header& header, const AP4_UI08* frame_bytes, unsigned int frame_size) {
    for (unsigned int pres_idx = 0; pres_idx < header.m_NPresentations; ++pres_idx) {
        auto& p = header.m_PresentationV1[pres_idx];
        // parse immersive_audio_indicator_in_es
        if (p.d.v1.ac4_presentation_substream_index >= 0) {
            AP4_BitReader bits(frame_bytes, frame_size);
            int position = AP4_Ac4FindSubstreamPositionInBytes(header, p.d.v1.ac4_presentation_substream_index);
            if (position >= 0) {
                AP4_Size size = header.m_SubstreamSize[(unsigned)p.d.v1.ac4_presentation_substream_index];
                if (frame_size - position < size) {
                    printf("Invalid ac4_presentation_substream_index %d, size %d, position %d, bits left %d\n", p.d.v1.ac4_presentation_substream_index, size, position, frame_size - position);
                    return false;
                }
                bits.SkipBits(position * 8);
                ac4_presentation_substream(bits, p);
            }
        }
        if (p.d.v1.ac4_emdf_substream_index >= 0) {
            // parse dialog_enhancement_info
            AP4_BitReader bits(frame_bytes, frame_size);
            int position = AP4_Ac4FindSubstreamPositionInBytes(header, p.d.v1.ac4_emdf_substream_index);
            if (position >= 0) {
                AP4_Size size = header.m_SubstreamSize[(unsigned)p.d.v1.ac4_emdf_substream_index];
                if (frame_size - position < size) {
                    printf("Invalid ac4_emdf_substream_index %d, size %d, position %d, bits left %d\n", p.d.v1.ac4_emdf_substream_index, size, position, frame_size - position);
                    return false;
                }
                bits.SkipBits(position * 8);
                emdf_payloads_substream(bits, p);
            }
        }
        // parse dialog_enhancement_info
        bool done = false;
        for (unsigned int sg = 0; sg < p.d.v1.n_substream_groups && !done; ++sg) {
            auto substream_group = &p.d.v1.substream_groups[sg];
            for (unsigned int s = 0; s < substream_group->d.v1.n_substreams; ++s) {
                auto substream = &substream_group->d.v1.substreams[s];
                if (substream && substream->ac4_substream_index > -1) {
                    // store the DE related info from last substream
                    AP4_UI08 b_dei_prevent_de_processing = p.d.v1.b_dei_prevent_de_processing;
                    AP4_UI08 b_dialog_max_gain = p.d.v1.b_dialog_max_gain;
                    AP4_UI08 b_de_data_present = p.d.v1.b_de_data_present;
                    AP4_UI32 dei_dialog_gain_code = p.d.v1.dei_dialog_gain_code;
                    AP4_UI08 b_dei_dialog_gain_code_present = p.d.v1.b_dei_dialog_gain_code_present;
                    int position = AP4_Ac4FindSubstreamPositionInBytes(header, substream->ac4_substream_index);
                    if (position >= 0) {
                        AP4_Size size = header.m_SubstreamSize[(unsigned)substream->ac4_substream_index];
                        AP4_BitReader bits(frame_bytes, frame_size);
                        if (frame_size - position < size) {
                            printf("Invalid ac4_substream_index %d, size %d, position %d, bits left %d\n", substream->ac4_substream_index, size, position, frame_size - position);
                            return false;
                        }
                        bits.SkipBits(position * 8);
                        // parse ac4_substream()
                        p.d.v1.b_iframe = header.m_BIframeGlobal;
                        ac4_substream(bits, p, substream_group, substream);
                        // restore the DE related info
                        p.d.v1.b_dei_prevent_de_processing |= b_dei_prevent_de_processing;
                        p.d.v1.b_dialog_max_gain |= b_dialog_max_gain;
                        p.d.v1.b_de_data_present |= b_de_data_present;
                        if (p.d.v1.b_dei_dialog_gain_code_present == 0) {
                            p.d.v1.dei_dialog_gain_code = dei_dialog_gain_code;
                            p.d.v1.b_dei_dialog_gain_code_present |= b_dei_dialog_gain_code_present;
                        }
                    }
                }
            }
        }
    }
    return true;
}

AP4_Result basic_metadata(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::SubStream* substream, AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1* substream_group);
AP4_Result extended_metadata(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::PresentationV1& p, AP4_Dac4Atom::Ac4Dsi::SubStream* substream, AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1* substream_group);
AP4_Result further_loudness_info(AP4_BitReader& bits, AP4_UI08 sus_ver, AP4_UI08 b_presentation_ldn);

/*----------------------------------------------------------------------+
|    AC4 Huffman codebook tables (ETSI TS 103 190-2 Annex A)
+----------------------------------------------------------------------*/
static const int DE_HCB_ABS_0_LEN[32] = {
    3,  3,  4,  4,  5,  5,  5,  5,  4,  4,  3,  7,  7,  7,  8,  7,
    7,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  6,  7,  7,  8,
};
static const AP4_SI32 DE_HCB_ABS_0_CW[32] = {
    0x000006, 0x000003, 0x000009, 0x000001, 0x00001f, 0x00001d, 0x000015, 0x000017,
    0x000000, 0x000003, 0x000002, 0x000079, 0x000015, 0x000010, 0x000029, 0x000078,
    0x00005a, 0x000020, 0x00003d, 0x000039, 0x000022, 0x000021, 0x000028, 0x00002c,
    0x000029, 0x000038, 0x000023, 0x00000b, 0x000009, 0x00005b, 0x000011, 0x000028,
};
static const int DE_HCB_DIFF_0_LEN[63] = {
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14,
    14, 14, 13, 13, 13, 10, 10,  9,  8,  8,  7,  6,  5,  4,  3,  1,
     3,  5,  5,  6,  7,  7,  7,  8,  8,  8, 10, 10, 10, 11, 11, 11,
    11, 12, 12, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 13,
};
static const AP4_SI32 DE_HCB_DIFF_0_CW[63] = {
    0x0002b0, 0x0002b1, 0x0002b4, 0x0002b5, 0x0002b7, 0x0002b6, 0x0002ba, 0x0002bb,
    0x0002bc, 0x00079d, 0x00148d, 0x0014b1, 0x00079e, 0x00079f, 0x00148f, 0x0014b6,
    0x0014b4, 0x0014b5, 0x00014c, 0x000159, 0x0003cc, 0x00002a, 0x00014a, 0x00003d,
    0x00000b, 0x000057, 0x000028, 0x000003, 0x000002, 0x000004, 0x000003, 0x000001,
    0x000001, 0x00000b, 0x000000, 0x000006, 0x00002a, 0x00000e, 0x000004, 0x000053,
    0x00001f, 0x000056, 0x000149, 0x000078, 0x000028, 0x000297, 0x000290, 0x0000f2,
    0x000052, 0x000522, 0x0000a7, 0x000a59, 0x0003cd, 0x00015c, 0x0014b7, 0x0014b0,
    0x00148e, 0x00148c, 0x00079c, 0x0002bf, 0x0002bd, 0x0002be, 0x00014d,
};
static const int DE_HCB_ABS_1_LEN[61] = {
     9, 12, 12, 12, 12, 12, 12, 11, 10, 11, 11, 10, 10, 10, 10, 10,
    10, 10,  9,  9,  9,  9,  8,  8,  8,  7,  6,  6,  6,  5,  1,  4,
     5,  5,  5,  5,  5,  5,  6,  6,  5,  6,  7,  6,  7,  8,  8,  9,
     9,  9,  9, 10, 10, 10, 11, 10, 11, 11, 12, 12, 10,
};
static const AP4_SI32 DE_HCB_ABS_1_CW[61] = {
    0x00015c, 0x000aea, 0x000aeb, 0x000c56, 0x000c78, 0x000c79, 0x000e51, 0x000427,
    0x000210, 0x00062a, 0x00063d, 0x000212, 0x00021d, 0x00031d, 0x0002bb, 0x000314,
    0x000390, 0x000395, 0x00010b, 0x00015f, 0x00018b, 0x0001cb, 0x000086, 0x0000ab,
    0x0000c6, 0x000054, 0x000020, 0x000024, 0x000038, 0x00001e, 0x000000, 0x00000d,
    0x00001f, 0x000017, 0x000016, 0x000014, 0x000013, 0x000011, 0x00003b, 0x00003a,
    0x000019, 0x000025, 0x000073, 0x000030, 0x000056, 0x0000c4, 0x0000aa, 0x0001c9,
    0x00015e, 0x00010f, 0x00010a, 0x000391, 0x00031c, 0x00021c, 0x000729, 0x000211,
    0x000574, 0x000426, 0x000e50, 0x000c57, 0x00031f,
};
static const int DE_HCB_DIFF_1_LEN[121] = {
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 12, 13, 13, 13, 13, 13, 13, 12, 12, 12, 11, 12,
    12, 12, 12, 12, 12, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10,  9,
     9,  9,  8,  8,  7,  7,  7,  6,  6,  5,  4,  3,  1,  4,  5,  5,
     6,  6,  7,  7,  8,  8,  8,  9,  9,  9, 10, 10, 11, 11, 11, 11,
    11, 12, 11, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13,
};
static const AP4_SI32 DE_HCB_DIFF_1_CW[121] = {
    0x001cf0, 0x001cbd, 0x001cbc, 0x001ccc, 0x001cb8, 0x001ccb, 0x001ccf, 0x001cc6,
    0x001cca, 0x001cc9, 0x001cc8, 0x001cce, 0x001cc5, 0x001cd8, 0x001cc4, 0x001cdf,
    0x001cd1, 0x001cd5, 0x001ce2, 0x001ce3, 0x000c21, 0x001cfc, 0x001cf1, 0x001cf2,
    0x001cf6, 0x001cf4, 0x001cf7, 0x000c20, 0x000c24, 0x000c25, 0x000729, 0x000d1e,
    0x000d1f, 0x000d59, 0x000d5b, 0x000d5f, 0x000e6d, 0x000e77, 0x00061b, 0x000613,
    0x0006a8, 0x0006ae, 0x000730, 0x00030a, 0x00030c, 0x000355, 0x000396, 0x00016c,
    0x0001a2, 0x0001bb, 0x0000d0, 0x0000dc, 0x00005a, 0x000069, 0x00006f, 0x000031,
    0x000038, 0x000019, 0x00000a, 0x000004, 0x000000, 0x00000f, 0x00001d, 0x000017,
    0x000036, 0x00002c, 0x00006b, 0x000060, 0x0000e4, 0x0000d4, 0x0000b7, 0x0001ba,
    0x000187, 0x00016d, 0x000395, 0x00030b, 0x00073e, 0x000728, 0x0006a9, 0x00068d,
    0x00068e, 0x000e7f, 0x000611, 0x000d5e, 0x000d5a, 0x000d58, 0x000d19, 0x000d18,
    0x000c34, 0x000c35, 0x000e74, 0x001cfd, 0x001cf3, 0x001cf5, 0x001cec, 0x001ced,
    0x001cea, 0x001ce7, 0x001ce5, 0x001ce6, 0x001ce4, 0x001cdd, 0x001ce1, 0x001ce0,
    0x001cd4, 0x001cde, 0x001cd7, 0x001cdc, 0x001cd2, 0x001cd6, 0x001ccd, 0x001cd3,
    0x001cd9, 0x001cbf, 0x001cbe, 0x001cd0, 0x001cbb, 0x001cba, 0x001cb9, 0x001cc7,
    0x001ceb,
};

// Decode one Huffman symbol from the bitstream using the given (length, codeword) table.
// Returns the symbol index on success, -1 on failure.
static int de_huff_decode(const int* len, const AP4_SI32* cw, int n, AP4_BitReader& bits) {
    AP4_UI32 code = 0;
    for (int n_bits = 1; n_bits <= 16; n_bits++) {
        code = (code << 1) | bits.ReadBit();
        for (int i = 0; i < n; i++) {
            if (len[i] == n_bits && (AP4_UI32)cw[i] == code) {
                return i;
            }
        }
    }
    return -1;
}

// Decode an absolute DE parameter using the appropriate HCB table.
static int de_abs_huffman(AP4_UI08 table_idx, AP4_BitReader& bits) {
    if (table_idx == 0) return de_huff_decode(DE_HCB_ABS_0_LEN, DE_HCB_ABS_0_CW, 32, bits);
    else                return de_huff_decode(DE_HCB_ABS_1_LEN, DE_HCB_ABS_1_CW, 61, bits);
}

// Decode a differential DE parameter using the appropriate HCB table.
// Returns a signed delta: center index 31 for DIFF_0 (63 entries), 60 for DIFF_1 (121 entries).
static int de_diff_huffman(AP4_UI08 table_idx, AP4_BitReader& bits) {
    if (table_idx == 0) {
        int idx = de_huff_decode(DE_HCB_DIFF_0_LEN, DE_HCB_DIFF_0_CW, 63, bits);
        return (idx >= 0) ? (idx - 31) : 0;
    } else {
        int idx = de_huff_decode(DE_HCB_DIFF_1_LEN, DE_HCB_DIFF_1_CW, 121, bits);
        return (idx >= 0) ? (idx - 60) : 0;
    }
}

// Map de_channel_config (3 bits) to de_nr_channels. Table 171 in ETSI TS 103 190-1 V1.4.1 (2025-07)
static void de_decode_channel_config(AP4_UI08 de_channel_config, AP4_UI08& de_nr_channels) {
    switch (de_channel_config) {
        case 0b000: de_nr_channels = 0; break; // no parameters
        case 0b001: // Centre
        case 0b010: // Right
        case 0b100: // Left
            de_nr_channels = 1;
            break;

        case 0b011: // Right + Centre
        case 0b101: // Left + Centre
        case 0b110: // Left + Right
            de_nr_channels = 2;
            break;

        case 0b111: // Left + Right + Centre
            de_nr_channels = 3;
            break;
    }
}

enum ObjectInfoStatus {
    OBJECT_INFO_DEFAULT = 0,
    OBJECT_INFO_REUSE,
    OBJECT_INFO_PART_REUSE,
    OBJECT_INFO_ALL_NEW
};

static void ext_prec_pos(AP4_BitReader& bits)
{
    AP4_UI08 ext_prec_pos_presence = bits.ReadBits(3);
    if (ext_prec_pos_presence & 0x4) bits.ReadBits(2); // ext_prec_pos3D_X
    if (ext_prec_pos_presence & 0x2) bits.ReadBits(2); // ext_prec_pos3D_Y
    if (ext_prec_pos_presence & 0x1) bits.ReadBits(2); // ext_prec_pos3D_Z
}

static AP4_UI32 add_per_object_md(AP4_BitReader& bits, bool b_object_not_active, bool b_dynamic_object)
{
    AP4_UI32 begin = bits.GetBitsPosition();

    bits.ReadBit(); // b_obj_trim_disable
    if (!b_object_not_active && b_dynamic_object) {
        AP4_UI08 b_ext_prec_pos = bits.ReadBit();
        if (b_ext_prec_pos) {
            ext_prec_pos(bits);
        }
    }
    AP4_UI08 b_headphone = bits.ReadBit();
    if (b_headphone) {
        bits.ReadBits(2); // hp_render_mode_obj
        bits.ReadBit();   // b_head_track_disable_obj
    }

    return bits.GetBitsPosition() - begin;
}

static void object_basic_info(AP4_BitReader& bits)
{
    AP4_UI08 b_default_basic_info_md = bits.ReadBit();
    if (b_default_basic_info_md == 0) {
        AP4_UI08 basic_info_md = bits.ReadBits(2);
        if (basic_info_md == 0 || basic_info_md == 2) {
            AP4_UI08 object_gain_code = bits.ReadBits(2);
            if (object_gain_code == 0) {
                bits.ReadBits(6); // object_gain_value
            }
        }
        if (basic_info_md == 2 || basic_info_md == 3) {
            bits.ReadBits(5); // object_priority_code
        }
    }
}

static void object_render_info(AP4_BitReader& bits, AP4_UI08 object_render_info_status, bool b_no_delta)
{
    AP4_UI08 b_obj_render_otherprops_present = 0;
    AP4_UI08 b_obj_render_zone_present = 0;
    AP4_UI08 b_obj_render_position_present = 0;

    if (object_render_info_status == OBJECT_INFO_ALL_NEW) {
        b_obj_render_otherprops_present = 1;
        b_obj_render_zone_present = 1;
        b_obj_render_position_present = 1;
    } else {
        b_obj_render_otherprops_present = bits.ReadBit();
        b_obj_render_zone_present = bits.ReadBit();
        b_obj_render_position_present = bits.ReadBit();
    }

    if (b_obj_render_position_present) {
        AP4_UI08 b_diff_pos_coding = b_no_delta ? 0 : bits.ReadBit();
        if (b_diff_pos_coding) {
            bits.ReadBits(3); // diff_pos3D_X
            bits.ReadBits(3); // diff_pos3D_Y
            bits.ReadBits(3); // diff_pos3D_Z
        } else {
            bits.ReadBits(6); // pos3D_X
            bits.ReadBits(6); // pos3D_Y
            bits.ReadBit();   // pos3D_Z_sign
            bits.ReadBits(4); // pos3D_Z
        }
    }

    if (b_obj_render_zone_present) {
        AP4_UI08 b_grouped_zone_defaults = bits.ReadBit();
        if (b_grouped_zone_defaults == 0) {
            AP4_UI08 group_zone_flag = bits.ReadBits(3);
            if (group_zone_flag & 0x4) {
                bits.ReadBits(3); // zone_mask
            }
        }
    }

    if (b_obj_render_otherprops_present) {
        AP4_UI08 b_grouped_other_defaults = bits.ReadBit();
        if (b_grouped_other_defaults == 0) {
            AP4_UI08 group_other_mask = bits.ReadBits(4);
            if (group_other_mask & 0x1) {
                AP4_UI08 object_width_mode = bits.ReadBit();
                if (object_width_mode == 0) {
                    bits.ReadBits(5); // object_width_code
                } else {
                    bits.ReadBits(5); // object_width_X_code
                    bits.ReadBits(5); // object_width_Y_code
                    bits.ReadBits(5); // object_width_Z_code
                }
            }
            if (group_other_mask & 0x2) {
                bits.ReadBits(3); // object_screen_factor_code
                bits.ReadBits(2); // object_depth_factor
            }
            if (group_other_mask & 0x4) {
                AP4_UI08 b_obj_at_infinity = bits.ReadBit();
                if (!b_obj_at_infinity) {
                    bits.ReadBits(4); // obj_distance_factor_code
                }
            }
            if (group_other_mask & 0x8) {
                AP4_UI08 object_div_mode = bits.ReadBits(2);
                if (object_div_mode == 0) {
                    bits.ReadBits(2); // object_div_table
                } else if (object_div_mode & 0x2) {
                    bits.ReadBits(6); // object_div_code
                }
            }
        }
    }
}

static void object_info_block(AP4_BitReader& bits, bool b_no_delta, bool b_dynamic_object)
{
    AP4_UI08 b_object_not_active = bits.ReadBit();

    AP4_UI08 object_basic_info_status;
    if (b_object_not_active) {
        object_basic_info_status = OBJECT_INFO_DEFAULT;
    } else if (b_no_delta) {
        object_basic_info_status = OBJECT_INFO_ALL_NEW;
    } else {
        AP4_UI08 b_basic_info_reuse = bits.ReadBit();
        object_basic_info_status = b_basic_info_reuse ? OBJECT_INFO_REUSE : OBJECT_INFO_ALL_NEW;
    }

    if (object_basic_info_status == OBJECT_INFO_ALL_NEW) {
        object_basic_info(bits);
    }

    AP4_UI08 object_render_info_status;
    if (b_object_not_active) {
        object_render_info_status = OBJECT_INFO_DEFAULT;
    } else if (!b_dynamic_object) {
        object_render_info_status = OBJECT_INFO_DEFAULT;
    } else if (b_no_delta) {
        object_render_info_status = OBJECT_INFO_ALL_NEW;
    } else {
        AP4_UI08 b_render_info_reuse = bits.ReadBit();
        if (b_render_info_reuse) {
            object_render_info_status = OBJECT_INFO_REUSE;
        } else {
            AP4_UI08 b_render_info_partial_reuse = bits.ReadBit();
            object_render_info_status = b_render_info_partial_reuse ? OBJECT_INFO_PART_REUSE : OBJECT_INFO_ALL_NEW;
        }
    }

    if (object_render_info_status == OBJECT_INFO_ALL_NEW ||
        object_render_info_status == OBJECT_INFO_PART_REUSE) {
        object_render_info(bits, object_render_info_status, b_no_delta);
    }

    AP4_UI08 b_add_table_data = bits.ReadBit();
    if (b_add_table_data) {
        AP4_UI08 add_table_data_size_minus1 = bits.ReadBits(4);
        AP4_UI32 atd_size = add_table_data_size_minus1 + 1;
        AP4_UI32 used_bits = add_per_object_md(bits, b_object_not_active != 0, b_dynamic_object);
        AP4_UI32 remain_bits = (8 * atd_size > used_bits) ? (8 * atd_size - used_bits) : 0;
        if (remain_bits) {
            bits.SkipBits(remain_bits);
        }
    }
}

AP4_Result AP4_Ac4Parser::ac4_substream(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::PresentationV1& presentation, AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1* substream_group, AP4_Dac4Atom::Ac4Dsi::SubStream* substream) {
    AP4_UI32 pos = bits.GetBitsPosition(); 
    // audio size in bytes
    AP4_UI32 audio_size_value = bits.ReadBits(15);
    bool b_more_bits = bits.ReadBit();
    if (b_more_bits) {
        audio_size_value += AP4_Ac4VariableBits(bits, 7) << 15;
    } 
    // skip audio size
    bits.SkipBits(audio_size_value * 8);
    // byte align
    bits.ByteAlign();
    metadata(bits, presentation, substream_group, substream);
    return AP4_SUCCESS;
}

AP4_Result AP4_Ac4Parser::metadata(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::PresentationV1& p, AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1* substream_group, AP4_Dac4Atom::Ac4Dsi::SubStream* substream) {
    basic_metadata(bits, substream, substream_group);
    extended_metadata(bits, p, substream, substream_group);
    if (p.d.v1.b_alternative && substream->b_ajoc == 0) {
        printf("unsupported bitstream with b_alternative=1 and b_ajoc=0.\n");
        exit(1);
        // TODO: parse oamd_dyndata_single() if needed, currently just throw unsupported error for any bitstream with b_alternative=1 and b_ajoc=0, as it's not expected to be used right now.
        // oamd_dyndata_single(substream, bits, b_iframe, n_blocks);
    }
    AP4_UI16 tools_metadata_size_value = bits.ReadBits(7);
    if (bits.ReadBit()) {
        tools_metadata_size_value += AP4_Ac4VariableBits(bits, 3) << 7;
    }
    if (substream_group->d.v1.sus_ver == 0) {
        // drc_frame(b_iframe);
        // TODO: parse drc_frame() for sus_ver=0 if needed, currently just throw unsupported error for any bitstream with drc_frame in sus_ver=0, as it's not expected to be used right now.
        printf("unsupported bitstream in sus_ver=0.\n");
        exit(1);
    }
    dialog_enhancement(bits, p, substream->ch_mode);
    if (bits.ReadBit()) {
        emdf_payloads_substream(bits, p);
    }
    return AP4_SUCCESS;
}

// de_data(): parse one dialog enhancement data block.
AP4_Result AP4_Ac4Parser::de_data(AP4_BitReader& bits, bool b_iframe, bool b_de_simulcast)
{
    int (*par_prev)[64] = de_par_prev[b_de_simulcast ? 1 : 0];
    int de_par[4][64];

    if (de_nr_channels == 0) return AP4_SUCCESS;

    AP4_UI08 table_idx = de_method % 2;

    // Mixing position update (methods 1 and 3, multi-channel, non-simulcast only)
    if ((de_method == 1 || de_method == 3) && de_nr_channels > 1 && !b_de_simulcast) {
        bool de_keep_pos_flag = false;
        if (!b_iframe) {
            de_keep_pos_flag = bits.ReadBit() != 0;
        }
        if (!de_keep_pos_flag) {
            bits.ReadBits(5); // de_mix_coef1_idx
            if (de_nr_channels == 3) {
                bits.ReadBits(5); // de_mix_coef2_idx
            }
        }
    }

    // Data block
    bool de_keep_data_flag = false;
    if (!b_iframe) {
        de_keep_data_flag = bits.ReadBit() != 0;
    }
    if (!de_keep_data_flag) {
        AP4_UI08 de_ms_proc_flag = 0;
        if ((de_method == 0 || de_method == 2) && de_nr_channels == 2) {
            de_ms_proc_flag = bits.ReadBit();
        }
        int ref_val = 0;
        for (int ch = 0; ch < (int)(de_nr_channels - de_ms_proc_flag); ch++) {
            if (b_iframe && ch == 0) {
                // Absolute coding for first channel, first band
                de_par[0][0] = de_abs_huffman(table_idx, bits);
                ref_val = de_par[0][0];
                par_prev[0][0] = de_par[0][0];
                // Differential coding for remaining bands
                for (int band = 1; band < de_nr_bands; band++) {
                    int delta = de_diff_huffman(table_idx, bits);
                    de_par[0][band] = ref_val + delta;
                    ref_val = de_par[0][band];
                    par_prev[0][band] = de_par[0][band];
                }
            } else {
                for (int band = 0; band < de_nr_bands; band++) {
                    if (b_iframe) {
                        // Inter-channel delta coding: reference is channel 0's values
                        int delta = de_diff_huffman(table_idx, bits);
                        de_par[ch][band] = ref_val + delta;
                        ref_val = de_par[ch][band];
                    } else {
                        // Temporal delta coding: reference is previous frame
                        int delta = de_diff_huffman(table_idx, bits);
                        de_par[ch][band] = par_prev[ch][band] + delta;
                    }
                    par_prev[ch][band] = de_par[ch][band];
                }
            }
            ref_val = de_par[ch][0];
        }
        if (de_method >= 2) {
            bits.ReadBits(5); // de_signal_contribution
        }
    }
    return AP4_SUCCESS;
}

AP4_Result AP4_Ac4Parser::dialog_enhancement(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::PresentationV1& p, AP4_UI08 ch_mode)
{
    p.d.v1.b_de_data_present = bits.ReadBit();
    if (!p.d.v1.b_de_data_present) {
        return AP4_SUCCESS;
    }
    if (p.d.v1.b_iframe) {
        // de_config();
        de_method            = bits.ReadBits(2); // de_method
        bits.ReadBits(2); // de_max_gain 
        de_channel_config = bits.ReadBits(3); // de_channel_config
    } else {
        bool b_de_config_flag = bits.ReadBit();
        if (b_de_config_flag) {
            de_method            = bits.ReadBits(2); // de_method
            bits.ReadBits(2); // de_max_gain 
            de_channel_config = bits.ReadBits(3); // de_channel_config
        }
    }
    // de_channel_config to de_nr_channels
    de_decode_channel_config(de_channel_config, de_nr_channels);

    de_data(bits, p.d.v1.b_iframe, false); // primary DE data

    if (ch_mode == 13 || ch_mode == 14) {
        AP4_UI08 b_de_simulcast = bits.ReadBit();
        if (b_de_simulcast) {
            de_data(bits, p.d.v1.b_iframe, true); // simulcast DE data
        }
    }
    return AP4_SUCCESS;
}

AP4_Result AP4_Ac4Parser::oamd_dyndata_single(AP4_Dac4Atom::Ac4Dsi::SubStream* substream,
                                                AP4_BitReader&                  bits,
                                                bool                             b_iframe,
                                                AP4_UI32                         n_blocks)
{
    for (AP4_UI08 i = 0; i < substream->n_objs; i++) {
        bool b_dynamic_object =
            (substream->obj_type[i] == AP4_Dac4Atom::Ac4Dsi::SubStream::OBJ_TYPE_DYN) &&
            (substream->obj_b_lfe[i] == 0);
        for (AP4_UI32 b = 0; b < n_blocks; b++) {
            object_info_block(bits, (b_iframe != 0) && (b == 0), b_dynamic_object);
        }
    }

    bits.ReadBit(); // b_ducking_disabled
    AP4_UI08 object_sound_category = bits.ReadBits(2);
    if (object_sound_category == 3) {
        AP4_Ac4VariableBits(bits, 2); // object_sound_category extension
    }

    AP4_UI08 n_alt_data_sets = bits.ReadBits(2);
    if (n_alt_data_sets == 3) {
        n_alt_data_sets += AP4_Ac4VariableBits(bits,2);
    }

    for (int s = 0; s < n_alt_data_sets; s++) {
        AP4_UI08 b_keep = bits.ReadBit();
        if (!b_keep) {
            AP4_UI08 n_data_points = substream->n_objs;
            if (substream->n_objs > 0 &&
                substream->obj_type[0] == AP4_Dac4Atom::Ac4Dsi::SubStream::OBJ_TYPE_ISF) {
                n_data_points = 1;
            } else {
                AP4_UI08 b_common_data = bits.ReadBit();
                if (b_common_data) {
                    n_data_points = 1;
                }
            }

            for (AP4_UI08 dp = 0; dp < n_data_points; dp++) {
                AP4_UI08 obj_type = substream->obj_type[dp];
                AP4_UI08 obj_b_lfe = substream->obj_b_lfe[dp];

                if (obj_type == AP4_Dac4Atom::Ac4Dsi::SubStream::OBJ_TYPE_BED ||
                    obj_type == AP4_Dac4Atom::Ac4Dsi::SubStream::OBJ_TYPE_ISF) {
                    AP4_UI08 b_alt_gain = bits.ReadBit();
                    if (b_alt_gain) {
                        bits.ReadBits(6); // alt_obj_gain
                    }
                } else if (obj_type == AP4_Dac4Atom::Ac4Dsi::SubStream::OBJ_TYPE_DYN) {
                    AP4_UI08 b_alt_gain = bits.ReadBit();
                    if (b_alt_gain) {
                        bits.ReadBits(6); // alt_obj_gain
                    }
                    if (obj_b_lfe == 0) {
                        AP4_UI08 b_alt_position = bits.ReadBit();
                        if (b_alt_position) {
                            bits.ReadBits(6); // alt_pos3D_X
                            bits.ReadBits(6); // alt_pos3D_Y
                            bits.ReadBit();   // alt_pos3D_Z_sign
                            bits.ReadBits(4); // alt_pos3D_Z
                        }
                    }
                }
            }
        }

        AP4_UI08 b_additional_data = bits.ReadBit();
        if (b_additional_data) {
            AP4_UI32 skip_bits = (AP4_Ac4VariableBits(bits, 2) + 1) * 8;
            bits.SkipBits(skip_bits);
        }
    }

    return AP4_SUCCESS;
}

AP4_Result basic_metadata(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::SubStream* substream, AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1* substream_group) {
    if (substream_group->d.v1.sus_ver == 0) {
        bits.ReadBits(7); // dialnorm_bits
    }
    AP4_UI08 b_more_basic_metadata = bits.ReadBit(); // b_more_basic_metadata
    if (b_more_basic_metadata) {
        if (substream_group->d.v1.sus_ver == 0) {
            AP4_UI08 b_further_loudness_info = bits.ReadBit(); 
            if (b_further_loudness_info) {
                further_loudness_info(bits, substream_group->d.v1.sus_ver, 0);
            }
        } else {
            AP4_UI08 b_further_loudness_info = bits.ReadBit(); 
            if (b_further_loudness_info) {
                AP4_UI16 substream_loudness_bits = bits.ReadBits(8);
                AP4_UI08 b_further_substream_loudness_info = bits.ReadBit();
                if (b_further_substream_loudness_info) {
                    further_loudness_info(bits, substream_group->d.v1.sus_ver, 0);
                }
            }
        }
        AP4_UI08 ch_mode = substream->ch_mode;
        AP4_UI08 sus_ver = substream_group->d.v1.sus_ver; // always 1

        // channel_mode == stereo (ch_mode 1)
        if (ch_mode == 1) {
            AP4_UI08 b_prev_dmx_info = bits.ReadBit();
            if (b_prev_dmx_info) {
                bits.ReadBits(3); // pre_dmixtyp_2ch
                bits.ReadBits(2); // phase90_info_2ch
            }
        }

        // channel_mode > stereo (ch_mode > 1)
        if (ch_mode > 1) {
            if (sus_ver == 0) {
                AP4_UI08 b_stereo_dmx_coeff = bits.ReadBit();
                if (b_stereo_dmx_coeff) {
                    bits.ReadBits(3); // loro_centre_mixgain
                    bits.ReadBits(3); // loro_surround_mixgain
                    AP4_UI08 b_loro_dmx_loud_corr = bits.ReadBit();
                    if (b_loro_dmx_loud_corr) {
                        bits.ReadBits(5); // loro_dmx_loud_corr
                    }
                    AP4_UI08 b_ltrt_mixinfo = bits.ReadBit();
                    if (b_ltrt_mixinfo) {
                        bits.ReadBits(3); // ltrt_centre_mixgain
                        bits.ReadBits(3); // ltrt_surround_mixgain
                    }
                    AP4_UI08 b_ltrt_dmx_loud_corr = bits.ReadBit();
                    if (b_ltrt_dmx_loud_corr) {
                        bits.ReadBits(5); // ltrt_dmx_loud_corr
                    }
                    // channel_mode_contains_Lfe: ch_mode in {4,6,8,10,12,14,15}
                    static const AP4_UI08 lfe_modes[] = {4,6,8,10,12,14,15};
                    bool has_lfe = false;
                    for (AP4_UI08 m : lfe_modes) { if (ch_mode == m) { has_lfe = true; break; } }
                    if (has_lfe) {
                        AP4_UI08 b_lfe_mixinfo = bits.ReadBit();
                        if (b_lfe_mixinfo) {
                            bits.ReadBits(5); // lfe_mixgain
                        }
                    }
                    bits.ReadBits(2); // preferred_dmx_method
                }
            }

            // channel_mode == 5_X: ch_mode 3 (5.0) or 4 (5.1)
            if (ch_mode == 3 || ch_mode == 4) {
                AP4_UI08 b_predmixtyp_5ch = bits.ReadBit();
                if (b_predmixtyp_5ch) {
                    bits.ReadBits(3); // pre_dmixtyp_5ch
                }
                AP4_UI08 b_preupmixtyp_5ch = bits.ReadBit();
                if (b_preupmixtyp_5ch) {
                    bits.ReadBits(4); // pre_upmixtyp_5ch
                }
            }

            // channel_mode == 7_X: ch_mode 5 - 10
            if (ch_mode >= 5 && ch_mode <= 10) {
                AP4_UI08 b_upmixtyp_7ch = bits.ReadBit();
                if (b_upmixtyp_7ch) {
                    // 3/4/0: L,C,R + Ls,Rs,Lrs,Rrs (ch_mode 5 or 6)
                    if (ch_mode == 5 || ch_mode == 6) {
                        bits.ReadBits(2); // pre_upmixtyp_3_4
                    }
                    // 3/2/2: L,C,R + Ls,Rs + Lw,Rw (ch_mode 7 or 8)
                    else if (ch_mode == 9 || ch_mode == 10) {
                        bits.ReadBit(); // pre_upmixtyp_3_2_2
                    }
                }
            }

            bits.ReadBits(2); // phase90_info_mc
            bits.ReadBit();   // b_surround_attenuation_known
            bits.ReadBit();   // b_lfe_attenuation_known
        }

        AP4_UI08 b_dc_blocking = bits.ReadBit();
        if (b_dc_blocking) {
            bits.ReadBit(); // dc_block_on
        }
    }
    return AP4_SUCCESS;
}

AP4_Result extended_metadata(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::PresentationV1& p,AP4_Dac4Atom::Ac4Dsi::SubStream* substream, AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1* substream_group) {
    AP4_UI08 b_dialog = 0;
    if (substream_group->d.v1.sus_ver >= 1) {
        b_dialog = bits.ReadBit();
    } else {
        if (p.d.v1.b_associated) {
            if (bits.ReadBit()) { // b_scale_main
                bits.ReadBits(8); // scale_main
            }
            AP4_UI08 b_scale_main_centre = bits.ReadBit();
            if (b_scale_main_centre) {
                bits.ReadBits(8); // scale_main_centre
            }
            if (bits.ReadBit()) { // b_scale_main_front
                bits.ReadBits(8); // scale_main_front
            }
            // mono
            if (substream->ch_mode == 0) {
                bits.ReadBits(8); // pan_associated
            }
        }
    }
    if (b_dialog) {
        p.d.v1.b_dialog_max_gain = bits.ReadBit();
        if (p.d.v1.b_dialog_max_gain) { // b_dialog_max_gain
            bits.ReadBits(2); // dialog_max_gain
        }
        if (bits.ReadBit()) { // b_pan_dialog_present
            if (substream->ch_mode == 0) { // mono
                bits.ReadBits(8); // pan_dialog
            } else {
                bits.ReadBits(8); // pan_dialog[0];
                bits.ReadBits(8); // pan_dialog[1];
                bits.ReadBits(2); // pan_signal_selector
           }
        }
    }
    if (bits.ReadBit()) { // b_channels_classifier
        if (substream->ch_mode == 0 || (substream->ch_mode >= 2 && substream->ch_mode <= 15)) {
            if (bits.ReadBit()) { // b_c_active
                bits.ReadBit(); // b_c_has_dialog
            }
        }
        if (substream->ch_mode >=1 && substream->ch_mode <= 15) {
            if (bits.ReadBit()) { // b_s_active
                bits.ReadBit(); // b_l_has_dialog
            }
            if (bits.ReadBit()) { // b_r_active
                bits.ReadBit(); // b_r_has_dialog
            }
        }
        AP4_UI08 cm = substream->ch_mode;
        // channel_mode_contains_LsRs: ch_mode >= 3 && ch_mode <= 15
        if (cm >= 3 && cm <= 15) {
            bits.ReadBit(); // b_ls_active
            bits.ReadBit(); // b_rs_active
        }
        // channel_mode_contains_LbRb: ch_mode == 5,6 or 11..15
        if (cm == 5 || cm == 6 || (cm >= 11 && cm <= 15)) {
            bits.ReadBit(); // b_lb_active
            bits.ReadBit(); // b_rb_active
        }
        // channel_mode_contains_LwRw: ch_mode == 7,8,15
        if (cm == 7 || cm == 8 || cm == 15) {
            bits.ReadBit(); // b_lw_active
            bits.ReadBit(); // b_rw_active
        }
        // channel_mode_contains_TflTfr: ch_mode == 9,10
        if (cm == 9 || cm == 10) {
            bits.ReadBit(); // b_tfl_active
            bits.ReadBit(); // b_tfr_active
        }
        // channel_mode_contains_Lfe: ch_mode in {4,6,8,10,12,14,15}
        if (cm == 4 || cm == 6 || cm == 8 || cm == 10 || cm == 12 || cm == 14 || cm == 15) {
            bits.ReadBit(); // b_lfe_active
        }
    }
    AP4_UI08 b_event_probability = bits.ReadBit();
    if (b_event_probability) {
        bits.ReadBits(4); // event_probability
    }
    return AP4_SUCCESS;
}

AP4_Result further_loudness_info(AP4_BitReader& bits, AP4_UI08 sus_ver, AP4_UI08 b_presentation_ldn) {
    if (b_presentation_ldn || sus_ver == 0) {
        AP4_UI08 loudness_version = bits.ReadBits(2);
        if (loudness_version == 3) {
            loudness_version += bits.ReadBits(4); // extended_loudness_version
        }
        AP4_UI08 loud_prac_type = bits.ReadBits(4);
        if (loud_prac_type != 0) {
            AP4_UI08 b_loudcorr_dialgate = bits.ReadBit();
            if (b_loudcorr_dialgate) {
                bits.ReadBits(3); // dialgate_prac_type
            }
            bits.ReadBit(); // b_loudcorr_type
        }
    } else {
        bits.ReadBit(); // b_loudcorr_dialgate
    }

    AP4_UI08 b_loudrelgat = bits.ReadBit();
    if (b_loudrelgat) {
        bits.ReadBits(11); // loudrelgat
    }
    AP4_UI08 b_loudspchgat = bits.ReadBit();
    if (b_loudspchgat) {
        bits.ReadBits(11); // loudspchgat
        bits.ReadBits(3);  // dialgate_prac_type
    }
    AP4_UI08 b_loudstrm3s = bits.ReadBit();
    if (b_loudstrm3s) {
        bits.ReadBits(11); // loudstrm3s
    }
    AP4_UI08 b_max_loudstrm3s = bits.ReadBit();
    if (b_max_loudstrm3s) {
        bits.ReadBits(11); // max_loudstrm3s
    }
    AP4_UI08 b_truepk = bits.ReadBit();
    if (b_truepk) {
        bits.ReadBits(11); // truepk
    }
    AP4_UI08 b_max_truepk = bits.ReadBit();
    if (b_max_truepk) {
        bits.ReadBits(11); // max_truepk
    }

    if (b_presentation_ldn || sus_ver == 0) {
        AP4_UI08 b_prgmbndy = bits.ReadBit();
        if (b_prgmbndy) {
            AP4_UI32 prgmbndy = 1;
            AP4_UI08 prgmbndy_bit = 0;
            while (prgmbndy_bit == 0) {
                prgmbndy <<= 1;
                prgmbndy_bit = bits.ReadBit();
            }
            bits.ReadBit(); // b_end_or_start
            AP4_UI08 b_prgmbndy_offset = bits.ReadBit();
            if (b_prgmbndy_offset) {
                bits.ReadBits(11); // prgmbndy_offset
            }
        }
    }

    AP4_UI08 b_lra = bits.ReadBit();
    if (b_lra) {
        bits.ReadBits(10); // lra
        bits.ReadBits(3);  // lra_prac_type
    }
    AP4_UI08 b_loudmntry = bits.ReadBit();
    if (b_loudmntry) {
        bits.ReadBits(11); // loudmntry
    }
    AP4_UI08 b_max_loudmntry = bits.ReadBit();
    if (b_max_loudmntry) {
        bits.ReadBits(11); // max_loudmntry
    }

    if (sus_ver >= 1) {
        AP4_UI08 b_rtllcomp = bits.ReadBit();
        if (b_rtllcomp) {
            bits.ReadBits(8); // rtll_comp
        }
        AP4_UI08 b_extension = bits.ReadBit();
        if (b_extension) {
            AP4_UI32 e_bits_size = bits.ReadBits(5);
            if (e_bits_size == 31) {
                e_bits_size += AP4_Ac4VariableBits(bits, 4);
            }
            bits.SkipBits(e_bits_size); // extensions_bits
        }
    } else {
        AP4_UI08 b_extension = bits.ReadBit();
        if (b_extension) {
            AP4_UI32 e_bits_size = bits.ReadBits(5);
            if (e_bits_size == 31) {
                e_bits_size += AP4_Ac4VariableBits(bits, 4);
            }
            AP4_UI08 b_rtllcomp = bits.ReadBit();
            if (b_rtllcomp) {
                bits.ReadBits(8); // rtll_comp
                if (e_bits_size > 9) bits.SkipBits(e_bits_size - 9); // extensions_bits
            } else {
                if (e_bits_size > 1) bits.SkipBits(e_bits_size - 1); // extensions_bits
            }
        }
    }
    return AP4_SUCCESS;
}

AP4_Result AP4_Ac4Parser::emdf_payloads_substream(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::PresentationV1& presentation) {
    AP4_UI32 emdf_payload_id = bits.ReadBits(5);
    while (emdf_payload_id != 0) {
        if (emdf_payload_id == 31) {
            emdf_payload_id += AP4_Ac4VariableBits(bits,5);
        }
        // emdf_payload_config()
        bool b_smpoffst = bits.ReadBit();
        if (b_smpoffst) {
            AP4_Ac4VariableBits(bits,11); // smpoffset
        }
        bool b_duration = bits.ReadBit();
        if (b_duration) {
            AP4_Ac4VariableBits(bits,11); // duration
        }
        bool b_groupid = bits.ReadBit();
        if (b_groupid) {
            AP4_Ac4VariableBits(bits,2); // groupid
        }
        bool b_codecdata = bits.ReadBit();
        if (b_codecdata) {
            bits.ReadBits(8); // codecdata
        }
        bool b_discard_unknown_payload = bits.ReadBit();
        if (!b_discard_unknown_payload ) {
            bool b_payload_frame_aligned = 0;
            if (b_smpoffst == 0) {
                b_payload_frame_aligned = bits.ReadBit();
                if (b_payload_frame_aligned) {
                    bits.ReadBits(2); // b_create_duplicate, b_remove_duplicate
                }
            }
            if (b_smpoffst == 1 || b_payload_frame_aligned == 1) {
                bits.ReadBits(7); // priority, proc_allowed
            }
        }
        // emdf_payload_size
        AP4_UI32 emdf_payload_size = AP4_Ac4VariableBits(bits, 8); // (in bytes)
        // emdf payload
        AP4_UI32 dei_start = bits.GetBitsPosition();
        if (emdf_payload_id == AC4ANALYZE_EMDF_ID_DEI) {
            dialog_enhancement_info(bits, presentation);
        }
        AP4_UI32 dei_end = bits.GetBitsPosition();
        // dei_skip
        AP4_UI64 remaining_bits = 8 * emdf_payload_size - (dei_end - dei_start);
        if (remaining_bits > bits.GetBitsAvailable()) {
            printf("WARNING: not enough bits for emdf payload, remaining_bits=%llu, bits_available=%llu\n", remaining_bits, bits.GetBitsAvailable());
            break;
        }
        bits.SkipBits(remaining_bits);
        emdf_payload_id = bits.ReadBits(5);
    }
    // byte align
    bits.ByteAlign();
    return AP4_SUCCESS;
}

AP4_Result AP4_Ac4Parser::dialog_enhancement_info(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::PresentationV1& presentation) {
    AP4_UI08 dei_version = bits.ReadBits(2);
    if (dei_version == 0x0) {
        presentation.d.v1.b_dei_dialog_gain_code_present = bits.ReadBit();
        if (presentation.d.v1.b_dei_dialog_gain_code_present) {
            presentation.d.v1.dei_dialog_gain_code = bits.ReadBits(6);
            presentation.d.v1.b_dei_prevent_de_processing = 1;
        } else {
            presentation.d.v1.b_dei_prevent_de_processing = bits.ReadBit();
        }
        AP4_UI08 dei_drc_offset_value = 0;
        if (presentation.d.v1.b_dei_prevent_de_processing) {
            if (presentation.d.v1.b_dei_dialog_gain_code_present != 0) {
                AP4_UI08 dei_drc_offset_code = bits.ReadBits(4);
                dei_drc_offset_value = (dei_drc_offset_code - 6)/2;
                if (dei_drc_offset_code == 0xf) {
                    AP4_UI08 dei_drc_offset_ext = bits.ReadBits(4);
                    dei_drc_offset_value = dei_drc_offset_ext + 5;
                }
            }
        }   
        bool dei_group_id_present = bits.ReadBit();
        AP4_UI08 dei_group_id = 0;
        if (dei_group_id_present != 0) {
            AP4_UI08 dei_group_id_minus1 = AP4_Ac4VariableBits(bits, 2);
            dei_group_id = dei_group_id_minus1 + 1;
        }
        bool dei_dialog_is_separated = bits.ReadBit();
        if (dei_dialog_is_separated) {
            bool dei_main_contains_dialog = bits.ReadBit();
        }
    }
    return AP4_SUCCESS;
}

AP4_Result AP4_Ac4Parser::ac4_presentation_substream(AP4_BitReader& bits, AP4_Dac4Atom::Ac4Dsi::PresentationV1& presentation) {
    if (presentation.d.v1.b_alternative) {
        bool b_name_present = bits.ReadBit();
        if (b_name_present) {
            bool b_length = bits.ReadBit();
            AP4_UI32 name_length = 0;
            if (b_length) {
                name_length = bits.ReadBits(5);
            } else {
                name_length = 32;
            }
            bits.ReadBits(name_length * 8); // presentation_name
        }
        AP4_Size n_targets = bits.ReadBits(2) + 1;
        if (n_targets == 4) {
            n_targets += AP4_Ac4VariableBits(bits, 2);
        }
        for (int t = 0; t < n_targets; t++) {
            bits.ReadBits(3); // target_level
            bits.ReadBits(4); // target_device_category
            bool b_tdc_extension = bits.ReadBit();
            if (b_tdc_extension) {
                bits.ReadBits(4); // reserved
            }
            bool b_ducking_depth_present = bits.ReadBit();
            if (b_ducking_depth_present) {
                bits.ReadBits(6); // max_ducking_depth
            }
            bool b_loud_corr_target = bits.ReadBit();
            if (b_loud_corr_target) {
                bits.ReadBits(5); // loud_corr_target
            }
            for (int sus = 0; sus < presentation.d.v1.n_substreams_in_presentation; sus++) {
                bool b_active = bits.ReadBit();
                if (b_active) {
                    uint8_t alt_data_set_index = bits.ReadBit();
                    if (alt_data_set_index == 1) {
                        alt_data_set_index += AP4_Ac4VariableBits(bits, 2);
                    }
                }
            }
        }
    }
    AP4_UI08 b_additional_data = bits.ReadBit();
    if (b_additional_data) {
        uint8_t add_data_bytes_minus1 = bits.ReadBits(4);
        uint8_t add_data_bytes = add_data_bytes_minus1 + 1;
        if (add_data_bytes == 16) {
            add_data_bytes += AP4_Ac4VariableBits(bits, 2);
        }
        AP4_UI32 add_data_bits = add_data_bytes * 8;
        unsigned mis = bits.GetBitsPosition() & 7;
        if (mis) bits.SkipBits(8 - mis);
        presentation.d.v1.immersive_audio_indicator_in_es = bits.ReadBit()? immersive_audio_indicator_TRUE:immersive_audio_indicator_FALSE;
        add_data_bits = add_data_bits - 1;
        int pres_ch_mode = presentation.GetPresentationChMode();
        if (pres_ch_mode == -1) {
            bits.ReadBit(); // b_oamd_common_timing
            add_data_bits = add_data_bits - 1;
        }
        AP4_UI08 b_advanced_de_data_present = bits.ReadBit();
        add_data_bits = add_data_bits - 1;
        if (b_advanced_de_data_present) {
            AP4_UI16 advanced_de_data_bits = 1;
            AP4_UI08 b_advanced_de_config_present = bits.ReadBit();
            if (b_advanced_de_config_present) {
                bits.ReadBits(6); // advanced_de_compr_tc_attack
                bits.ReadBits(6); // advanced_de_compr_tc_release
                bits.ReadBits(4); // advanced_de_compr_ratio
                advanced_de_data_bits += 16;
            }
            bits.ReadBits(6); // advanced_de_compr_thresh
            bits.ReadBits(5); // advanced_de_compr_gain
            advanced_de_data_bits += 11;
            add_data_bits = add_data_bits - advanced_de_data_bits;
        }
        bits.SkipBits(add_data_bits); // add_data
    } else {
        presentation.d.v1.immersive_audio_indicator_in_es = immersive_audio_indicator_NONE;
    }
    AP4_UI08 dialnorm_bits = bits.ReadBits(7);
    AP4_UI08 b_further_loudness_info = bits.ReadBit();
    if (b_further_loudness_info) {
        further_loudness_info(bits, 1, 1);
    }
    // drc_metadata
    AP4_UI32 drc_metadata_size_value = bits.ReadBits(5);
    AP4_UI32 drc_metadata_size = drc_metadata_size_value;
    AP4_UI08 b_drc_more_bits = bits.ReadBit();
    if (b_drc_more_bits) {
        drc_metadata_size += AP4_Ac4VariableBits(bits, 3) << 5;
    }
    bits.SkipBits(drc_metadata_size * 8); // drc_frame(b_pres_ndot)

    AP4_UI32 n_substream_groups = presentation.d.v1.n_substream_groups;
    if (n_substream_groups > 1) {
        AP4_UI08 b_substream_group_gains_present = bits.ReadBit();
        if (b_substream_group_gains_present) {
            AP4_UI08 b_keep = bits.ReadBit();
            if (!b_keep) {
                for (AP4_UI32 sg = 0; sg < n_substream_groups; sg++) {
                    bits.ReadBits(6); // sg_gain[sg]
                }
            }
        }
    }
    presentation.d.v1.b_associated = bits.ReadBit();
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::GetBytesFree
+----------------------------------------------------------------------*/
AP4_Size
AP4_Ac4Parser::GetBytesFree()
{
  return (m_Bits.GetBytesFree());
}

/*----------------------------------------------------------------------+
|    AP4_Ac4Parser::GetBytesAvailable
+----------------------------------------------------------------------*/
AP4_Size  
AP4_Ac4Parser::GetBytesAvailable()
{
  return (m_Bits.GetBytesAvailable());
}
