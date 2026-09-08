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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4AeppAtom.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_AeppAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_AEPP_FLAG_AZIMUTH_MASK       = 0x00000001;
const AP4_UI32 AP4_AEPP_FLAG_ELEVATION_MASK     = 0x00000002;
const AP4_UI32 AP4_AEPP_FLAG_DISTANCE_MASK      = 0x00000004;

/*----------------------------------------------------------------------
|   AP4_AeppAtom::Create
+---------------------------------------------------------------------*/
AP4_AeppAtom*
AP4_AeppAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_AeppAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_AeppAtom::AP4_AeppAtom
+---------------------------------------------------------------------*/
AP4_AeppAtom::AP4_AeppAtom(AP4_Flags azimuth,
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
                           AP4_UI32 default_distance) :
    AP4_Atom(AP4_ATOM_TYPE_AEPP, AP4_FULL_ATOM_HEADER_SIZE, 0, 0)
{
    if (azimuth) {
        m_MinAzimuth = min_azimuth;
        m_MaxAzimuth = max_azimuth;
        m_DefaultAzimuth = default_azimuth;
        m_Size32 += 6;
        m_Flags |= AP4_AEPP_FLAG_AZIMUTH_MASK;
    } else {
        m_Flags &= ~AP4_AEPP_FLAG_AZIMUTH_MASK;
    }

    if (elevation) {
        m_MinElevation = min_elevation;
        m_MaxElevation = max_elevation;
        m_DefaultElevation = default_elevation;
        m_Size32 += 3;
        m_Flags |= AP4_AEPP_FLAG_ELEVATION_MASK;
    } else {
        m_Flags &= ~AP4_AEPP_FLAG_ELEVATION_MASK;
    }

    if (distance) {
        m_MinDistance = min_distance;
        m_MaxDistance = max_distance;
        m_DefaultDistance = default_distance;
        m_Size32 += 12;
        m_Flags |= AP4_AEPP_FLAG_DISTANCE_MASK;
    } else {
        m_Flags &= ~AP4_AEPP_FLAG_DISTANCE_MASK;
    }
}

/*----------------------------------------------------------------------
|   AP4_AeppAtom::AP4_AeppAtom
+---------------------------------------------------------------------*/
AP4_AeppAtom::AP4_AeppAtom(AP4_UI32        size,
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_AEPP, size, version, flags)
{
    if (flags & AP4_AEPP_FLAG_AZIMUTH_MASK) {
        AP4_UI16 tmp16;

        stream.ReadUI16(tmp16);
        m_MinAzimuth = (AP4_SI16)tmp16;

        stream.ReadUI16(tmp16);
        m_MaxAzimuth = (AP4_SI16)tmp16;

        stream.ReadUI16(tmp16);
        m_DefaultAzimuth = (AP4_SI16)tmp16;
    }

    if (flags & AP4_AEPP_FLAG_ELEVATION_MASK) {
        AP4_UI08 tmp08;

        stream.ReadUI08(tmp08);
        m_MinElevation = (AP4_SI08)tmp08;

        stream.ReadUI08(tmp08);
        m_MaxElevation = (AP4_SI08)tmp08;

        stream.ReadUI08(tmp08);
        m_DefaultElevation = (AP4_SI08)tmp08;
    }

    if (flags & AP4_AEPP_FLAG_DISTANCE_MASK) {
        stream.ReadUI32(m_MinDistance);
        stream.ReadUI32(m_MaxDistance);
        stream.ReadUI32(m_DefaultDistance);
    }
}

/*----------------------------------------------------------------------
|   AP4_AeppAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AeppAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    AP4_UI32   flags = this->GetFlags();
    
    if (flags & AP4_AEPP_FLAG_AZIMUTH_MASK) {
        result = stream.WriteUI16((AP4_UI16)m_MinAzimuth);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI16((AP4_UI16)m_MaxAzimuth);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI16((AP4_UI16)m_DefaultAzimuth);
        if (AP4_FAILED(result)) return result;
    }

    if (flags & AP4_AEPP_FLAG_ELEVATION_MASK) {
        result = stream.WriteUI08((AP4_UI08)m_MinElevation);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI08((AP4_UI08)m_MaxElevation);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI08((AP4_UI08)m_DefaultElevation);
        if (AP4_FAILED(result)) return result;
    }

    if (flags & AP4_AEPP_FLAG_DISTANCE_MASK) {
        result = stream.WriteUI32(m_MinDistance);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_MaxDistance);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI32(m_DefaultDistance);
        if (AP4_FAILED(result)) return result;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_AeppAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_AeppAtom::InspectFields(AP4_AtomInspector& inspector)
{
    AP4_UI32 flags = this->GetFlags();

    if (flags & AP4_AEPP_FLAG_AZIMUTH_MASK) {
        inspector.AddField("min_azimuth", m_MinAzimuth);
        inspector.AddField("max_azimuth", m_MaxAzimuth);
        inspector.AddField("default_azimuth", m_DefaultAzimuth);
    }

    if (flags & AP4_AEPP_FLAG_ELEVATION_MASK) {
        inspector.AddField("min_elevation", m_MinElevation);
        inspector.AddField("max_elevation", m_MaxElevation);
        inspector.AddField("default_elevation", m_DefaultElevation);
    }

    if (flags & AP4_AEPP_FLAG_DISTANCE_MASK) {
        inspector.AddField("min_distance", m_MinDistance);
        inspector.AddField("max_distance", m_MaxDistance);
        inspector.AddField("default_distance", m_DefaultDistance);
    }

    return AP4_SUCCESS;
}
