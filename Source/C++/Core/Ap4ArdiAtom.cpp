/*****************************************************************
|
|    AP4 - Audio Rendering Indication Atoms
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
#include "Ap4ArdiAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_ArdiAtom)

/*----------------------------------------------------------------------
|   AP4_ArdiAtom::Create
+---------------------------------------------------------------------*/
AP4_ArdiAtom*
AP4_ArdiAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_ArdiAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_ArdiAtom::AP4_ArdiAtom
+---------------------------------------------------------------------*/
AP4_ArdiAtom::AP4_ArdiAtom(AP4_UI08 audio_rendering_indication) :
    AP4_Atom(AP4_ATOM_TYPE_ARDI, AP4_FULL_ATOM_HEADER_SIZE+1, 0, 0),
        m_AudioRenderingIndication(audio_rendering_indication)
{
}

/*----------------------------------------------------------------------
|   AP4_ArdiAtom::AP4_ArdiAtom
+---------------------------------------------------------------------*/
AP4_ArdiAtom::AP4_ArdiAtom(AP4_UI32        size,
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_ARDI, size, version, flags)
{
    stream.ReadUI08(m_AudioRenderingIndication);
}

/*----------------------------------------------------------------------
|   AP4_ArdiAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ArdiAtom::WriteFields(AP4_ByteStream& stream)
{
    return stream.WriteUI08(m_AudioRenderingIndication);
}

/*----------------------------------------------------------------------
|   AP4_ArdiAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ArdiAtom::InspectFields(AP4_AtomInspector& inspector)
{
    static const char* ArdiDescription[] = {
        "0: no preference given for the reproduction channel layout",
        "1: preferred reproduction channel layout is stereo",
        "2: preferred reproduction channel layout is two-dimensional (e.g. 5.1 multi-channel)",
        "3: preferred reproduction channel layout is three-dimensional",
        "4: content is pre-rendered for consumption with headphones",
    };
    static const char ArdiDescriptionReserved[] = "reserved for future use";

    // Decode enum when printing, raw for JSON
    if (AP4_DYNAMIC_CAST(AP4_PrintInspector, &inspector)) {
        if (m_AudioRenderingIndication < (sizeof(ArdiDescription) / sizeof(ArdiDescription[0]))) {
            inspector.AddField("audio_rendering_indication", ArdiDescription[m_AudioRenderingIndication]);
        } else {
            char str[sizeof(ArdiDescriptionReserved) + 5]; // up to 3 digits, colon, space
            AP4_FormatString(str, sizeof(str),
                             "%u: %s",
                             m_AudioRenderingIndication,
                             ArdiDescriptionReserved);
            inspector.AddField("audio_rendering_indication", str);
        }
    } else {
        inspector.AddField("audio_rendering_indication", m_AudioRenderingIndication);
    }

    return AP4_SUCCESS;
}
