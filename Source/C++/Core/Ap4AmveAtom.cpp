/*****************************************************************
|
|    AP4 - amve Atoms
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
#include "Ap4AmveAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_AmveAtom)


/*----------------------------------------------------------------------
|   AP4_AmveAtom::Create
+---------------------------------------------------------------------*/
AP4_AmveAtom*
AP4_AmveAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size - AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;

    return new AP4_AmveAtom(size, payload_data.GetData());
}

/*----------------------------------------------------------------------
|   AP4_AmveAtom::AP4_AmveAtom
+---------------------------------------------------------------------*/
AP4_AmveAtom::AP4_AmveAtom() :
    AP4_Atom(AP4_ATOM_TYPE_AMVE, AP4_ATOM_HEADER_SIZE + 8),
    m_AmbientIlluminance(0),
    m_AmbientLightX(0),
    m_AmbientLightY(0)
{
}

/*----------------------------------------------------------------------
|   AP4_AmveAtom::AP4_AmveAtom
+---------------------------------------------------------------------*/
AP4_AmveAtom::AP4_AmveAtom(AP4_UI32 ambient_illuminance,
                           AP4_UI16 ambient_light_x,
                           AP4_UI16 ambient_light_y) :
    AP4_Atom(AP4_ATOM_TYPE_AMVE, AP4_ATOM_HEADER_SIZE + 8),
    m_AmbientIlluminance(ambient_illuminance),
    m_AmbientLightX(ambient_light_x),
    m_AmbientLightY(ambient_light_y)
{
}

/*----------------------------------------------------------------------
|   AP4_AmveAtom::AP4_AmveAtom
+---------------------------------------------------------------------*/
AP4_AmveAtom::AP4_AmveAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_AMVE, size)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size - AP4_ATOM_HEADER_SIZE;
    // payload size of AmveAtom is 8
    if (payload_size != 8) return;

    // parse the payload
    m_AmbientIlluminance = AP4_BytesToUInt32BE(&payload[0]);
    m_AmbientLightX = AP4_BytesToUInt16BE(&payload[4]);
    m_AmbientLightY = AP4_BytesToUInt16BE(&payload[6]);
}

/*----------------------------------------------------------------------
|   AP4_AmveAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AmveAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_UI08 payload[8];
    AP4_SetMemory(&payload, 0, 8);

    payload[0] = m_AmbientIlluminance >> 24;
    payload[1] = (m_AmbientIlluminance >> 16) & 0xFF;
    payload[2] = (m_AmbientIlluminance >> 8) & 0xFF;
    payload[3] = m_AmbientIlluminance & 0xFF;
    
    payload[4] = m_AmbientLightX >> 8;
    payload[5] = m_AmbientLightX & 0xFF;
    
    payload[6] = m_AmbientLightY >> 8;
    payload[7] = m_AmbientLightY & 0xFF;

    return stream.Write(payload, 8);
}

/*----------------------------------------------------------------------
|   AP4_AmveAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AmveAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("Ambient Illuminance:", m_AmbientIlluminance);
    inspector.AddField("Ambient Light X:", m_AmbientLightX);
    inspector.AddField("Ambient Light Y:", m_AmbientLightY);
    
    return AP4_SUCCESS;
}
