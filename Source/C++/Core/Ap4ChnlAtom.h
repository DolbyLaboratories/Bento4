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

#ifndef _AP4_CHNL_ATOM_H_
#define _AP4_CHNL_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Atom.h"
#include "Ap4String.h"

/*----------------------------------------------------------------------
|   AP4_ChnlAtom
+---------------------------------------------------------------------*/
class AP4_ChnlAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_ChnlAtom, AP4_Atom)
    
    // inner classes
    struct AP4_Chnl {
        AP4_UI08 version;
        union {
            struct
            {
                AP4_UI08 stream_structure;
                AP4_UI08 defined_layout;
                union {
                    AP4_UI08 layout_channel_count; // this member is not present in the box data
                    struct {
                        AP4_UI08 speaker_position;
                        AP4_SI16 azimuth;
                        AP4_SI08 elevation;
                    } speaker[256];
                }dl0;
                union {
                    AP4_UI64 omitted_channels_map;
                }dl;
                AP4_UI08 object_count;
            }v0;
            struct
            {
                AP4_UI08 stream_structure;
                AP4_UI08 format_ordering;
                AP4_UI08 base_channel_count;
                AP4_UI08 defined_layout;
                union {
                    AP4_UI08 layout_channel_count;
                    struct {
                        AP4_UI08 speaker_position;
                        AP4_SI16 azimuth;
                        AP4_SI08 elevation;
                    } speaker[256];
                }dl0;
                union {
                    AP4_UI08 channel_order_definition;
                    AP4_Flags omitted_channels_present_flag;
                    AP4_UI64 omitted_channels_map;
                }dl;
            }v1;
        }chnl;
    };
    
    // class methods
    static AP4_ChnlAtom* Create(AP4_Size size, AP4_ByteStream& stream);
    
    // methods
    AP4_ChnlAtom(const AP4_Chnl* channel);
    
    // methods
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);
    
    // accessors
    const AP4_Chnl&         GetChannel() const { return m_Channel; }

    
private:
    // methods
    AP4_ChnlAtom(AP4_UI32        size,
                 AP4_UI08        version,
                 AP4_UI32        flags,
                 AP4_ByteStream& stream);

    // members
    AP4_Chnl         m_Channel;
};

#endif // _AP4_CHNL_ATOM_H_
