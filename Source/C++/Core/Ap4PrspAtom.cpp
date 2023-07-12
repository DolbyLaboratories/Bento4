/*****************************************************************
|
|    AP4 - Preselection Processing Atoms
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
#include "Ap4PrspAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PrspAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_PRSP_SAMPLE_MERGE_FLAG_MASK = 0x80;

/*----------------------------------------------------------------------
|   AP4_PrspAtom::Create
+---------------------------------------------------------------------*/
AP4_PrspAtom*
AP4_PrspAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_PrspAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_PrspAtom::AP4_PrspAtom
+---------------------------------------------------------------------*/
AP4_PrspAtom::AP4_PrspAtom(AP4_UI08    track_order,
                           AP4_Flags   sample_merge_flag) :
    AP4_Atom(AP4_ATOM_TYPE_PRSP, AP4_FULL_ATOM_HEADER_SIZE+2, 0, 0),
        m_TrackOrder(track_order),
        m_SampleMergeFlag(sample_merge_flag)
{
}

/*----------------------------------------------------------------------
|   AP4_PrspAtom::AP4_PrspAtom
+---------------------------------------------------------------------*/
AP4_PrspAtom::AP4_PrspAtom(AP4_UI32        size,
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_PRSP, size, version, flags)
{
    stream.ReadUI08(m_TrackOrder);

    AP4_UI08 f;
    stream.ReadUI08(f);
    m_SampleMergeFlag = (f & AP4_PRSP_SAMPLE_MERGE_FLAG_MASK) ? true : false;
}

/*----------------------------------------------------------------------
|   AP4_PrspAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PrspAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    result = stream.WriteUI08(m_TrackOrder);
    if (AP4_FAILED(result)) return result;

    AP4_UI08 f = 0x00;
    if (m_SampleMergeFlag) f |= AP4_PRSP_SAMPLE_MERGE_FLAG_MASK;
    result = stream.WriteUI08(f);
    if (AP4_FAILED(result)) return result;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PrspAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PrspAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("track_order", m_TrackOrder);
    inspector.AddField("sample_merge_flag", m_SampleMergeFlag);

    return AP4_SUCCESS;
}
