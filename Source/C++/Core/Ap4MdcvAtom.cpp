/*****************************************************************
|
|    AP4 - hvcC Atoms
|
|    Copyright 2002-2016 Axiomatic Systems, LLC
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
#include "Ap4MdcvAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_MdcvAtom)


/*----------------------------------------------------------------------
|   AP4_MdcvAtom::Create
+---------------------------------------------------------------------*/
AP4_MdcvAtom*
AP4_MdcvAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;

    return new AP4_MdcvAtom(size, payload_data.GetData());
}

/*----------------------------------------------------------------------
|   AP4_MdcvAtom::AP4_MdcvAtom
+---------------------------------------------------------------------*/
AP4_MdcvAtom::AP4_MdcvAtom() :
    AP4_Atom(AP4_ATOM_TYPE_MDCV, AP4_ATOM_HEADER_SIZE),
    m_WhitePointX(0),
    m_WhitePointY(0),
    m_MaxDisplayMasteringLuminance(0),
    m_MinDisplayMasteringLuminance(0)
{
    AP4_SetMemory(&m_DisplayPrimariesX, 0, sizeof(m_DisplayPrimariesX));
    AP4_SetMemory(&m_DisplayPrimariesY, 0, sizeof(m_DisplayPrimariesY));
}

/*----------------------------------------------------------------------
|   AP4_MdcvAtom::AP4_MdcvAtom
+---------------------------------------------------------------------*/
AP4_MdcvAtom::AP4_MdcvAtom(AP4_UI16 display_primaries_x[3],
                           AP4_UI16 display_primaries_y[3],
                           AP4_UI16 white_point_x,
                           AP4_UI16 white_point_y,
                           AP4_UI32 max_display_mastering_luminance,
                           AP4_UI32 min_display_mastering_luminance) :
    AP4_Atom(AP4_ATOM_TYPE_MDCV, AP4_ATOM_HEADER_SIZE + 24),
    m_WhitePointX(white_point_x),
    m_WhitePointY(white_point_y),
    m_MaxDisplayMasteringLuminance(max_display_mastering_luminance),
    m_MinDisplayMasteringLuminance(min_display_mastering_luminance)
{
    for (int i = 0; i < 3; i++) {
        m_DisplayPrimariesX[i] = display_primaries_x[i];
        m_DisplayPrimariesY[i] = display_primaries_y[i];
    }
}

/*----------------------------------------------------------------------
|   AP4_MdcvAtom::AP4_MdcvAtom
+---------------------------------------------------------------------*/
AP4_MdcvAtom::AP4_MdcvAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_MDCV, size)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    if (payload_size != 24) return;

    // parse the payload
    m_DisplayPrimariesX[0] = AP4_BytesToUInt16BE(&payload[0]);
    m_DisplayPrimariesY[0] = AP4_BytesToUInt16BE(&payload[2]);
    m_DisplayPrimariesX[1] = AP4_BytesToUInt16BE(&payload[4]);
    m_DisplayPrimariesY[1] = AP4_BytesToUInt16BE(&payload[6]);
    m_DisplayPrimariesX[2] = AP4_BytesToUInt16BE(&payload[8]);
    m_DisplayPrimariesY[2] = AP4_BytesToUInt16BE(&payload[10]);

    m_WhitePointX = AP4_BytesToUInt16BE(&payload[12]);
    m_WhitePointY = AP4_BytesToUInt16BE(&payload[14]);

    m_MaxDisplayMasteringLuminance = AP4_BytesToUInt32BE(&payload[16]);
    m_MinDisplayMasteringLuminance = AP4_BytesToUInt32BE(&payload[20]);

}

///*----------------------------------------------------------------------
//|   AP4_MdcvAtom::UpdateRawBytes
//+---------------------------------------------------------------------*/
//void
//AP4_MdcvAtom::UpdateRawBytes()
//{
//}

/*----------------------------------------------------------------------
|   AP4_MdcvAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_MdcvAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_UI08 payload[24];
    AP4_SetMemory(&payload, 0, 24);

    payload[0] = m_DisplayPrimariesX[0] >> 8;
    payload[1] = m_DisplayPrimariesX[0] & 0x00FF;
    payload[2] = m_DisplayPrimariesY[0] >> 8;
    payload[3] = m_DisplayPrimariesY[0] & 0x00FF;
    payload[4] = m_DisplayPrimariesX[1] >> 8;
    payload[5] = m_DisplayPrimariesX[1] & 0x00FF;
    payload[6] = m_DisplayPrimariesY[1] >> 8;
    payload[7] = m_DisplayPrimariesY[1] & 0x00FF;
    payload[8] = m_DisplayPrimariesX[2] >> 8;
    payload[9] = m_DisplayPrimariesX[2] & 0x00FF;
    payload[10] = m_DisplayPrimariesY[2] >> 8;
    payload[11] = m_DisplayPrimariesY[2] & 0x00FF;
    payload[12] = m_WhitePointX >> 8;
    payload[13] = m_WhitePointX & 0x00FF;
    payload[14] = m_WhitePointY >> 8;
    payload[15] = m_WhitePointY & 0x00FF;

    payload[16] = m_MaxDisplayMasteringLuminance >> 24;
    payload[17] = m_MaxDisplayMasteringLuminance >> 16;
    payload[18] = m_MaxDisplayMasteringLuminance >> 8;
    payload[19] = m_MaxDisplayMasteringLuminance;
    payload[20] = m_MinDisplayMasteringLuminance >> 24;
    payload[21] = m_MinDisplayMasteringLuminance >> 16;
    payload[22] = m_MinDisplayMasteringLuminance >> 8;
    payload[23] = m_MinDisplayMasteringLuminance;

    //payload[2] = (m_DvProfile << 1) | ((m_DvLevel & 0x20) >> 5);
    //payload[3] = (m_DvLevel << 3) | (m_RpuPresentFlag ? 4 : 0) | (m_ElPresentFlag ? 2 : 0) | (m_BlPresentFlag ? 1 : 0);
    //payload[4] = m_DvBlSignalCompatibilityID << 4;

    return stream.Write(payload, 24);
}

/*----------------------------------------------------------------------
|   AP4_MdcvAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_MdcvAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("Display Primaries X[0]", m_DisplayPrimariesX[0]);
    inspector.AddField("Display Primaries Y[0]", m_DisplayPrimariesY[0]);
    inspector.AddField("Display Primaries X[1]", m_DisplayPrimariesX[1]);
    inspector.AddField("Display Primaries Y[1]", m_DisplayPrimariesY[1]);
    inspector.AddField("Display Primaries X[2]", m_DisplayPrimariesX[2]);
    inspector.AddField("Display Primaries Y[2]", m_DisplayPrimariesY[2]);

    inspector.AddField("White Point X", m_WhitePointX);
    inspector.AddField("White Point Y", m_WhitePointY);

    inspector.AddField("Max Display Mastering Luminance", m_MaxDisplayMasteringLuminance);
    inspector.AddField("Min Display Mastering Luminance", m_MinDisplayMasteringLuminance);
    return AP4_SUCCESS;
}
