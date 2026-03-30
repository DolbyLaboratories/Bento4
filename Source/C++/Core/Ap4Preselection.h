/*****************************************************************
|
|    AP4 - Preselection
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

#ifndef _AP4_PRESELECTION_H_
#define _AP4_PRESELECTION_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <map>
#include "Ap4Atom.h"
#include "Ap4ContainerAtom.h"
#include "Ap4KindAtom.h"
#include "Ap4LablAtom.h"
#include "Ap4ElngAtom.h"
#include "Ap4ArdiAtom.h"
#include "Ap4PrslAtom.h"
#include "Ap4DiapAtom.h"
#include "Ap4Array.h"
#include "Ap4Ac4Utils.h"
#include "Ap4String.h"
#include "Ap4Ac4Parser.h"
#include "Ap4Dac4Atom.h"

/*----------------------------------------------------------------------
|   Constants
+---------------------------------------------------------------------*/
// Kind value: Use IEC 23009-1 (DASH) "Role", see Section 5.8.5.5 and Table 34
#define AP4_PRES_KIND_SCHEME_URI_DASH_URN "urn:mpeg:dash:role:2011"
#define AP4_PRES_KIND_VALUE_DASH_MAIN "main" // default
#define AP4_PRES_KIND_VALUE_DASH_ALTERNATE "alternate"
#define AP4_PRES_KIND_VALUE_DASH_SUPPLEMENTARY "supplementary"
#define AP4_PRES_KIND_VALUE_DASH_COMMENTARY "commentary"
#define AP4_PRES_KIND_VALUE_DASH_DUB "dub"
#define AP4_PRES_KIND_VALUE_DASH_DESCRIPTION "description"
#define AP4_PRES_KIND_VALUE_DASH_CAPTION "caption"
#define AP4_PRES_KIND_VALUE_DASH_SUBTITLE "subtitle"
#define AP4_PRES_KIND_VALUE_DASH_SIGN "sign"
#define AP4_PRES_KIND_VALUE_DASH_METADATA "metadata"
#define AP4_PRES_KIND_VALUE_DASH_ENHANCED_AUDIO_INTELLIGIBILITY "enhanced-audio-intelligibility"
#define AP4_PRES_KIND_VALUE_DASH_EMERGENCY "emergency"
#define AP4_PRES_KIND_VALUE_DASH_FORCED_SUBTITLE "forced-subtitle"
#define AP4_PRES_KIND_VALUE_DASH_EASYREADER "easyreader"
#define AP4_PRES_KIND_VALUE_DASH_KARAOKE "karaoke"

#define AP4_PRES_DEFAULT_GROUP_ID 1000
/*----------------------------------------------------------------------
|   AP4_Preselection
+---------------------------------------------------------------------*/

class AP4_Preselection
{
    struct ConfigSection {
        AP4_UI08       skip = 0;
        AP4_String     name;        // config section name
        AP4_String     track;       // input track file name
        AP4_UI32       presentation_index = 0;
        AP4_UI32       group_id = 0;
        AP4_UI08       selection_priority = 0;
        AP4_String     preselection_tag;
        AP4_String     interleaving_tag;
        AP4_SI08       dialog_gain = 0;
        AP4_UI08       dialog_gain_exist = 0; // 0: not exist, 1: exist
        AP4_UI08       audio_rendering_indication = 0;
        AP4_SI16       audio_rendering_indication_exist = 0;
        AP4_List<AP4_String> label;
        AP4_List<AP4_String> group_label;
        AP4_List<AP4_String> kind;
        AP4_List<AP4_String> kind_urn;
        AP4_String     extend_language;
    };
public:
    AP4_Preselection (const char* config_file);
    AP4_ContainerAtom* getGrpl();
    AP4_Result generateGrpl(const char* input_name, AP4_UI32 track_id, AP4_Ac4Frame* reference_frame);
    AP4_Result generateGrpl(const char* input_name, AP4_UI32 track_id, const char *tag);

private:
    AP4_Result parseConfig(const char* config_file);
    AP4_Result parsePreselectionSection(ConfigSection *current_section, const char *key, const char *value);
    void verbosePrintf(AP4_UI32 track_id, AP4_Ac4Frame *reference_frame);
    AP4_Result applyConfig(AP4_PrslAtom* prsl, AP4_Dac4Atom::Ac4Dsi::PresentationV1 *pres, ConfigSection *section);
    AP4_Result applyDSI(AP4_PrslAtom* prsl, AP4_Dac4Atom::Ac4Dsi::PresentationV1 *pres);
    ConfigSection* getSection(const char* input_name, AP4_UI32 presentation_index);

    // member
    AP4_List<ConfigSection> m_Config;
    AP4_List<ConfigSection> m_UdtaLabl;
    AP4_ContainerAtom *m_Grpl = NULL;
    AP4_ContainerAtom *m_Udta = NULL;
    AP4_UI32 m_GroupId = AP4_PRES_DEFAULT_GROUP_ID;
};

#endif // _AP4_PRESELECTION_H_
