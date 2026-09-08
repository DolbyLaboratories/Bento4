/*****************************************************************
|
|    AP4 - lhvC Atoms 
|
|    Copyright 2002-2024 Axiomatic Systems, LLC
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

#ifndef _AP4_LHVC_ATOM_H_
#define _AP4_LHVC_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4Array.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class AP4_HevcFrameParser;

/*----------------------------------------------------------------------
|   AP4_LhvcAtom
+---------------------------------------------------------------------*/
class AP4_LhvcAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_LhvcAtom, AP4_Atom)

    class Sequence {
    public:
        AP4_UI08                  m_ArrayCompleteness;
        AP4_UI08                  m_LayerId;
        AP4_UI08                  m_NaluType;
        AP4_Array<AP4_DataBuffer> m_Nalus;
    };
    
    // class methods
    static AP4_LhvcAtom* Create(AP4_Size size, AP4_ByteStream& stream);
    static const char*   GetProfileName(AP4_UI08 profile_space, AP4_UI08 profile);
    
    // constructors
    AP4_LhvcAtom();
    
    // Enhanced constructor using HEVC frame parser for ES-based initialization
    AP4_LhvcAtom(AP4_HevcFrameParser& parser, AP4_UI08 parameters_completeness);

    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    const AP4_Array<Sequence>& GetSequences()      const { return m_Sequences; }
    const AP4_DataBuffer& GetRawBytes()            const { return m_RawBytes; }

private:
    // methods
    AP4_LhvcAtom(AP4_UI32 size, const AP4_UI08* payload);
    void UpdateRawBytes();
    
    // members
    AP4_UI08                  m_ConfigurationVersion;
    AP4_UI08                  m_Reserved1;                // 4 bits
    AP4_UI16                  m_min_spatial_segmentation; // 12 bits
    AP4_UI08                  m_Reserved2;                 // 6 bits
    AP4_UI08                  m_parallelismType;           // 2 bits
    AP4_UI08                  m_Reserved3;                 // 2 bits   
    AP4_UI08                  m_numTemporalLayers;         // 3 bits  
    AP4_UI08                  m_temporalIdNested;          // 1 bit
    AP4_UI08                  m_lengthSizeMinusOne;        // 2 bits
    AP4_Array<Sequence>       m_Sequences;
    AP4_DataBuffer            m_RawBytes;
};

#endif // _AP4_LHVC_ATOM_H_