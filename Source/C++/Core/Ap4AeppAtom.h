/*****************************************************************
|
|    AP4 - Audio Element Positioning Interactivity Polar Atoms 
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

#ifndef _AP4_AEPP_ATOM_H_
#define _AP4_AEPP_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4String.h"

/*----------------------------------------------------------------------
|   AP4_AeppAtom
+---------------------------------------------------------------------*/
class AP4_AeppAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_AeppAtom, AP4_Atom)

    // class methods
    static AP4_AeppAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // methods
    AP4_AeppAtom(AP4_Flags azimuth,
                 AP4_SI16 min_azimuth,
                 AP4_SI16 max_azimuth,
                 AP4_SI16 default_azimuth,
                 AP4_Flags elevation,
                 AP4_SI08 min_elevation,
                 AP4_SI08 max_elevation,
                 AP4_SI08 default_elevation,
                 AP4_Flags distance,
                 AP4_UI32 min_distance,
                 AP4_UI32 max_distance,
                 AP4_UI32 default_distance);
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

private:
    // methods
    AP4_AeppAtom(AP4_UI32        size,
                 AP4_UI08        version,
                 AP4_UI32        flags,
                 AP4_ByteStream& stream);

    // members
    AP4_SI16 m_MinAzimuth;
    AP4_SI16 m_MaxAzimuth;
    AP4_SI16 m_DefaultAzimuth;
    AP4_SI08 m_MinElevation;
    AP4_SI08 m_MaxElevation;
    AP4_SI08 m_DefaultElevation;
    AP4_UI32 m_MinDistance;
    AP4_UI32 m_MaxDistance;
    AP4_UI32 m_DefaultDistance;
};

#endif // _AP4_AEPP_ATOM_H_
