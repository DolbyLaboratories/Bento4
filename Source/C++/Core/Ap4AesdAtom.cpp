/*****************************************************************
|
|    AP4 - Audio Element Selection Description Atoms
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
#include "Ap4AesdAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_AesdAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const char* default_selection_tag = "";

/*----------------------------------------------------------------------
|   AP4_AesdAtom::Create
+---------------------------------------------------------------------*/
AP4_AesdAtom*
AP4_AesdAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_AesdAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_AesdAtom::AP4_AesdAtom
+---------------------------------------------------------------------*/
AP4_AesdAtom::AP4_AesdAtom(const char* selection_tag) :
    AP4_Atom(AP4_ATOM_TYPE_AESD, AP4_FULL_ATOM_HEADER_SIZE, 0, 0)
{
    if (selection_tag) {
        m_SelectionTag = selection_tag;
    } else {
        m_SelectionTag = default_selection_tag;
    }
    m_Size32 += m_SelectionTag.GetLength()+1;
}

/*----------------------------------------------------------------------
|   AP4_AesdAtom::AP4_AesdAtom
+---------------------------------------------------------------------*/
AP4_AesdAtom::AP4_AesdAtom(AP4_UI32         size,
                           AP4_UI08         version,
                           AP4_UI32         flags,
                           AP4_ByteStream&  stream) :
    AP4_Atom(AP4_ATOM_TYPE_AESD, size, version, flags)
{
    stream.ReadNullTerminatedString(m_SelectionTag);
}

/*----------------------------------------------------------------------
|   AP4_AesdAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AesdAtom::WriteFields(AP4_ByteStream& stream)
{
    return stream.WriteNullTerminatedString(m_SelectionTag.GetChars());
}

/*----------------------------------------------------------------------
|   AP4_AesdAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AesdAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("selection_tag", m_SelectionTag.GetChars());

    return AP4_SUCCESS;
}

