/*****************************************************************
|
|    AP4 - pasp Atoms
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
#include "Ap4PaspAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"
#include "Ap4HevcParser.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PaspAtom)


/*----------------------------------------------------------------------
|   AP4_PaspAtom::Create
+---------------------------------------------------------------------*/
AP4_PaspAtom*
AP4_PaspAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    unsigned int payload_size = size - AP4_ATOM_HEADER_SIZE;
    AP4_DataBuffer payload_data(payload_size);
    AP4_Result result = stream.Read(payload_data.UseData(), payload_size);
    if (AP4_FAILED(result)) return NULL;

    return new AP4_PaspAtom(size, payload_data.GetData());
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::AP4_PaspAtom
+---------------------------------------------------------------------*/
AP4_PaspAtom::AP4_PaspAtom() :
    AP4_Atom(AP4_ATOM_TYPE_PASP, AP4_ATOM_HEADER_SIZE + 8),
    m_hSpacing(0),
    m_vSpacing(0)
{
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::AP4_PaspAtom
+---------------------------------------------------------------------*/
AP4_PaspAtom::AP4_PaspAtom(AP4_UI32 hSpacing,
                           AP4_UI32 vSpacing) :
    AP4_Atom(AP4_ATOM_TYPE_PASP, AP4_ATOM_HEADER_SIZE + 8),
    m_hSpacing(hSpacing),
    m_vSpacing(vSpacing)
{
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::AP4_PaspAtom
+---------------------------------------------------------------------*/
AP4_PaspAtom::AP4_PaspAtom(AP4_UI32 size, const AP4_UI08* payload) :
    AP4_Atom(AP4_ATOM_TYPE_PASP, size)
{
    // make a copy of our configuration bytes
    unsigned int payload_size = size - AP4_ATOM_HEADER_SIZE;
    // payload size of AP4_PaspAtom is 8
    if (payload_size != 8) return;

    // parse the payload
    m_hSpacing = AP4_BytesToUInt32BE(&payload[0]);
    m_vSpacing = AP4_BytesToUInt32BE(&payload[4]);
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PaspAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result = stream.WriteUI32(m_hSpacing);
    if (AP4_FAILED(result)) {
        return result;
    }
    
    result = stream.WriteUI32(m_vSpacing);
    return result;
}

/*----------------------------------------------------------------------
|   AP4_PaspAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PaspAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("hSpacing:", m_hSpacing);
    inspector.AddField("vSpacing:", m_vSpacing);
    
    return AP4_SUCCESS;
}
