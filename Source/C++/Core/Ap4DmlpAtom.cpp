/*****************************************************************
|
|    AP4 - dmlp Atoms
|
|    Copyright 2002-2019 Axiomatic Systems, LLC
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
#include "Ap4DmlpAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_DmlpAtom)

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::Create
+---------------------------------------------------------------------*/
AP4_DmlpAtom*
AP4_DmlpAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size - AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;

    const AP4_UI08* payload = payload_data.GetData();
    return new AP4_DmlpAtom(size, payload);
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::AP4_DmlpAtom
+---------------------------------------------------------------------*/
AP4_DmlpAtom::AP4_DmlpAtom(const AP4_DmlpAtom& other) :
    AP4_Atom(AP4_ATOM_TYPE_DMLP, other.m_Size32),
    m_RawBytes(other.m_RawBytes),
    m_StreamInfo(other.m_StreamInfo) {
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::AP4_DmlpAtom
+---------------------------------------------------------------------*/
AP4_DmlpAtom::AP4_DmlpAtom(AP4_UI32 size, const StreamInfo* m_StreamInfo) :
    AP4_Atom(AP4_ATOM_TYPE_DMLP, AP4_ATOM_HEADER_SIZE) {
    AP4_BitWriter bits(size);

    bits.Write(m_StreamInfo->format_info, 32);
    bits.Write(m_StreamInfo->peak_data_rate, 15);
    bits.Write(0, 1);   // reserved
    bits.Write(0, 32);  // reserved 

    m_RawBytes.SetData(bits.GetData(), bits.GetBitCount() / 8);
    m_Size32 += m_RawBytes.GetDataSize();
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::AP4_DmlpAtom
+---------------------------------------------------------------------*/
AP4_DmlpAtom::AP4_DmlpAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_DMLP, size)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size - AP4_ATOM_HEADER_SIZE;
    m_RawBytes.SetData(payload, payload_size);

    // sanity check
    if (payload_size < 10) {
        memset(&m_StreamInfo, 0, sizeof(m_StreamInfo));
        return;
    }

    // parse the payload
    m_StreamInfo.format_info = (payload[0] << 24) | (payload[1] << 16) | (payload[2] << 8) | payload[3];
    m_StreamInfo.peak_data_rate = ((payload[4] << 7) | (payload[5] >> 1)) & 0x7fff;
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DmlpAtom::WriteFields(AP4_ByteStream& stream)
{
    return stream.Write(m_RawBytes.GetData(), m_RawBytes.GetDataSize());
}

/*----------------------------------------------------------------------
|   AP4_DmlpAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_DmlpAtom::InspectFields(AP4_AtomInspector& inspector)
{
    // inspector.AddField("data_rate", m_DataRate);
    inspector.AddField("format_info", m_StreamInfo.format_info);
    inspector.AddField("peak_data_rate", m_StreamInfo.peak_data_rate);
    return AP4_SUCCESS;
}
