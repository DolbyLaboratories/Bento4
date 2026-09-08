/*****************************************************************
|
|    AP4 - lhvC Atoms
|
|    Copyright 2002-2024 Axiomatic Systems, LLC
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
#include "Ap4LhvcAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"
#include "Ap4HevcParser.h"
#include "Ap4HvccAtom.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_LhvcAtom)

/*----------------------------------------------------------------------
|   AP4_LhvcAtom::GetProfileName
+---------------------------------------------------------------------*/
const char*
AP4_LhvcAtom::GetProfileName(AP4_UI08 profile_space, AP4_UI08 profile)
{
    if (profile_space != 0) {
        return NULL;
    }
    switch (profile) {
        case AP4_HEVC_PROFILE_MAIN:               return "Main";
        case AP4_HEVC_PROFILE_MAIN_10:            return "Main 10";
        case AP4_HEVC_PROFILE_MAIN_STILL_PICTURE: return "Main Still Picture";
        case AP4_HEVC_PROFILE_REXT:               return "Rext";
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_LhvcAtom::Create
+---------------------------------------------------------------------*/
AP4_LhvcAtom*
AP4_LhvcAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;
    
    return new AP4_LhvcAtom(size, payload_data.GetData());
}

/*----------------------------------------------------------------------
|   AP4_LhvcAtom::AP4_LhvcAtom
+---------------------------------------------------------------------*/
AP4_LhvcAtom::AP4_LhvcAtom() :
    AP4_Atom(AP4_ATOM_TYPE_LHVC, AP4_ATOM_HEADER_SIZE),
    m_ConfigurationVersion(1)
{
    UpdateRawBytes();
    m_Size32 += m_RawBytes.GetDataSize();
}

/*----------------------------------------------------------------------
|   AP4_LhvcAtom::AP4_LhvcAtom - Enhanced ES-based constructor
+---------------------------------------------------------------------*/
AP4_LhvcAtom::AP4_LhvcAtom(AP4_HevcFrameParser& parser, AP4_UI08 parameters_completeness) :
    AP4_Atom(AP4_ATOM_TYPE_LHVC, AP4_ATOM_HEADER_SIZE),
    m_ConfigurationVersion(1)
{
    // Extract base layer information for general fields from SPS
    AP4_HevcSequenceParameterSet* sps = parser.GetSequenceParameterSets()[0];
    if (sps) {
        m_min_spatial_segmentation = sps->vui_parameters.min_spatial_segmentation_idc;
    }
    bool all_tiles_enabled = true;
    bool all_entropy_sync_enabled = true;
    bool all_flags_zero = true;
    int i = 0;
    AP4_HevcPictureParameterSet* pps = parser.GetPictureParameterSets()[i];
    while (pps) {
        if (pps->tiles_enabled_flag != 1) all_tiles_enabled = false;
        if (pps->entropy_coding_sync_enabled_flag != 1) all_entropy_sync_enabled = false;
        if (!(pps->tiles_enabled_flag == 0 && pps->entropy_coding_sync_enabled_flag == 0))
            all_flags_zero = false;
        i++;
        pps = parser.GetPictureParameterSets()[i];
    }
    if (all_flags_zero) {
        m_parallelismType = 1; // slice-based
    } else if (all_tiles_enabled) {
        m_parallelismType = 2; // tile-based
    } else if (all_entropy_sync_enabled) {
        m_parallelismType = 3; // entropy coding sync
    } else {
        m_parallelismType = 0; // mixed or unknown
    }
    AP4_HevcVideoParameterSet* vps = parser.GetVideoParameterSets()[0];
    if (vps) {
        m_numTemporalLayers = vps->vps_max_sub_layers_minus1 + 1;
        m_temporalIdNested = vps->vps_temporal_id_nesting_flag;
    }
    m_lengthSizeMinusOne = 3;
    
    Sequence sps_sequence;
    sps_sequence.m_NaluType = AP4_HEVC_NALU_TYPE_SPS_NUT;
    sps_sequence.m_ArrayCompleteness = parameters_completeness;
    sps_sequence.m_Nalus.Append(parser.GetSequenceParameterSets()[1]->raw_bytes);
    if (sps_sequence.m_Nalus.ItemCount()) {
        m_Sequences.Append(sps_sequence);
    }

    Sequence pps_sequence;
    pps_sequence.m_NaluType = AP4_HEVC_NALU_TYPE_PPS_NUT;
    pps_sequence.m_ArrayCompleteness = parameters_completeness;
    pps_sequence.m_Nalus.Append(parser.GetPictureParameterSets()[1]->raw_bytes);
    if (pps_sequence.m_Nalus.ItemCount()) {
        m_Sequences.Append(pps_sequence);
    }
    
    UpdateRawBytes();
    m_Size32 += m_RawBytes.GetDataSize();
}

/*----------------------------------------------------------------------
|   AP4_LhvcAtom::AP4_LhvcAtom
+---------------------------------------------------------------------*/
AP4_LhvcAtom::AP4_LhvcAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_LHVC, size)
{
    // payload size
    unsigned int payload_size = size - AP4_ATOM_HEADER_SIZE;
    if (payload_size < 5) return;

    // keep raw copy
    m_RawBytes.SetData(payload, payload_size);

    // cursor
    unsigned int cursor = 0;

    // 1. configurationVersion
    m_ConfigurationVersion = payload[cursor++];

    // 2. reserved(4) + min_spatial_segmentation_idc(12)
    AP4_UI16 tmp16 = AP4_BytesToUInt16BE(&payload[cursor]);
    m_min_spatial_segmentation = tmp16 & 0x0FFF;
    cursor += 2;

    // 3. reserved(6) + parallelismType(2)
    m_parallelismType = payload[cursor] & 0x03;
    cursor += 1;

    // 4. reserved(2) + numTemporalLayers(3) + temporalIdNested(1) + lengthSizeMinusOne(2)
    m_numTemporalLayers  = (payload[cursor] >> 3) & 0x07;
    m_temporalIdNested   = (payload[cursor] >> 2) & 0x01;
    m_lengthSizeMinusOne = payload[cursor] & 0x03;
    cursor += 1;

    // 5. numOfArrays
    if (cursor >= payload_size) return;
    AP4_UI08 numOfArrays = payload[cursor++];

    // 6. arrays
    for (unsigned int i = 0; i < numOfArrays; i++) {
        if (cursor + 3 > payload_size) break;

        Sequence seq;
        AP4_UI08 header = payload[cursor++];
        seq.m_ArrayCompleteness = (header >> 7) & 0x01;
        seq.m_NaluType          = header & 0x3F;

        AP4_UI16 nalu_count = AP4_BytesToUInt16BE(&payload[cursor]);
        cursor += 2;
        seq.m_Nalus.SetItemCount(nalu_count);

        for (unsigned int j = 0; j < nalu_count; j++) {
            if (cursor + 2 > payload_size) break;
            AP4_UI16 nalu_length = AP4_BytesToUInt16BE(&payload[cursor]);
            cursor += 2;
            if (cursor + nalu_length > payload_size) break;

            seq.m_Nalus[j].SetData(&payload[cursor], nalu_length);
            cursor += nalu_length;
        }
        m_Sequences.Append(seq);
    }
}



/*----------------------------------------------------------------------
|   AP4_LhvcAtom::UpdateRawBytes
+---------------------------------------------------------------------*/
void
AP4_LhvcAtom::UpdateRawBytes()
{
    AP4_BitWriter bits(6);
    bits.Write(m_ConfigurationVersion, 8);
    bits.Write(m_Reserved1, 4);
    bits.Write(m_min_spatial_segmentation, 12);
    bits.Write(m_Reserved2, 6);
    bits.Write(m_parallelismType, 2);
    bits.Write(m_Reserved3, 2);
    bits.Write(m_numTemporalLayers, 3);
    bits.Write(m_temporalIdNested, 1);
    bits.Write(m_lengthSizeMinusOne, 2);
    uint8_t size = m_Sequences.ItemCount();
    bits.Write(size, 8);
    m_RawBytes.SetData(bits.GetData(), 6);
    for (unsigned int i=0; i<m_Sequences.ItemCount(); i++) {
        AP4_UI08 bytes[3];
        bytes[0] = (m_Sequences[i].m_ArrayCompleteness ? (1<<7) : 0) | m_Sequences[i].m_NaluType;
        AP4_BytesFromUInt16BE(&bytes[1], m_Sequences[i].m_Nalus.ItemCount());
        m_RawBytes.AppendData(bytes, 3);
        
        for (unsigned int j=0; j<m_Sequences[i].m_Nalus.ItemCount(); j++) {
            AP4_UI08 size[2];
            AP4_BytesFromUInt16BE(&size[0], (AP4_UI16)m_Sequences[i].m_Nalus[j].GetDataSize());
            m_RawBytes.AppendData(size, 2);
            m_RawBytes.AppendData(m_Sequences[i].m_Nalus[j].GetData(), m_Sequences[i].m_Nalus[j].GetDataSize());
        }
    } 
}

/*----------------------------------------------------------------------
|   AP4_LhvcAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_LhvcAtom::WriteFields(AP4_ByteStream& stream)
{
    return stream.Write(m_RawBytes.GetData(), m_RawBytes.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_LhvcAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_LhvcAtom::InspectFields(AP4_AtomInspector& inspector)
{
    return AP4_SUCCESS;
}