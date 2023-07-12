/*****************************************************************
|
|    AP4 - labl Atoms
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
#include "Ap4LablAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_LablAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_LABL_IS_GROUP_LABEL_MASK = 0x80;
const char* default_language = "en";
const char* default_label = "";

/*----------------------------------------------------------------------
|   AP4_LablAtom::Create
+---------------------------------------------------------------------*/
AP4_LablAtom*
AP4_LablAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_LablAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_LablAtom::AP4_LablAtom
+---------------------------------------------------------------------*/
AP4_LablAtom::AP4_LablAtom(AP4_Flags   is_group_label,
                           AP4_UI16    label_id,
                           const char* language,
                           const char* label) :
    AP4_Atom(AP4_ATOM_TYPE_LABL, AP4_FULL_ATOM_HEADER_SIZE+3, 0, 0),
        m_IsGroupLabel(is_group_label),
        m_LabelID(label_id)
{
    if (language) {
        m_Language = language;
    } else {
        m_Language = default_language;
    }
    m_Size32 += m_Language.GetLength()+1;

    if (label) {
        m_Label = label;
    } else {
        m_Label = default_label;
    }
    m_Size32 += m_Label.GetLength()+1;
}

/*----------------------------------------------------------------------
|   AP4_LablAtom::AP4_LablAtom
+---------------------------------------------------------------------*/
AP4_LablAtom::AP4_LablAtom(AP4_UI32        size,
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_LABL, size, version, flags)
{
    AP4_UI08 f;
    stream.ReadUI08(f);
    m_IsGroupLabel = (f & AP4_LABL_IS_GROUP_LABEL_MASK) ? true : false;
    stream.ReadUI16(m_LabelID);
    stream.ReadNullTerminatedString(m_Language);
    stream.ReadNullTerminatedString(m_Label);
}

/*----------------------------------------------------------------------
|   AP4_LablAtom::SetLanguage
+---------------------------------------------------------------------*/
void AP4_LablAtom::SetLanguage(const char* language)
{
    m_Size32 -= m_Language.GetLength()+1;
    if (language) {
        m_Language = language;
    } else {
        m_Language = default_language;
    }
    m_Size32 += m_Language.GetLength()+1;
}

/*----------------------------------------------------------------------
|   AP4_LablAtom::SetLabel
+---------------------------------------------------------------------*/
void AP4_LablAtom::SetLabel(const char* label)
{
    m_Size32 -= m_Label.GetLength()+1;
    if (label) {
        m_Label = label;
    } else {
        m_Label = default_label;
    }
    m_Size32 += m_Label.GetLength()+1;
}

/*----------------------------------------------------------------------
|   AP4_LablAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_LablAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    AP4_UI08 f = 0x00;
    if (m_IsGroupLabel) f |= AP4_LABL_IS_GROUP_LABEL_MASK;
    result = stream.WriteUI08(f);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI16(m_LabelID);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteNullTerminatedString(m_Language.GetChars());
    if (AP4_FAILED(result)) return result;
    result = stream.WriteNullTerminatedString(m_Label.GetChars());
    if (AP4_FAILED(result)) return result;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_LablAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_LablAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("is_group_label", m_IsGroupLabel);
    inspector.AddField("label_id", m_LabelID);
    inspector.AddField("language", m_Language.GetChars());
    inspector.AddField("label", m_Label.GetChars());

    return AP4_SUCCESS;
}
