/*****************************************************************
|
|    AP4 - Audio Element Description Atoms
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
#include "Ap4AedbAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_AedbAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_AEDB_IS_TOGGLEABLE_FLAG_MASK         = 0x80;
const AP4_UI08 AP4_AEDB_IS_DEFAULT_ENABLED_FLAG_MASK    = 0x40;
const char* default_audio_element_tag = "";

/*----------------------------------------------------------------------
|   AP4_AedbAtom::Create
+---------------------------------------------------------------------*/
AP4_AedbAtom*
AP4_AedbAtom::Create(AP4_Size           size,
                     AP4_ByteStream&    stream,
                     AP4_AtomFactory&   atom_factory)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_AedbAtom(size, version, flags, stream, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_AedbAtom::AP4_AedbAtom
+---------------------------------------------------------------------*/
AP4_AedbAtom::AP4_AedbAtom(const char* audio_element_tag,
                           AP4_Flags   is_toggleable,
                           AP4_Flags   is_default_enabled) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_AEDB, (AP4_UI32)0, (AP4_UI32)0)
{
    
    if (audio_element_tag) {
        m_audioElementTag = audio_element_tag;
    } else {
        m_audioElementTag = default_audio_element_tag;
    }
    m_Size32 += m_audioElementTag.GetLength()+1;

    m_isToggleable = is_toggleable;
    m_isDefaultEnabled = is_default_enabled;
    m_Size32 += 1;

    // No children, for now. Add them later, as needed.
}

/*----------------------------------------------------------------------
|   AP4_AedbAtom::AP4_AedbAtom
+---------------------------------------------------------------------*/
AP4_AedbAtom::AP4_AedbAtom(AP4_UI32         size,
                           AP4_UI08         version,
                           AP4_UI32         flags,
                           AP4_ByteStream&  stream,
                           AP4_AtomFactory& atom_factory) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_AEDB, size, false, version, flags)
{
    AP4_Size trim = GetHeaderSize();
    AP4_UI08 f;

    stream.ReadNullTerminatedString(m_audioElementTag);
    trim += m_audioElementTag.GetLength()+1;

    stream.ReadUI08(f);
    trim++;
    m_isToggleable      = (f & AP4_AEDB_IS_TOGGLEABLE_FLAG_MASK) ? true : false;
    m_isDefaultEnabled  = (f & AP4_AEDB_IS_DEFAULT_ENABLED_FLAG_MASK) ? true : false;

    ReadChildren(atom_factory, stream, size - trim);
}

/*----------------------------------------------------------------------
|   AP4_AedbAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AedbAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    AP4_UI08 f = 0x00;

    result = stream.WriteNullTerminatedString(m_audioElementTag.GetChars());
    if (AP4_FAILED(result)) return result;

    if (m_isToggleable)     f |= AP4_AEDB_IS_TOGGLEABLE_FLAG_MASK;
    if (m_isDefaultEnabled) f |= AP4_AEDB_IS_DEFAULT_ENABLED_FLAG_MASK;
    result = stream.WriteUI08(f);

    // write all children
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|   AP4_AedbAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AedbAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("audio_element_tag", m_audioElementTag.GetChars());
    inspector.AddField("is_toggleable", m_isToggleable);
    inspector.AddField("is_default_enabled", m_isDefaultEnabled);

    return InspectChildren(inspector);
}

