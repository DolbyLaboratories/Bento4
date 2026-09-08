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

#ifndef _AP4_MDCV_ATOM_H_
#define _AP4_MDCV_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   AP4_MdcvAtom
+---------------------------------------------------------------------*/
class AP4_MdcvAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_MdcvAtom, AP4_Atom)

    // class methods
    static AP4_MdcvAtom* Create(AP4_Size size, AP4_ByteStream& stream);

    // constructors
    AP4_MdcvAtom();
    AP4_MdcvAtom(AP4_UI32 size, const AP4_UI08* payload);
    AP4_MdcvAtom(AP4_UI16 display_primaries_x[3],
                 AP4_UI16 display_primaries_y[3],
                 //AP4_UI16 display_primaries_bx,
                 //AP4_UI16 display_primaries_by,
                 //AP4_UI16 display_primaries_rx,
                 //AP4_UI16 display_primaries_ry,
                 AP4_UI16 white_point_x,
                 AP4_UI16 white_point_y,
                 AP4_UI32 max_display_mastering_luminance,
                 AP4_UI32 min_display_mastering_luminance);

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI16* GetDisplayPrimariesX()           { return m_DisplayPrimariesX; }
    AP4_UI16* GetDisplayPrimariesY()           { return m_DisplayPrimariesY; }
    //AP4_UI16 GetDisplayPrimariesBX()           { return m_DisplayPrimariesBX; }
    //AP4_UI16 GetDisplayPrimariesBY()           { return m_DisplayPrimariesBY; }
    //AP4_UI16 GetDisplayPrimariesRX()           { return m_DisplayPrimariesRX; }
    //AP4_UI16 GetDisplayPrimariesRY()           { return m_DisplayPrimariesRY; }
    AP4_UI16 GetWhitePointX()                  { return m_WhitePointX; }
    AP4_UI16 GetWhitePointY()                  { return m_WhitePointY; }
    AP4_UI32 GetMaxDisplayMasteringLuminance() { return m_MaxDisplayMasteringLuminance; }
    AP4_UI32 GetMinDisplayMasteringLuminance() { return m_MinDisplayMasteringLuminance; }

private:

    // members
    AP4_UI16                  m_DisplayPrimariesX[3];
    AP4_UI16                  m_DisplayPrimariesY[3];
    /*AP4_UI16                  m_DisplayPrimariesBX;
    AP4_UI16                  m_DisplayPrimariesBY;
    AP4_UI16                  m_DisplayPrimariesRX;
    AP4_UI16                  m_DisplayPrimariesRY;*/
    AP4_UI16                  m_WhitePointX;
    AP4_UI16                  m_WhitePointY;
    AP4_UI32                  m_MaxDisplayMasteringLuminance;
    AP4_UI32                  m_MinDisplayMasteringLuminance;

};

#endif // _AP4_HVCC_ATOM_H_
