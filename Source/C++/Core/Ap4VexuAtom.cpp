/*****************************************************************
|
|    AP4 - VEXU Atoms (Video Extended Usage for Stereo Video)
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

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4VexuAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"
#include "Ap4Types.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_VexuAtom)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_EyesAtom)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_StriAtom)
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_HeroAtom)

/*----------------------------------------------------------------------
|   AP4_VexuAtom::AP4_VexuAtom - Default constructor
+---------------------------------------------------------------------*/
AP4_VexuAtom::AP4_VexuAtom(AP4_UI08 hero_eye) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_VEXU),
    m_HeroEye(hero_eye)
{
    // Create default EYES atom
    m_EyesAtom = new AP4_EyesAtom(hero_eye);
    AddChild(m_EyesAtom);
}

/*----------------------------------------------------------------------
|   AP4_VexuAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result AP4_VexuAtom::WriteFields(AP4_ByteStream& stream) 
{
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|   AP4_EyesAtom::AP4_EyesAtom - Default constructor
+---------------------------------------------------------------------*/
AP4_EyesAtom::AP4_EyesAtom(AP4_UI08 hero_eye) :
    AP4_ContainerAtom(AP4_ATOM_TYPE_EYES)
{
    // Create default STRI and HERO atoms
    m_StriAtom = new AP4_StriAtom();
    m_HeroAtom = new AP4_HeroAtom(hero_eye);
    AddChild(m_StriAtom);
    AddChild(m_HeroAtom);
}

/*----------------------------------------------------------------------
|   AP4_EyesAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result AP4_EyesAtom::WriteFields(AP4_ByteStream& stream)
{
    return m_Children.Apply(AP4_AtomListWriter(stream));
}

/*----------------------------------------------------------------------
|   AP4_StriAtom::Create
+---------------------------------------------------------------------*/
AP4_StriAtom*
AP4_StriAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    // read the raw bytes in a buffer
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;//870
    if (version != 0) return NULL;
    if (size - AP4_FULL_ATOM_HEADER_SIZE < 1) return NULL;
    return new AP4_StriAtom(size, stream);
}

/*----------------------------------------------------------------------
|   AP4_StriAtom::AP4_StriAtom - Default constructor
+---------------------------------------------------------------------*/
AP4_StriAtom::AP4_StriAtom() :
    AP4_Atom(AP4_ATOM_TYPE_STRI, AP4_FULL_ATOM_HEADER_SIZE + 1, 0, 0),
    m_Reserved(0),
    m_EyeViewsReversed(0),
    m_HasAdditionalViews(0),
    m_HasRightEyeView(1),
    m_HasLeftEyeView(1)
{
}

/*----------------------------------------------------------------------
|   AP4_StriAtom::AP4_StriAtom - Constructor from payload
+---------------------------------------------------------------------*/
AP4_StriAtom::AP4_StriAtom(AP4_UI32 size, AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_STRI, size, 0, 0),
    m_Reserved(0),
    m_EyeViewsReversed(0),
    m_HasAdditionalViews(0),
    m_HasRightEyeView(0),
    m_HasLeftEyeView(0)
{
    // Parse the fields: reserved(4) + eye_views_reversed(1) + has_additional_views(1) + has_right_eye_view(1) + has_left_eye_view(1)
    AP4_UI08 fields_byte;
    stream.ReadUI08(fields_byte);
    m_Reserved = (fields_byte >> 4) & 0x0F;
    m_EyeViewsReversed = (fields_byte >> 3) & 0x01;
    m_HasAdditionalViews = (fields_byte >> 2) & 0x01;
    m_HasRightEyeView = (fields_byte >> 1) & 0x01;
    m_HasLeftEyeView = fields_byte & 0x01;
}

/*----------------------------------------------------------------------
|   AP4_StriAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_StriAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_BitWriter bits(1);
    bits.Write(m_Reserved, 4);
    bits.Write(m_EyeViewsReversed, 1);
    bits.Write(m_HasAdditionalViews, 1);
    bits.Write(m_HasRightEyeView, 1);
    bits.Write(m_HasLeftEyeView, 1);
    stream.Write(bits.GetData(), 1); // 854
    return AP4_SUCCESS;
}

AP4_Result AP4_StriAtom::InspectFields(AP4_AtomInspector& inspector) {
    inspector.AddField("reserved", m_Reserved);
    inspector.AddField("eye_views_reversed", m_EyeViewsReversed);
    inspector.AddField("has_additional_views", m_HasAdditionalViews);
    inspector.AddField("has_right_eye_view", m_HasRightEyeView);
    inspector.AddField("has_left_eye_view", m_HasLeftEyeView);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_HeroAtom::Create
+---------------------------------------------------------------------*/
AP4_HeroAtom*
AP4_HeroAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{   
    // read the raw bytes in a buffer
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    // payload size
    unsigned int payload_size = size - AP4_FULL_ATOM_HEADER_SIZE;
    if (payload_size < 1) return NULL;
    return new AP4_HeroAtom(size, stream);
}

/*----------------------------------------------------------------------
|   AP4_HeroAtom::AP4_HeroAtom - Default constructor
+---------------------------------------------------------------------*/
AP4_HeroAtom::AP4_HeroAtom() :
    AP4_Atom(AP4_ATOM_TYPE_HERO, AP4_FULL_ATOM_HEADER_SIZE + 1, 0, 0),
    m_HeroEyeIndicator(0) // Default to none
{
}

/*----------------------------------------------------------------------
|   AP4_HeroAtom::AP4_HeroAtom - Constructor with parameters
+---------------------------------------------------------------------*/
AP4_HeroAtom::AP4_HeroAtom(AP4_UI08 hero_eye_indicator) :
    AP4_Atom(AP4_ATOM_TYPE_HERO, AP4_FULL_ATOM_HEADER_SIZE + 1, 0, 0),
    m_HeroEyeIndicator(hero_eye_indicator)
{
}

/*----------------------------------------------------------------------
|   AP4_HeroAtom::AP4_HeroAtom - Constructor from payload
+---------------------------------------------------------------------*/
AP4_HeroAtom::AP4_HeroAtom(AP4_UI32 size, AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_HERO, size, 0, 0),
    m_HeroEyeIndicator(0) // Default to none
{
    stream.ReadUI08(m_HeroEyeIndicator);
}

/*----------------------------------------------------------------------
|   AP4_HeroAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_HeroAtom::WriteFields(AP4_ByteStream& stream)
{
    return stream.WriteUI08(m_HeroEyeIndicator); // 867
}