/*****************************************************************
|
|    AP4 - Track Kind Atoms
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
#include "Ap4KindAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_KindAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const char* default_scheme_uri = "urn:mpeg:dash:role:2011";
const char* default_value = "main";

/*----------------------------------------------------------------------
|   AP4_KindAtom::Create
+---------------------------------------------------------------------*/
AP4_KindAtom*
AP4_KindAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_KindAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_KindAtom::AP4_KindAtom
+---------------------------------------------------------------------*/
AP4_KindAtom::AP4_KindAtom(const char* scheme_uri,
                           const char* value) :
    AP4_Atom(AP4_ATOM_TYPE_KIND, AP4_FULL_ATOM_HEADER_SIZE, 0, 0)
{
    if (scheme_uri) {
        m_SchemeURI = scheme_uri;
    } else {
        m_SchemeURI = default_scheme_uri;
    }
    m_Size32 += m_SchemeURI.GetLength()+1;

    if (value) {
        m_Value = value;
    } else {
        m_Value = default_value;
    }
    m_Size32 += m_Value.GetLength()+1;
}

/*----------------------------------------------------------------------
|   AP4_KindAtom::AP4_KindAtom
+---------------------------------------------------------------------*/
AP4_KindAtom::AP4_KindAtom(AP4_UI32        size,
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_KIND, size, version, flags)
{
    stream.ReadNullTerminatedString(m_SchemeURI);
    stream.ReadNullTerminatedString(m_Value);
}

/*----------------------------------------------------------------------
|   AP4_KindAtom::SetSchemeURI
+---------------------------------------------------------------------*/
void AP4_KindAtom::SetSchemeURI(const char* scheme_uri)
{
    m_Size32 -= m_SchemeURI.GetLength()+1;
    if (scheme_uri) {
        m_SchemeURI = scheme_uri;
    } else {
        m_SchemeURI = default_scheme_uri;
    }
    m_Size32 += m_SchemeURI.GetLength()+1;
}

/*----------------------------------------------------------------------
|   AP4_KindAtom::SetValue
+---------------------------------------------------------------------*/
void AP4_KindAtom::SetValue(const char* value)
{
    m_Size32 -= m_Value.GetLength()+1;
    if (value) {
        m_Value = value;
    } else {
        m_Value = default_value;
    }
    m_Size32 += m_Value.GetLength()+1;
}

/*----------------------------------------------------------------------
|   AP4_KindAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_KindAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;

    result = stream.WriteNullTerminatedString(m_SchemeURI.GetChars());
    if (AP4_FAILED(result)) return result;
    result = stream.WriteNullTerminatedString(m_Value.GetChars());
    if (AP4_FAILED(result)) return result;
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_KindAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_KindAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("scheme_uri", m_SchemeURI.GetChars());
    inspector.AddField("value", m_Value.GetChars());

    return AP4_SUCCESS;
}
