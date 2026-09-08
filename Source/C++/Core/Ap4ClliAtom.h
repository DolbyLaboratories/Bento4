/*****************************************************************
|
|    AP4 - hvcC Atoms
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

#ifndef _AP4_CLLI_ATOM_H_
#define _AP4_CLLI_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   AP4_ClliAtom
+---------------------------------------------------------------------*/
class AP4_ClliAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_ClliAtom, AP4_Atom)

    // class methods
    static AP4_ClliAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // constructors
    AP4_ClliAtom();
    AP4_ClliAtom(AP4_UI16 max_content_light_level,
                 AP4_UI16 max_pic_average_light_level);
    AP4_ClliAtom(AP4_UI32 size, const AP4_UI08* payload);

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI16 GetMaxContentLightLevel()           { return m_MaxContentLightLevel; }
    AP4_UI16 GetMaxPicAverageLightLevel()        { return m_MaxPicAverageLightLevel; }

private:

    // members
    AP4_UI16                  m_MaxContentLightLevel;
    AP4_UI16                  m_MaxPicAverageLightLevel;

};

#endif // _AP4_CLLI_ATOM_H_
