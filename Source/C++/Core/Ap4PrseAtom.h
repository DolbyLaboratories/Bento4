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

#ifndef _AP4_PRSE_ATOM_H_
#define _AP4_PRSE_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4String.h"
#include "Ap4ContainerAtom.h"

/*----------------------------------------------------------------------
|   AP4_PrseAtom
+---------------------------------------------------------------------*/
class AP4_PrseAtom : public AP4_ContainerAtom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_PrseAtom, AP4_ContainerAtom)

    // class methods
    static AP4_PrseAtom* Create(AP4_Size size,
                                AP4_ByteStream& stream,
                                AP4_AtomFactory& atom_factory);

    // methods
    AP4_PrseAtom(AP4_UI32    track_group_id,
                 AP4_UI08    num_tracks,
                 const char* preselection_tag,
                 AP4_Flags   selection_priority_present,
                 AP4_UI08    selection_priority,
                 AP4_Flags   segment_order_present,
                 AP4_UI08    segment_order);
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    AP4_UI32          GetTrackGroupID()  { return m_TrackGroupID;  }
    void              SetTrackGroupID(AP4_UI32 track_group_id) { m_TrackGroupID = track_group_id; }
    AP4_UI08          GetNumTracks() { return m_NumTracks; }
    void              SetNumTracks(AP4_UI08 num_tracks) { m_NumTracks = num_tracks; }
    const AP4_String& GetPreselectionTag()  { return m_PreselectionTag;  }
    void              SetPreselectionTag(const char* preselection_tag);
    AP4_Result        GetSelectionPriority(AP4_UI08 *selection_priority);
    void              SetSelectionPriority(AP4_UI08 selection_priority);
    void              ClearSelectionPriority();
    AP4_Result        GetSegmentOrder(AP4_UI08 *segment_order);
    void              SetSegmentOrder(AP4_UI08 segment_order);
    void              ClearSegmentOrder();

private:
    // methods
    AP4_PrseAtom(AP4_UI32         size,
                 AP4_UI08         version,
                 AP4_UI32         flags,
                 AP4_ByteStream&  stream,
                 AP4_AtomFactory& atom_factory);

    // members
    AP4_UI32   m_TrackGroupID;
    AP4_UI08   m_NumTracks;
    AP4_String m_PreselectionTag;
    AP4_UI08   m_SelectionPriority;
    AP4_UI08   m_SegmentOrder;
};

#endif // _AP4_PRSE_ATOM_H_
