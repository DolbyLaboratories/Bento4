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
#include "Ap4ClliAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_ClliAtom)


/*----------------------------------------------------------------------
|   AP4_ClliAtom::Create
+---------------------------------------------------------------------*/
AP4_ClliAtom*
AP4_ClliAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;

    return new AP4_ClliAtom(size, payload_data.GetData());
}

/*----------------------------------------------------------------------
|   AP4_ClliAtom::AP4_ClliAtom
+---------------------------------------------------------------------*/
AP4_ClliAtom::AP4_ClliAtom() :
    AP4_Atom(AP4_ATOM_TYPE_CLLI, AP4_ATOM_HEADER_SIZE),
    m_MaxContentLightLevel(0),
    m_MaxPicAverageLightLevel(0)
{
}

/*----------------------------------------------------------------------
|   AP4_ClliAtom::AP4_ClliAtom
+---------------------------------------------------------------------*/
AP4_ClliAtom::AP4_ClliAtom(AP4_UI16 max_content_light_level,
                           AP4_UI16 max_pic_average_light_level) :
    AP4_Atom(AP4_ATOM_TYPE_CLLI, AP4_ATOM_HEADER_SIZE + 4),
    m_MaxContentLightLevel(max_content_light_level),
    m_MaxPicAverageLightLevel(max_pic_average_light_level)
{
}

/*----------------------------------------------------------------------
|   AP4_ClliAtom::AP4_ClliAtom
+---------------------------------------------------------------------*/
AP4_ClliAtom::AP4_ClliAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_CLLI, size)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size-AP4_ATOM_HEADER_SIZE;
    if (payload_size != 4) return;

    // parse the payload
    m_MaxContentLightLevel    = AP4_BytesToUInt16BE(&payload[0]);
    m_MaxPicAverageLightLevel = AP4_BytesToUInt16BE(&payload[2]);
}

///*----------------------------------------------------------------------
//|   AP4_ClliAtom::UpdateRawBytes
//+---------------------------------------------------------------------*/
//void
//AP4_ClliAtom::UpdateRawBytes()
//{
//}

/*----------------------------------------------------------------------
|   AP4_ClliAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ClliAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_UI08 payload[24];
    AP4_SetMemory(&payload, 0, 24);

    payload[0] = m_MaxContentLightLevel >> 8;
    payload[1] = m_MaxContentLightLevel & 0x00FF;
    payload[2] = m_MaxPicAverageLightLevel >> 8;
    payload[3] = m_MaxPicAverageLightLevel & 0x00FF;

    return stream.Write(payload, 4);
}

/*----------------------------------------------------------------------
|   AP4_ClliAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ClliAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("Max Content Light Level:", m_MaxContentLightLevel);
    inspector.AddField("Max Picture Average Light Level:", m_MaxPicAverageLightLevel);
    return AP4_SUCCESS;
}
