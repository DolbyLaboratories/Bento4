/*****************************************************************
|
|    AP4 - Audio Element Prominence Interactivity Atoms
|
|    Copyright 2002-2008 Axiomatic Systems, LLC
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
#include "Ap4AeprAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_AeprAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_AEPR_SAMPLE_MERGE_FLAG_MASK = 0x80;

/*----------------------------------------------------------------------
|   AP4_AeprAtom::FloatFromInt
+---------------------------------------------------------------------*/
float
AP4_AeprAtom::FloatFromInt(AP4_UI32 value)
{
    void*    v_value = reinterpret_cast<void*>(&value);
    float*   f_value = reinterpret_cast<float*>(v_value);

    return *f_value;
}

/*----------------------------------------------------------------------
|   AP4_AeprAtom::IntFromFloat
+---------------------------------------------------------------------*/
AP4_UI32
AP4_AeprAtom::IntFromFloat(float value)
{
    void*     v_value = reinterpret_cast<void*>(&value);
    AP4_UI32* i_value = reinterpret_cast<AP4_UI32*>(v_value);

    return *i_value;
}

/*----------------------------------------------------------------------
|   AP4_AeprAtom::Create
+---------------------------------------------------------------------*/
AP4_AeprAtom*
AP4_AeprAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_AeprAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_AeprAtom::AP4_AeprAtom
+---------------------------------------------------------------------*/
AP4_AeprAtom::AP4_AeprAtom(float     min_prominence,
                           float     max_prominence,
                           float     default_prominence) :
    AP4_Atom(AP4_ATOM_TYPE_AEPR, AP4_FULL_ATOM_HEADER_SIZE+12, 0, 0),
        m_MinProminence(min_prominence),
        m_MaxProminence(max_prominence),
        m_DefaultProminence(default_prominence)
{
}

/*----------------------------------------------------------------------
|   AP4_AeprAtom::AP4_AeprAtom
+---------------------------------------------------------------------*/
AP4_AeprAtom::AP4_AeprAtom(AP4_UI32        size,
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_AEPR, size, version, flags)
{
    AP4_UI32 tmp32;

    stream.ReadUI32(tmp32);
    m_MinProminence = FloatFromInt(tmp32);

    stream.ReadUI32(tmp32);
    m_MaxProminence = FloatFromInt(tmp32);

    stream.ReadUI32(tmp32);
    m_DefaultProminence = FloatFromInt(tmp32);
}

/*----------------------------------------------------------------------
|   AP4_AeprAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AeprAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    result = stream.WriteUI32(IntFromFloat(m_MinProminence));
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(IntFromFloat(m_MaxProminence));
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI32(IntFromFloat(m_DefaultProminence));
    if (AP4_FAILED(result)) return result;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AeprAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AeprAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddFieldF("min_prominence", m_MinProminence);
    inspector.AddFieldF("max_prominence", m_MaxProminence);
    inspector.AddFieldF("default_prominence", m_DefaultProminence);

    return AP4_SUCCESS;
}
