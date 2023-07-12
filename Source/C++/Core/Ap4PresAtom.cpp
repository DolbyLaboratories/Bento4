/*****************************************************************
|
|    AP4 - Preselection Group Atoms
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
#include "Ap4PresAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PresAtom)

/*----------------------------------------------------------------------
|   AP4_PresAtom::Create
+---------------------------------------------------------------------*/
AP4_PresAtom*
AP4_PresAtom::Create(AP4_Size         size,
                     AP4_ByteStream&  stream,
                     AP4_AtomFactory& atom_factory)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_PresAtom(size, version, flags, stream, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_PresAtom::AP4_PresAtom
+---------------------------------------------------------------------*/
AP4_PresAtom::AP4_PresAtom(AP4_UI32    track_group_id) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_PRES, (AP4_UI32)0, (AP4_UI32)0)
{
    m_Size32 += 4;

    m_TrackGroupID = track_group_id;
}

/*----------------------------------------------------------------------
|   AP4_PresAtom::AP4_PresAtom
+---------------------------------------------------------------------*/
AP4_PresAtom::AP4_PresAtom(AP4_UI32         size,
                           AP4_UI08         version,
                           AP4_UI32         flags,
                           AP4_ByteStream&  stream,
                           AP4_AtomFactory& atom_factory) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_PRES, size, false, version, flags)
{
    AP4_Size trim = GetHeaderSize() + 4;
    stream.ReadUI32(m_TrackGroupID);
    if (size > trim) {
        ReadChildren(atom_factory, stream, size - trim);
    }
}

/*----------------------------------------------------------------------
|   AP4_PresAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PresAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    result = stream.WriteUI32(m_TrackGroupID);
    if (AP4_FAILED(result)) return result;
    
    // write all children
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|   AP4_PresAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PresAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("track_group_id", m_TrackGroupID);
    
    return InspectChildren(inspector);
}
