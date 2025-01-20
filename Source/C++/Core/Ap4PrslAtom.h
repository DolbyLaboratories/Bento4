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

#ifndef _AP4_PRSL_ATOM_H_
#define _AP4_PRSL_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4String.h"
#include "Ap4ContainerAtom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   AP4_PrslAtom
+---------------------------------------------------------------------*/
class AP4_PrslAtom : public AP4_ContainerAtom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_PrslAtom, AP4_Atom)

    // class methods
    static AP4_PrslAtom* Create(AP4_Size size,
                                AP4_ByteStream& stream,
                                AP4_AtomFactory& atom_factory);

    // methods
    AP4_PrslAtom(AP4_UI32       group_id,
                 const char*    preselection_tag,
                 AP4_Flags      selection_priority_present,
                 AP4_UI08       selection_priority,
                 const char*    interleaving_tag);
    
    virtual     AP4_Result      InspectFields(AP4_AtomInspector& inspector);
    virtual     AP4_Result      WriteFields(AP4_ByteStream& stream);
    virtual     AP4_Atom*       Clone();

    AP4_UI32          GetGroupID()  { return m_GroupID;  }
    void              SetGroupID(AP4_UI32 group_id) { m_GroupID = group_id; }
    AP4_UI32          GetNumEntitiesInGroup() { return m_EntityIDs.ItemCount(); }
    AP4_UI32          GetEntityID(AP4_UI32 index) { return m_EntityIDs[index]; }
    AP4_Array<AP4_UI32>& GetEntityIDs() { return m_EntityIDs; }
    AP4_Result        AddEntityID(AP4_UI32 entity_id);
    void              SetPreselectionTag(const char* preselection_tag);
    void              SetSelectionPriority(AP4_UI08 selection_priority);
    void              ClearSelectionPriority();
    void              SetInterleavingTag(const char* interleaving_tag);

private:
    // methods
    AP4_PrslAtom(AP4_UI32         size,
                 AP4_UI08         version,
                 AP4_UI32         flags,
                 AP4_ByteStream&  stream,
                 AP4_AtomFactory& atom_factory);

    // members
    AP4_UI32   m_GroupID;
    AP4_Array<AP4_UI32> m_EntityIDs;
    AP4_String m_PreselectionTag;
    AP4_UI08   m_SelectionPriority;
    AP4_String m_InterleavingTag;
};

#endif // _AP4_PRSL_ATOM_H_
