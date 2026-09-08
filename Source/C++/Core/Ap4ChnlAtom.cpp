/*****************************************************************
|
|    AP4 - Channel Layout Atoms
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
#include "Ap4ChnlAtom.h"
#include "Ap4AtomFactory.h"
#include "Ap4Utils.h"

/*----------------------------------------------------------------------
|   dynamic cast support
+---------------------------------------------------------------------*/
AP4_DEFINE_DYNAMIC_CAST_ANCHOR(AP4_ChnlAtom)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI08 AP4_CHNL_CHANNEL_STRUCTURED_MASK = 0x01;
const AP4_UI08 AP4_CHNL_OBJECT_STRUCTURED_MASK = 0x02;

/*----------------------------------------------------------------------
|   AP4_ChnlAtom::Create
+---------------------------------------------------------------------*/
AP4_ChnlAtom*
AP4_ChnlAtom::Create(AP4_Size size, AP4_ByteStream& stream)
{
    AP4_UI08 version;
    AP4_UI32 flags;
    if (size < AP4_FULL_ATOM_HEADER_SIZE) return NULL;
    if (AP4_FAILED(AP4_Atom::ReadFullHeader(stream, version, flags))) return NULL;
    if (version > 1) return NULL;
    return new AP4_ChnlAtom(size, version, flags, stream);
}

/*----------------------------------------------------------------------
|   AP4_ChnlAtom::AP4_ChnlAtom
+---------------------------------------------------------------------*/
AP4_ChnlAtom::AP4_ChnlAtom(const AP4_Chnl* channel) :
    AP4_Atom(AP4_ATOM_TYPE_CHNL, AP4_FULL_ATOM_HEADER_SIZE, 0, 0)
{
    memcpy(&m_Channel, channel, sizeof(AP4_Chnl));
    m_Version = channel->version;
    
    if (channel->version == 0) {
        m_Size32 += 1;
        if (channel->chnl.v0.stream_structure & AP4_CHNL_CHANNEL_STRUCTURED_MASK) {
            m_Size32 += 1;
            if (channel->chnl.v0.defined_layout == 0) {
                // layout_channel_count not present, no bytes
                for (unsigned int i = 0 ; i < channel->chnl.v0.dl0.layout_channel_count ; i++) {
                    m_Size32 += 1;
                    if (channel->chnl.v0.dl0.speaker[i].speaker_position == 126) {
                        m_Size32 += 3;
                    }
                }
            } else {
                m_Size32 += 8;
            }
        }
        if (channel->chnl.v0.stream_structure & AP4_CHNL_OBJECT_STRUCTURED_MASK) {
            m_Size32 += 1;
        }
    } else {
        m_Size32 += 2;
        if (channel->chnl.v1.stream_structure & AP4_CHNL_CHANNEL_STRUCTURED_MASK) {
            m_Size32 += 1;
            if (channel->chnl.v1.defined_layout == 0) {
                m_Size32 += 1;
                for (unsigned int i = 0 ; i < channel->chnl.v1.dl0.layout_channel_count ; i++) {
                    m_Size32 += 1;
                    if (channel->chnl.v1.dl0.speaker[i].speaker_position == 126) {
                        m_Size32 += 3;
                    }
                }
            } else {
                m_Size32 += 1;
                if (channel->chnl.v1.dl.omitted_channels_present_flag) {
                    m_Size32 += 8;
                }
            }
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_ChnlAtom::AP4_ChnlAtom
+---------------------------------------------------------------------*/
AP4_ChnlAtom::AP4_ChnlAtom(AP4_UI32        size,
                           AP4_UI08        version,
                           AP4_UI32        flags,
                           AP4_ByteStream& stream) :
    AP4_Atom(AP4_ATOM_TYPE_CHNL, size, version, flags)
{
    AP4_UI08 f;
    AP4_UI32 remaining = size - GetHeaderSize();

    m_Channel.version = version;
    if (m_Channel.version == 0) {
        stream.ReadUI08(m_Channel.chnl.v0.stream_structure);
        remaining--;
        if (m_Channel.chnl.v0.stream_structure & AP4_CHNL_CHANNEL_STRUCTURED_MASK) {
            stream.ReadUI08(m_Channel.chnl.v0.defined_layout);
            remaining--;
            if (m_Channel.chnl.v0.defined_layout == 0) {
                // This count is supposed to come from a sample entry box, but that is not available here
                // Instead, count remaining bytes to determine number of channels present in this box
                //m_Channel.chnl.v0.dl0.layout_channel_count = ???;
                //for (unsigned int i = 0 ; i < m_Channel.chnl.v0.dl0.layout_channel_count ; i++) {
                unsigned int i = 0;
                if (m_Channel.chnl.v0.stream_structure & AP4_CHNL_OBJECT_STRUCTURED_MASK) {
                    remaining--; // account for 'object_count' after the list
                }
                while (remaining > 0) {
                    stream.ReadUI08(m_Channel.chnl.v0.dl0.speaker[i].speaker_position);
                    remaining--;
                    if (m_Channel.chnl.v0.dl0.speaker[i].speaker_position == 126) {
                        AP4_UI16 tmp16;
                        AP4_UI08 tmp08;
                        stream.ReadUI16(tmp16);
                        m_Channel.chnl.v0.dl0.speaker[i].azimuth = (AP4_SI16)tmp16;
                        stream.ReadUI08(tmp08);
                        m_Channel.chnl.v0.dl0.speaker[i].elevation = (AP4_SI08)tmp08;
                        remaining -= 3;
                    }
                    i++;
                }
                m_Channel.chnl.v0.dl0.layout_channel_count = i;
            } else {
                stream.ReadUI64(m_Channel.chnl.v0.dl.omitted_channels_map);
            }
        }
        if (m_Channel.chnl.v0.stream_structure & AP4_CHNL_OBJECT_STRUCTURED_MASK) {
            stream.ReadUI08(m_Channel.chnl.v0.object_count);
        }
    } else {
        stream.ReadUI08(f);
        m_Channel.chnl.v1.stream_structure = (f >> 4) & 0x0F;
        m_Channel.chnl.v1.format_ordering = f & 0x0F;
        stream.ReadUI08(m_Channel.chnl.v1.base_channel_count);
        if (m_Channel.chnl.v1.stream_structure & AP4_CHNL_CHANNEL_STRUCTURED_MASK) {
            stream.ReadUI08(m_Channel.chnl.v1.defined_layout);
            if (m_Channel.chnl.v1.defined_layout == 0) {
                stream.ReadUI08(m_Channel.chnl.v1.dl0.layout_channel_count);
                for (unsigned int i = 0 ; i < m_Channel.chnl.v1.dl0.layout_channel_count ; i++) {
                    stream.ReadUI08(m_Channel.chnl.v1.dl0.speaker[i].speaker_position);
                    if (m_Channel.chnl.v1.dl0.speaker[i].speaker_position == 126) {
                        AP4_UI16 tmp16;
                        AP4_UI08 tmp08;
                        stream.ReadUI16(tmp16);
                        m_Channel.chnl.v1.dl0.speaker[i].azimuth = (AP4_SI16)tmp16;
                        stream.ReadUI08(tmp08);
                        m_Channel.chnl.v1.dl0.speaker[i].elevation = (AP4_SI08)tmp08;
                    }
                }
            } else {
                stream.ReadUI08(f);
                m_Channel.chnl.v1.dl.channel_order_definition = (f >> 1) & 0x07;
                m_Channel.chnl.v1.dl.omitted_channels_present_flag = (f & 0x01) ? true : false;
                if (m_Channel.chnl.v1.dl.omitted_channels_present_flag) {
                    stream.ReadUI64(m_Channel.chnl.v1.dl.omitted_channels_map);
                }
            }
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_ChnlAtom::WriteFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ChnlAtom::WriteFields(AP4_ByteStream& stream)
{
    AP4_Result result;
    AP4_UI08 f;

    if (m_Channel.version == 0) {
        result = stream.WriteUI08(m_Channel.chnl.v0.stream_structure);
        if (AP4_FAILED(result)) return result;
        if (m_Channel.chnl.v0.stream_structure & AP4_CHNL_CHANNEL_STRUCTURED_MASK) {
            result = stream.WriteUI08(m_Channel.chnl.v0.defined_layout);
            if (AP4_FAILED(result)) return result;
            if (m_Channel.chnl.v0.defined_layout == 0) {
                // layout_channel_count is not written to the box
                for (unsigned int i = 0 ; i < m_Channel.chnl.v0.dl0.layout_channel_count ; i++) {
                    result = stream.WriteUI08(m_Channel.chnl.v0.dl0.speaker[i].speaker_position);
                    if (AP4_FAILED(result)) return result;
                    if (m_Channel.chnl.v0.dl0.speaker[i].speaker_position == 126) {
                        result = stream.WriteUI16((AP4_UI16)m_Channel.chnl.v0.dl0.speaker[i].azimuth);
                        if (AP4_FAILED(result)) return result;
                        result = stream.WriteUI08((AP4_UI08)(m_Channel.chnl.v0.dl0.speaker[i].elevation & 0xFF));
                        if (AP4_FAILED(result)) return result;
                    }
                }
            } else {
                result = stream.WriteUI64(m_Channel.chnl.v0.dl.omitted_channels_map);
                if (AP4_FAILED(result)) return result;
            }
        }
        if (m_Channel.chnl.v0.stream_structure & AP4_CHNL_OBJECT_STRUCTURED_MASK) {
            result = stream.WriteUI08(m_Channel.chnl.v0.object_count);
            if (AP4_FAILED(result)) return result;
        }
    } else {
        f =
            ((m_Channel.chnl.v1.stream_structure << 4) & 0xF0) |
            (m_Channel.chnl.v1.format_ordering & 0x0F);
        result = stream.WriteUI08(f);
        if (AP4_FAILED(result)) return result;
        result = stream.WriteUI08(m_Channel.chnl.v1.base_channel_count);
        if (AP4_FAILED(result)) return result;
        if (m_Channel.chnl.v1.stream_structure & AP4_CHNL_CHANNEL_STRUCTURED_MASK) {
            result = stream.WriteUI08(m_Channel.chnl.v1.defined_layout);
            if (AP4_FAILED(result)) return result;
            if (m_Channel.chnl.v1.defined_layout == 0) {
                result = stream.WriteUI08(m_Channel.chnl.v1.dl0.layout_channel_count);
                if (AP4_FAILED(result)) return result;
                for (unsigned int i = 0 ; i < m_Channel.chnl.v1.dl0.layout_channel_count ; i++) {
                    result = stream.WriteUI08(m_Channel.chnl.v1.dl0.speaker[i].speaker_position);
                    if (m_Channel.chnl.v1.dl0.speaker[i].speaker_position == 126) {
                        result = stream.WriteUI16((AP4_UI16)m_Channel.chnl.v1.dl0.speaker[i].azimuth);
                        if (AP4_FAILED(result)) return result;
                        result = stream.WriteUI08((AP4_UI08)(m_Channel.chnl.v1.dl0.speaker[i].elevation & 0xFF));
                        if (AP4_FAILED(result)) return result;
                    }
                }
            } else {
                            f =
                    ((m_Channel.chnl.v1.dl.channel_order_definition << 1) & 0x0E) |
                    (m_Channel.chnl.v1.dl.omitted_channels_present_flag ? 0x01 : 0x00);
                result = stream.WriteUI08(f);
                if (AP4_FAILED(result)) return result;
                if (m_Channel.chnl.v1.dl.omitted_channels_present_flag) {
                    result = stream.WriteUI64(m_Channel.chnl.v1.dl.omitted_channels_map);
                    if (AP4_FAILED(result)) return result;
                }
            }
        }
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_ChnlAtom::InspectFields
+---------------------------------------------------------------------*/
AP4_Result
AP4_ChnlAtom::InspectFields(AP4_AtomInspector& inspector)
{
    inspector.AddField("version", m_Channel.version);
    if (m_Channel.version == 0) {
        inspector.AddField("stream_structure", m_Channel.chnl.v0.stream_structure, AP4_AtomInspector::HINT_HEX);
        if (m_Channel.chnl.v0.stream_structure & AP4_CHNL_CHANNEL_STRUCTURED_MASK) {
            inspector.AddField("defined_layout", m_Channel.chnl.v0.defined_layout);
            if (m_Channel.chnl.v0.defined_layout == 0) {
                inspector.AddField("layout_channel_count", m_Channel.chnl.v0.dl0.layout_channel_count);
                for (unsigned int i = 0 ; i < m_Channel.chnl.v0.dl0.layout_channel_count ; i++) {
                    char field_name[64];
                    AP4_FormatString(field_name, sizeof(field_name), "[%03d].speaker_position", i + 1);
                    inspector.AddField(field_name, m_Channel.chnl.v0.dl0.speaker[i].speaker_position);
                    if (m_Channel.chnl.v0.dl0.speaker[i].speaker_position == 126) {
                        AP4_FormatString(field_name, sizeof(field_name), "[%03d].azimuth", i + 1);
                        inspector.AddField(field_name, m_Channel.chnl.v0.dl0.speaker[i].azimuth);
                        AP4_FormatString(field_name, sizeof(field_name), "[%03d].elevation", i + 1);
                        inspector.AddField(field_name, m_Channel.chnl.v0.dl0.speaker[i].elevation);
                    }
                }
            } else {
                inspector.AddField("omitted_channels_map", m_Channel.chnl.v0.dl.omitted_channels_map, AP4_AtomInspector::HINT_HEX);
            }
        }
        if (m_Channel.chnl.v0.stream_structure & AP4_CHNL_OBJECT_STRUCTURED_MASK) {
            inspector.AddField("", m_Channel.chnl.v0.object_count);
        }
    } else {
        inspector.AddField("stream_structure", m_Channel.chnl.v1.stream_structure);
        inspector.AddField("format_ordering", m_Channel.chnl.v1.format_ordering);
        inspector.AddField("base_channel_count", m_Channel.chnl.v1.base_channel_count);
        if (m_Channel.chnl.v1.stream_structure & AP4_CHNL_CHANNEL_STRUCTURED_MASK) {
            inspector.AddField("defined_layout", m_Channel.chnl.v1.defined_layout);
            if (m_Channel.chnl.v1.defined_layout == 0) {
                inspector.AddField("layout_channel_count", m_Channel.chnl.v1.dl0.layout_channel_count);
                for (unsigned int i = 0 ; i < m_Channel.chnl.v1.dl0.layout_channel_count ; i++) {
                    char field_name[64];
                    AP4_FormatString(field_name, sizeof(field_name), "[%03d].speaker_position", i + 1);
                    inspector.AddField(field_name, m_Channel.chnl.v1.dl0.speaker[i].speaker_position);
                    if (m_Channel.chnl.v1.dl0.speaker[i].speaker_position == 126) {
                        AP4_FormatString(field_name, sizeof(field_name), "[%03d].azimuth", i + 1);
                        inspector.AddField(field_name, m_Channel.chnl.v1.dl0.speaker[i].azimuth);
                        AP4_FormatString(field_name, sizeof(field_name), "[%03d].elevation", i + 1);
                        inspector.AddField(field_name, m_Channel.chnl.v1.dl0.speaker[i].elevation);
                    }
                }
            } else {
                inspector.AddField("channel_order_definition", m_Channel.chnl.v1.dl.channel_order_definition);
                inspector.AddField("omitted_channels_present_flag", m_Channel.chnl.v1.dl.omitted_channels_present_flag, AP4_AtomInspector::HINT_BOOLEAN);
                if (m_Channel.chnl.v1.dl.omitted_channels_present_flag) {
                    inspector.AddField("omitted_channels_map", m_Channel.chnl.v1.dl.omitted_channels_map, AP4_AtomInspector::HINT_HEX);
                }
            }
        }
    }

    return AP4_SUCCESS;
}
