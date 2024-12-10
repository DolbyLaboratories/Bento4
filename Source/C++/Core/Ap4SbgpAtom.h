/*****************************************************************
|
|    AP4 - sbgp Atoms
|
|    Copyright 2002-2014 Axiomatic Systems, LLC
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

#ifndef _AP4_SBGP_ATOM_H_
#define _AP4_SBGP_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   AP4_SbgpAtom
+---------------------------------------------------------------------*/
class AP4_SbgpAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST(AP4_SbgpAtom)

    // types
    struct Entry {
        AP4_UI32 sample_count;
        AP4_UI32 group_description_index;
    };
    
    // class methods
    static AP4_SbgpAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // methods
    AP4_SbgpAtom();
    ~AP4_SbgpAtom();
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);
    AP4_Result AddEntry(AP4_UI32 sc, AP4_UI32 idx);
    AP4_Result SetGroupType(AP4_UI32 group_type);
    AP4_Result SetGroupTypeParameter(AP4_UI32 group_type_param);
    AP4_Result ResetSize();

    // accessors
    AP4_UI32 GetGroupingType()          { return m_GroupingType;          }
    AP4_UI32 GetGroupingTypeParameter() { return m_GroupingTypeParameter; }
    AP4_Array<Entry>& GetEntries()      { return m_Entries;               }
    
private:
    // methods
    AP4_SbgpAtom(AP4_UI32        size,
                 AP4_UI08        version,
                 AP4_UI32        flags,
                 AP4_ByteStream& stream);

    // members
    AP4_UI32 m_GroupingType;
    AP4_UI32 m_GroupingTypeParameter;
    AP4_Array<Entry> m_Entries;
};

#endif // _AP4_SBGP_ATOM_H_
