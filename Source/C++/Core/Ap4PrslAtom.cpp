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
#include "Ap4PrslAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_PrslAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_PRSL_FLAG_PRESELECTION_TAG_MASK   = 0x00001000;
const AP4_UI32 AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK = 0x00002000;
const AP4_UI32 AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK   = 0x00004000;

/*----------------------------------------------------------------------
|   AP4_PrslAtom::Create
+---------------------------------------------------------------------*/
AP4_PrslAtom*
AP4_PrslAtom::Create(AP4_Size         size,
                     AP4_ByteStream&  stream,
                     AP4_AtomFactory& atom_factory)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_PrslAtom(size, version, flags, stream, atom_factory);
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::AP4_PrslAtom
+---------------------------------------------------------------------*/
AP4_PrslAtom::AP4_PrslAtom(AP4_UI32    group_id,
                           const char* preselection_tag,
                           AP4_Flags   selection_priority_present,
                           AP4_UI08    selection_priority,
                           const char* interleaving_tag) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_PRSL, (AP4_UI32)0, (AP4_UI32)0)
{
    m_Size32 += 8;

    m_GroupID = group_id;
    // Add entities later, one by one, with AddEntityID(AP4_UI32 entity_id)
    if (preselection_tag) {
        m_PreselectionTag = preselection_tag;
        m_Size32 += m_PreselectionTag.GetLength()+1;
        m_Flags |= AP4_PRSL_FLAG_PRESELECTION_TAG_MASK;
    } else {
        m_Flags &= ~AP4_PRSL_FLAG_PRESELECTION_TAG_MASK;
    }
    if (selection_priority_present) {
        m_SelectionPriority = selection_priority;
        m_Size32 ++;
        m_Flags |= AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK;
    } else {
        m_Flags &= ~AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK;
    }
    if (interleaving_tag) {
        m_InterleavingTag = interleaving_tag;
        m_Size32 += m_InterleavingTag.GetLength()+1;
        m_Flags |= AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK;
    } else {
        m_Flags &= ~AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK;
    }
    // No children, for now. Add them later, as needed.
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::AP4_PrslAtom
+---------------------------------------------------------------------*/
AP4_PrslAtom::AP4_PrslAtom(AP4_UI32         size,
                           AP4_UI08         version,
                           AP4_UI32         flags,
                           AP4_ByteStream&  stream,
                           AP4_AtomFactory& atom_factory) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_PRSL, size, false, version, flags)
{
    AP4_Size trim = GetHeaderSize() + 8;
    
    stream.ReadUI32(m_GroupID);
    
    AP4_UI32 num_entities_in_group = 0;
    stream.ReadUI32(num_entities_in_group);
    m_EntityIDs.SetItemCount(num_entities_in_group);
    for (unsigned int i = 0; i < num_entities_in_group; i++) {
        stream.ReadUI32(m_EntityIDs[i]);
        trim += 4;
    }
    
    if (flags & AP4_PRSL_FLAG_PRESELECTION_TAG_MASK) {
        stream.ReadNullTerminatedString(m_PreselectionTag);
        trim += m_PreselectionTag.GetLength()+1;
    }
    
    if (flags & AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK) {
        stream.ReadUI08(m_SelectionPriority);
        trim++;
    }
    
    if (flags & AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK) {
        stream.ReadNullTerminatedString(m_InterleavingTag);
        trim += m_InterleavingTag.GetLength()+1;
    }
    
    if (size > trim) {
        ReadChildren(atom_factory, stream, size - trim);
    }
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::AddEntityID
+---------------------------------------------------------------------*/
AP4_Result
AP4_PrslAtom::AddEntityID(AP4_UI32 entity_id)
{
    m_EntityIDs.Append(entity_id);
    m_Size32 += 4;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::SetPreselectionTag
+---------------------------------------------------------------------*/
void AP4_PrslAtom::SetPreselectionTag(const char* preselection_tag)
{
    if (m_Flags & AP4_PRSL_FLAG_PRESELECTION_TAG_MASK) {
        m_Size32 -= m_PreselectionTag.GetLength()+1;
    }
    m_PreselectionTag = preselection_tag;
    if (preselection_tag) {
        m_Size32 += m_PreselectionTag.GetLength()+1;
        m_Flags |= AP4_PRSL_FLAG_PRESELECTION_TAG_MASK;
    } else {
        m_Flags &= ~AP4_PRSL_FLAG_PRESELECTION_TAG_MASK;
    }
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::SetSelectionPriority
+---------------------------------------------------------------------*/
void AP4_PrslAtom::SetSelectionPriority(AP4_UI08 selection_priority)
{
    if (! (m_Flags & AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK)) {
        m_Size32++;
    }
    m_SelectionPriority = selection_priority;
    m_Flags |= AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK;
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::ClearSelectionPriority
+---------------------------------------------------------------------*/
void AP4_PrslAtom::ClearSelectionPriority()
{
    if (m_Flags & AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK) {
        m_Size32--;
    }
    m_Flags &= ~AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK;
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::SetInterleavingTag
+---------------------------------------------------------------------*/
void AP4_PrslAtom::SetInterleavingTag(const char* interleaving_tag)
{
    if (m_Flags & AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK) {
        m_Size32 -= m_InterleavingTag.GetLength()+1;
    }
    m_InterleavingTag = interleaving_tag;
    if (interleaving_tag) {
        m_Size32 += m_InterleavingTag.GetLength()+1;
        m_Flags |= AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK;
    } else {
        m_Flags &= ~AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK;
    }
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PrslAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    AP4_UI32   flags = GetFlags();
    
    result = stream.WriteUI32(m_GroupID);
    if (AP4_FAILED(result)) return result;
    
    result = stream.WriteUI32(m_EntityIDs.ItemCount());
    if (AP4_FAILED(result)) return result;
    for (unsigned int i = 0; i < m_EntityIDs.ItemCount(); i++) {
        result = stream.WriteUI32(m_EntityIDs[i]);
        if (AP4_FAILED(result)) return result;
    }
    
    if (flags & AP4_PRSL_FLAG_PRESELECTION_TAG_MASK) {
        result = stream.WriteNullTerminatedString(m_PreselectionTag.GetChars());
        if (AP4_FAILED(result)) return result;
    }
    
    if (flags & AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK) {
        result = stream.WriteUI08(m_SelectionPriority);
        if (AP4_FAILED(result)) return result;
    }
    
    if (flags & AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK) {
        result = stream.WriteNullTerminatedString(m_InterleavingTag.GetChars());
        if (AP4_FAILED(result)) return result;
    }
    // write all children
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_PrslAtom::InspectFields(AP4_AtomInspector& inspector)
{
    AP4_UI32 flags = GetFlags();
    
    inspector.AddField("group_id", m_GroupID);
    
    inspector.AddField("num_entities_in_group", m_EntityIDs.ItemCount());
    inspector.StartArray("entities_in_group", m_EntityIDs.ItemCount());
    for (unsigned int i = 0; i < m_EntityIDs.ItemCount(); i++) {
        inspector.StartObject(NULL, 1, true);
        inspector.AddField("entity_id", m_EntityIDs[i]);
        inspector.EndObject();
    }
    inspector.EndArray();
    
    if (flags & AP4_PRSL_FLAG_PRESELECTION_TAG_MASK) {
        inspector.AddField("preselection_tag", m_PreselectionTag.GetChars());
    }
    
    if (flags & AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK) {
        inspector.AddField("selection_priority", m_SelectionPriority);
    }
    
    if (flags & AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK) {
        inspector.AddField("interleaving_tag", m_InterleavingTag.GetChars());
    }

    return InspectChildren(inspector);
}

/*----------------------------------------------------------------------
|   AP4_PrslAtom::Clone
+---------------------------------------------------------------------*/
AP4_Atom*
AP4_PrslAtom::Clone()
{
    AP4_UI32        flags = GetFlags();
    AP4_PrslAtom*   clone;

    clone = new AP4_PrslAtom(m_GroupID,
                             (flags & AP4_PRSL_FLAG_PRESELECTION_TAG_MASK) ? m_PreselectionTag.GetChars() : NULL,
                             (flags & AP4_PRSL_FLAG_SELECTION_PRIORITY_MASK),
                             m_SelectionPriority,
                             (flags & AP4_PRSL_FLAG_INTERLEAVING_TAG_MASK) ? m_InterleavingTag.GetChars() : NULL);
    for (unsigned int i = 0; i < m_EntityIDs.ItemCount(); i++) {
        clone->AddEntityID(m_EntityIDs[i]);
    }

    AP4_List<AP4_Atom>::Item* child_item = m_Children.FirstItem();
    while (child_item) {
        AP4_Atom* child_clone = child_item->GetData()->Clone();
        if (child_clone) clone->AddChild(child_clone);
        child_item = child_item->GetNext();
    }

    return clone;
}

