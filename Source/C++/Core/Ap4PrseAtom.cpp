/*****************************************************************
|
|    AP4 - Preselection Track Group Entry Atoms
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
#include "Ap4PrseAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PrseAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK = 0x00000001;
const AP4_UI32 AP4_PRSE_FLAG_SEGMENT_ORDER_MASK      = 0x00000002;
const char* default_preselection_tag = "";

/*----------------------------------------------------------------------
|   AP4_PrseAtom::Create
+---------------------------------------------------------------------*/
AP4_PrseAtom*
AP4_PrseAtom::Create(AP4_Size         size,
                     AP4_ByteStream&  stream,
                     AP4_AtomFactory& atom_factory)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_PrseAtom(size, version, flags, stream, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::AP4_PrseAtom
+---------------------------------------------------------------------*/
AP4_PrseAtom::AP4_PrseAtom(AP4_UI32    track_group_id,
                           AP4_UI08    num_tracks,
                           const char* preselection_tag,
                           AP4_Flags   selection_priority_present,
                           AP4_UI08    selection_priority,
                           AP4_Flags   segment_order_present,
                           AP4_UI08    segment_order) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_PRSE, (AP4_UI32)0, (AP4_UI32)0)
{
    m_Size32 += 5;
    
    m_TrackGroupID = track_group_id;
    m_NumTracks = num_tracks;
    if (preselection_tag) {
        m_PreselectionTag = preselection_tag;
    } else {
        m_PreselectionTag = default_preselection_tag;
    }
    m_Size32 += m_PreselectionTag.GetLength()+1;
    if (selection_priority_present) {
        m_SelectionPriority = selection_priority;
        m_Size32 ++;
        m_Flags |= AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK;
    } else {
        m_Flags &= ~AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK;
    }
    if (segment_order_present) {
        m_SegmentOrder = segment_order;
        m_Size32 ++;
        m_Flags |= AP4_PRSE_FLAG_SEGMENT_ORDER_MASK;
    } else {
        m_Flags &= ~AP4_PRSE_FLAG_SEGMENT_ORDER_MASK;
    }
    // No children, for now. Add them later, as needed.
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::AP4_PrseAtom
+---------------------------------------------------------------------*/
AP4_PrseAtom::AP4_PrseAtom(AP4_UI32         size,
                           AP4_UI08         version,
                           AP4_UI32         flags,
                           AP4_ByteStream&  stream,
                           AP4_AtomFactory& atom_factory) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_PRSE, size, false, version, flags)
{
    AP4_Size trim = GetHeaderSize() + 5;
    stream.ReadUI32(m_TrackGroupID);
    stream.ReadUI08(m_NumTracks);
    stream.ReadNullTerminatedString(m_PreselectionTag);
    trim += m_PreselectionTag.GetLength()+1;
    if (flags & AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK) {
        stream.ReadUI08(m_SelectionPriority);
        trim++;
    }
    if (flags & AP4_PRSE_FLAG_SEGMENT_ORDER_MASK) {
        stream.ReadUI08(m_SegmentOrder);
        trim++;
    }
    ReadChildren(atom_factory, stream, size - trim);
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::SetPreselectionTag
+---------------------------------------------------------------------*/
void AP4_PrseAtom::SetPreselectionTag(const char* preselection_tag)
{
    m_Size32 -= m_PreselectionTag.GetLength()+1;
    if (preselection_tag) {
        m_PreselectionTag = preselection_tag;
    } else {
        m_PreselectionTag = default_preselection_tag;
    }
    m_Size32 += m_PreselectionTag.GetLength()+1;
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::GetSelectionPriority
+---------------------------------------------------------------------*/
AP4_Result AP4_PrseAtom::GetSelectionPriority(AP4_UI08 *selection_priority)
{
    if ((m_Flags & AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK) && selection_priority) {
        *selection_priority = m_SelectionPriority;
        return AP4_SUCCESS;
    } else {
        return AP4_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::SetSelectionPriority
+---------------------------------------------------------------------*/
void AP4_PrseAtom::SetSelectionPriority(AP4_UI08 selection_priority)
{
    if (! (m_Flags & AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK)) {
        m_Size32++;
    }
    m_SelectionPriority = selection_priority;
    m_Flags |= AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK;
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::ClearSelectionPriority
+---------------------------------------------------------------------*/
void AP4_PrseAtom::ClearSelectionPriority()
{
    if (m_Flags & AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK) {
        m_Size32--;
    }
    m_Flags &= ~AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK;
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::GetSegmentOrder
+---------------------------------------------------------------------*/
AP4_Result AP4_PrseAtom::GetSegmentOrder(AP4_UI08 *segment_order)
{
    if ((m_Flags & AP4_PRSE_FLAG_SEGMENT_ORDER_MASK) && segment_order) {
        *segment_order = m_SegmentOrder;
        return AP4_SUCCESS;
    } else {
        return AP4_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::SetSegmentOrder
+---------------------------------------------------------------------*/
void AP4_PrseAtom::SetSegmentOrder(AP4_UI08 segment_order)
{
    if (! (m_Flags & AP4_PRSE_FLAG_SEGMENT_ORDER_MASK)) {
        m_Size32++;
    }
    m_SegmentOrder = segment_order;
    m_Flags |= AP4_PRSE_FLAG_SEGMENT_ORDER_MASK;
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::ClearSegmentOrder
+---------------------------------------------------------------------*/
void AP4_PrseAtom::ClearSegmentOrder()
{
    if (m_Flags & AP4_PRSE_FLAG_SEGMENT_ORDER_MASK) {
        m_Size32--;
    }
    m_Flags &= ~AP4_PRSE_FLAG_SEGMENT_ORDER_MASK;
}


/*----------------------------------------------------------------------
|   AP4_PrseAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PrseAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    AP4_UI32   flags = this->GetFlags();
    
    result = stream.WriteUI32(m_TrackGroupID);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteUI08(m_NumTracks);
    if (AP4_FAILED(result)) return result;
    result = stream.WriteNullTerminatedString(m_PreselectionTag.GetChars());
    if (AP4_FAILED(result)) return result;
    
    if (flags & AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK) {
        result = stream.WriteUI08(m_SelectionPriority);
        if (AP4_FAILED(result)) return result;
    }
    if (flags & AP4_PRSE_FLAG_SEGMENT_ORDER_MASK) {
        result = stream.WriteUI08(m_SegmentOrder);
        if (AP4_FAILED(result)) return result;
    }

    // write all children
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|   AP4_PrseAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PrseAtom::InspectFields(AP4_AtomInspector& inspector)
{
    AP4_UI32 flags = this->GetFlags();
    
    inspector.AddField("track_group_id", m_TrackGroupID);
    inspector.AddField("num_tracks", m_NumTracks);
    inspector.AddField("preselection_tag", m_PreselectionTag.GetChars());
    if (flags & AP4_PRSE_FLAG_SELECTION_PRIORITY_MASK) {
        inspector.AddField("selection_priority", m_SelectionPriority);
    }
    if (flags & AP4_PRSE_FLAG_SEGMENT_ORDER_MASK) {
        // Decode enum when printing, raw for JSON
        if (AP4_DYNAMIC_CAST(AP4_PrintInspector, &inspector)) {
            static const char* PrseDescription[] = {
                "0: undefined",
                "1: time-ordered",
                "2: fully-ordered",
            };
            static const char* PrseDescriptionRserved = "reserved for future use";
            
            if (m_SegmentOrder < (sizeof(PrseDescription) / sizeof(PrseDescription[0]))) {
                inspector.AddField("segment_order", PrseDescription[m_SegmentOrder]);
            } else {
                char str[sizeof(PrseDescriptionRserved) + 5]; // up to 3 digits, colon, space
                AP4_FormatString(str, sizeof(str),
                                 "%u: %s",
                                 m_SegmentOrder,
                                 PrseDescriptionRserved);
                inspector.AddField("segment_order", str);
            }
        } else {
            inspector.AddField("segment_order", m_SegmentOrder);
        }
    }

    return InspectChildren(inspector);
}

