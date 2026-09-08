/*****************************************************************
|
|    AP4 - preselection
|
|    Copyright 2002-2016 Axiomatic Systems, LLC
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
#include "Ap4Utils.h"
#include "Ap4Types.h"
#include "Ap4Preselection.h"
#include <cstdlib>
#include <memory>


/*----------------------------------------------------------------------
|   AP4_Preselection
+---------------------------------------------------------------------*/
AP4_Preselection::AP4_Preselection(std::unique_ptr<InputConfig> input_config)
{
    m_Grpl = new AP4_ContainerAtom(AP4_ATOM_TYPE_GRPL);

    if (input_config) {
        ConfigSection* section = NULL;
        while (input_config->sections.PopHead(section) == AP4_SUCCESS) {
            m_Config.Add(section);
        }

        normalizeConfigSections();
    }
}

/*----------------------------------------------------------------------
|   AP4_Preselection
+---------------------------------------------------------------------*/
AP4_Preselection::AP4_Preselection(const char *config_file)
    : AP4_Preselection(parseConfigFileToInput(config_file))
{
}

/*----------------------------------------------------------------------
|   parseTrim
+---------------------------------------------------------------------*/
void parseTrim(char *str)
{
    char *start = str;
    while (*start == ' ' || *start == '\t')
        start++;

    char *end = start + strlen(start);
    while (end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n'))
    {
        *--end = '\0';
    }

    if (start != str)
        memmove(str, start, end - start + 1);
}

/*----------------------------------------------------------------------
|   splitString
+---------------------------------------------------------------------*/
void splitString(const AP4_String &input, char delimiter, AP4_List<AP4_String> &output)
{
    const char *str = input.GetChars();
    if (!str)
        return;

    const char *start = str;
    const char *p = str;

    while (*p)
    {
        if (*p == delimiter)
        {
            if (p > start)
            {
                AP4_String *token = new AP4_String(start, AP4_Size(p - start));
                output.Add(token);
            }
            start = p + 1;
        }
        ++p;
    }

    if (p > start)
    {
        AP4_String *token = new AP4_String(start, AP4_Size(p - start));
        output.Add(token);
    }
}

/*----------------------------------------------------------------------
|   getFileName
+---------------------------------------------------------------------*/
AP4_String
getFileName(const AP4_String path)
{
    const char *str = path.GetChars();
    if (!str)
        return AP4_String();

    const char *p = str + strlen(str);
    while (p > str && *p != '/' && *p != '\\')
        --p;

    if (*p == '/' || *p == '\\')
        ++p;

    return AP4_String(p);
}

/*----------------------------------------------------------------------
|   AP4_Preselection::parse_config
+---------------------------------------------------------------------*/
AP4_Result
AP4_Preselection::parsePreselectionSection(ConfigSection *current_section, const char *key, const char *value)
{
    AP4_String key_str(key);
    AP4_String value_str(value);

    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0')
    {
        return AP4_FAILURE;
    }

    if (key_str == "skip")
    {
        current_section->skip = static_cast<AP4_UI08>(std::atoi(value));
    }
    else if (key_str == "presentation_index")
    {
        current_section->presentation_index = static_cast<AP4_UI32>(std::atoi(value));
    }
    else if (key_str == "track")
    {
        current_section->track = getFileName(value);
    }
    else if (key_str == "group_id")
    {
        current_section->group_id = static_cast<AP4_UI32>(std::atoi(value));
    }
    else if (key_str == "selection_priority")
    {
        current_section->selection_priority = static_cast<AP4_UI08>(std::atoi(value));
    }
    else if (key_str == "dialog_gain")
    {
        // The value 63.5dB indicates +∞ dB, corresponding to an isolated dialogue, dialogue only
        // The value -64dB indicates -∞ dB, corresponding to completely removed dialogue
        // In configuration file, -128 and 128 are reserved special values that represent -∞ and +∞ gain

        double gain = std::atof(value);
        if (gain > 63.5 || gain < -64)
        {
            fprintf(stderr, "ERROR: dialog_gain must be in the range -64dB to 63.5dB, ignored this value %s\n", value);
        }
        else
        {
            current_section->dialog_gain = static_cast<AP4_SI08>(gain * 2);
            current_section->dialog_gain_exist = 1;
        }
    }
    else if (key_str == "label")
    {
        splitString(AP4_String(value), ',', current_section->label);
    }
    else if (key_str == "group_label")
    {
        splitString(AP4_String(value), ',', current_section->group_label);
    }
    else if (key_str == "extended_language")
    {
        current_section->extended_language = AP4_String(value);
    }
    else if (key_str == "preselection_tag")
    {
        current_section->preselection_tag = AP4_String(value);
    }
    else if (key_str == "interleaving_tag")
    {
        current_section->interleaving_tag = AP4_String(value);
    }
    else if (key_str == "kind")
    {
        splitString(AP4_String(value), ',', current_section->kind);
    }
    else if (key_str == "audio_rendering_indication")
    {
        fprintf(stderr, "Warning: audio_rendering_indication is deprecated, will use the inforamtion in DSI instead.\n");
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Preselection::parseConfigFileToInput
+---------------------------------------------------------------------*/
std::unique_ptr<AP4_Preselection::InputConfig>
AP4_Preselection::parseConfigFileToInput(const char* config_file)
{
    if (!config_file) {
        fprintf(stderr, "Error: cannot parse preselection config file (null)\n");
        return nullptr;
    }

    FILE *fp = fopen(config_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot parse preselection config file %s\n", config_file);
        return nullptr;
    }

    auto input_config = std::make_unique<InputConfig>();
    char line[1024];
    ConfigSection *current_section = new ConfigSection();

    while (fgets(line, sizeof(line), fp))
    {
        parseTrim(line);

        if (line[0] == '\0' || line[0] == ';' || line[0] == '#')
            continue;

        // [section]
        if (line[0] == '[')
        {
            char *end = strchr(line, ']');
            if (end)
            {
                *end = '\0';
                parseTrim(line + 1);

                if (current_section->name.GetLength() > 0)
                {
                    input_config->sections.Add(current_section);
                    current_section = new ConfigSection();
                }

                current_section->name = AP4_String(line + 1);
            }
            continue;
        }

        // key=value
        char *equal = strchr(line, '=');
        if (equal)
        {
            *equal = '\0';
            char *key = line;
            char *value = equal + 1;
            parseTrim(key);
            parseTrim(value);

            char *comment = strpbrk(value, ";#");
            if (comment)
                *comment = '\0';
            parseTrim(value);

            if (parsePreselectionSection(current_section, key, value) == AP4_FAILURE)
            {
                fprintf(stderr, "Warning: invalid line in config file: %s=%s\n", key, value);
            }
        }
    }

    input_config->sections.Add(current_section);
    fclose(fp);

    return input_config;
}

/*----------------------------------------------------------------------
|   AP4_Preselection::normalizeConfigSections
+---------------------------------------------------------------------*/
void
AP4_Preselection::normalizeConfigSections()
{
    // check group_id in all sections
    AP4_UI32 with_id = 0;
    AP4_UI32 without_id = 0;
    for (AP4_List<ConfigSection>::Item *item = m_Config.FirstItem(); item; item = item->GetNext())
    {
        ConfigSection *section = item->GetData();

        if (section->group_id != 0)
            with_id++;
        else
            without_id++;
    }

    if (with_id > 0 && without_id == 0)
    {
        for (AP4_List<ConfigSection>::Item *item = m_Config.FirstItem(); item; item = item->GetNext())
        {
            ConfigSection *section = item->GetData();

            if (section->group_id > m_GroupId)
                m_GroupId = section->group_id;
        }
        m_GroupId += 1; // mark the next group_id
    }
    else
    {
        if (with_id > 0 && without_id > 0)
        {
            fprintf(stderr, "Error: group_id must be set in all or none sections, will use automatical group_id instead.\n");
        }

        AP4_UI32 id = AP4_PRES_DEFAULT_GROUP_ID;

        // all sections do not have group_id, will add automatically
        for (AP4_List<ConfigSection>::Item *item = m_Config.FirstItem(); item; item = item->GetNext())
        {
            ConfigSection *section = item->GetData();

            section->group_id = id++;
        }
        m_GroupId = id;
    }
}

/*----------------------------------------------------------------------
|   AP4_Preselection::verbosePrintf
+---------------------------------------------------------------------*/
void AP4_Preselection::verbosePrintf(AP4_UI32 track_id, AP4_Ac4Frame *reference_frame)
{
    if (!reference_frame)
        return;

    fprintf(stderr, "Info: AC-4 track %u: n_pres = %u\n", track_id, reference_frame->m_Info.m_Ac4Dsi.d.v1.n_presentations);
    for (unsigned int presentation = 0; presentation < reference_frame->m_Info.m_Ac4Dsi.d.v1.n_presentations; presentation++)
    {
        fprintf(stderr, "Info: AC-4 pres[%u]: id = %u:%u, pres_v1 = 0x%X, pres_ch_mode = %u, hp = %u, ch_mask = 0x%X n_sub = %u\n",
                presentation,
                reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.b_presentation_id,
                reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.presentation_id,
                reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.presentation_config_v1,
                reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.dsi_presentation_ch_mode,
                reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.b_pre_virtualized,
                reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.presentation_channel_mask_v1,
                reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.n_substream_groups);
        for (unsigned int substream = 0; substream < reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.n_substream_groups; substream++)
        {
            fprintf(stderr, "Info: AC-4 pres[%u] sub[%u]: lang = %u:%u:%s, cont = %u:%u\n",
                    presentation,
                    substream,
                    reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.substream_groups[substream].d.v1.b_language_indicator,
                    reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.substream_groups[substream].d.v1.n_language_tag_bytes,
                    reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.substream_groups[substream].d.v1.language_tag_bytes,
                    reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.substream_groups[substream].d.v1.b_content_type,
                    reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[presentation].d.v1.substream_groups[substream].d.v1.content_classifier);
        }
    }
}

/*----------------------------------------------------------------------
|   AP4_Preselection::getSection
+---------------------------------------------------------------------*/
AP4_Preselection::ConfigSection *
AP4_Preselection::getSection(const char *input_name, AP4_UI32 presentation_index)
{
    AP4_String track = getFileName(AP4_String(input_name));
    for (AP4_List<ConfigSection>::Item *item = m_Config.FirstItem(); item; item = item->GetNext())
    {
        ConfigSection *section = item->GetData();
        if (section->track == track && section->presentation_index == presentation_index)
            return section;
    }
    return NULL;
}

AP4_SI16 computeDialogGainFromCode(AP4_UI32 code) {
    if (code == 0)  return -128;      // -inf dB
    if (code == 1)  return -24;       // -12 dB
    if (code >= 2 && code <= 13) return code - 14; // 2..13 => -12..-1.5 dB
    if (code >= 14 && code <= 61) return code - 13; // 14..61 => -1..+24.5 dB
    if (code == 62) return 60;        // +60 dB
    if (code == 63) return 127;       // +inf dB
    return 0; // should not happen
}

/*----------------------------------------------------------------------
|   AP4_Preselection::applyConfig
+---------------------------------------------------------------------*/
AP4_Result
AP4_Preselection::applyConfig(AP4_PrslAtom *prsl, AP4_Dac4Atom::Ac4Dsi::PresentationV1 *pres, AP4_Preselection::ConfigSection *section)
{
    if (!prsl || !pres || section == NULL)
        return AP4_ERROR_INVALID_PARAMETERS;

    // Apply config from INI file
    if (section->selection_priority > 0)
    {
        prsl->SetSelectionPriority(section->selection_priority);
    }
    AP4_ContainerAtom *udta = new AP4_ContainerAtom(AP4_ATOM_TYPE_UDTA);
    AP4_SI16 gain_from_es = 0;
    if (pres->d.v1.b_dei_dialog_gain_code_present) {
        gain_from_es = computeDialogGainFromCode(pres->d.v1.dei_dialog_gain_code);
    }
    if (!section->dialog_gain_exist) {
        section->dialog_gain = gain_from_es;
    } else {
        if (section->dialog_gain != gain_from_es) {
            fprintf(stderr, "ERROR: for presentation %u, dialog_gain in config file is %f dB, but the value from DSI is %f dB, please check your config file.\n", section->presentation_index, section->dialog_gain / 2.0, gain_from_es / 2.0);
            exit(1);
        }
    }
    AP4_DiapAtom *diap = new AP4_DiapAtom(section->dialog_gain);
    udta->AddChild(diap);
    prsl->AddChild(udta);
    if (section->group_label.ItemCount() > 0)
    {
        AP4_List<AP4_String>::Item *label_item = section->group_label.FirstItem();
        while (label_item)
        {
            AP4_List<AP4_String> split;
            splitString(label_item->GetData()->GetChars(), '|', split);
            if (split.ItemCount() == 2)
            {
                AP4_LablAtom *labl = new AP4_LablAtom(true, 0, split.FirstItem()->GetData()->GetChars(), split.LastItem()->GetData()->GetChars());
                prsl->AddChild(labl);
            }
            else if (split.ItemCount() == 3)
            {
                AP4_UI16 id = static_cast<AP4_UI16>(std::atoi(split.LastItem()->GetData()->GetChars()));
                AP4_String *label_lang, *label_text;
                split.Get(0, label_lang);
                split.Get(1, label_text);
                AP4_LablAtom *labl = new AP4_LablAtom(true, id, label_lang->GetChars(), label_text->GetChars());
                prsl->AddChild(labl);
            }
            else
            {
                fprintf(stderr, "ERROR: group_label must be in the format language|value[|id]\n");
            }
            label_item = label_item->GetNext();
        }
    }
    if (section->label.ItemCount() > 0)
    {
        AP4_List<AP4_String>::Item *label_item = section->label.FirstItem();
        while (label_item)
        {
            AP4_List<AP4_String> split;
            splitString(label_item->GetData()->GetChars(), '|', split);
            if (split.ItemCount() == 2)
            {
                AP4_LablAtom *labl = new AP4_LablAtom(false, 0, split.FirstItem()->GetData()->GetChars(), split.LastItem()->GetData()->GetChars());
                prsl->AddChild(labl);
            }
            else if (split.ItemCount() == 3)
            {
                AP4_UI16 id = static_cast<AP4_UI16>(std::atoi(split.LastItem()->GetData()->GetChars()));
                AP4_String *label_lang, *label_text;
                split.Get(0, label_lang);
                split.Get(1, label_text);
                AP4_LablAtom *labl = new AP4_LablAtom(false, id, label_lang->GetChars(), label_text->GetChars());
                prsl->AddChild(labl);
            }
            else
            {
                fprintf(stderr, "ERROR: label must be in the format language|label[|id]\n");
            }
            label_item = label_item->GetNext();
        }
    }
    if (section->extended_language.GetLength() > 0)
    {
        AP4_ElngAtom *elng = new AP4_ElngAtom(section->extended_language.GetChars());
        prsl->AddChild(elng);
    }
    if (section->preselection_tag.GetLength() > 0)
    {
        prsl->SetPreselectionTag(section->preselection_tag.GetChars());
    }
    if (section->interleaving_tag.GetLength() > 0)
    {
        prsl->SetInterleavingTag(section->interleaving_tag.GetChars());
    }
    if (section->kind.ItemCount() > 0)
    {
        AP4_List<AP4_String>::Item *kind_item = section->kind.FirstItem();

        while (kind_item)
        {
            AP4_List<AP4_String> split;
            splitString(kind_item->GetData()->GetChars(), '|', split);
            if (split.ItemCount() == 1)
            {
                // use default scheme URI
                AP4_KindAtom *kind_atom = new AP4_KindAtom(AP4_PRES_KIND_SCHEME_URI_DASH_URN, split.LastItem()->GetData()->GetChars());
                prsl->AddChild(kind_atom);
            }
            else if (split.ItemCount() == 2)
            {
                if (*(split.FirstItem()->GetData()) == "NULL")
                {
                    AP4_KindAtom *kind_atom = new AP4_KindAtom(split.LastItem()->GetData()->GetChars(), NULL);
                    prsl->AddChild(kind_atom);
                }
                else
                {
                    AP4_KindAtom *kind_atom = new AP4_KindAtom(split.LastItem()->GetData()->GetChars(), split.FirstItem()->GetData()->GetChars());
                    prsl->AddChild(kind_atom);
                }
            }
            else
            {
                fprintf(stderr, "ERROR: kind must be in the format value[|scheme_uri]\n");
            }
            kind_item = kind_item->GetNext();
        }
    }
    if (section->audio_rendering_indication_exist)
    {
        AP4_ArdiAtom *ardi = new AP4_ArdiAtom(section->audio_rendering_indication);
        prsl->AddChild(ardi);
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Preselection::applyDSI
+---------------------------------------------------------------------*/
AP4_Result
AP4_Preselection::applyDSI(AP4_PrslAtom *prsl, AP4_Dac4Atom::Ac4Dsi::PresentationV1 *pres)
{
    // Extract language for ELNG atom from DSI, if not already present
    if (!prsl->FindChild("elng"))
    {
        // Second substream for conf 0 or 3, first otherwise
        unsigned int substream = 0;
        if (pres->d.v1.n_substream_groups > 1)
        {
            switch (pres->d.v1.presentation_config_v1)
            {
            case 0:
            case 3:
                substream = 1;
                break;
            }
        }
        if (pres->d.v1.substream_groups[substream].d.v1.b_language_indicator)
        {
            char extended_language[64] = "";
            strncat(extended_language,
                    (const char *)pres->d.v1.substream_groups[substream].d.v1.language_tag_bytes,
                    pres->d.v1.substream_groups[substream].d.v1.n_language_tag_bytes);
            AP4_ElngAtom *elng = new AP4_ElngAtom(extended_language);
            prsl->AddChild(elng);
        }
    }

    // Determine ARDI atom from DSI, if not already present
    if (!prsl->FindChild("ardi"))
    {
        AP4_UI08 audio_rendering_indication = 0; // No preference

        if (!pres->d.v1.b_presentation_channel_coded) // Not channels, i.e., dynamic objects / ambisonics?
        {
            audio_rendering_indication = 3; // Spatial objects
        }
        else
        {
            if (pres->d.v1.b_pre_virtualized) // Pre-virtualized Headphone Mix?
            {
                audio_rendering_indication = 4; // Headphones
            }
            else
            {
                if (pres->d.v1.dsi_presentation_ch_mode <= 2) // Check channel mode
                {
                    audio_rendering_indication = 1; // Mono or Stereo
                }
                else if (pres->d.v1.dsi_presentation_ch_mode <= 8)
                {
                    audio_rendering_indication = 2; // 3.0 through 7.1
                }
                else if (pres->d.v1.dsi_presentation_ch_mode <= 15)
                {
                    audio_rendering_indication = 3; // 5.0.2 through 9.1.4 and 22.2
                }
                else
                {
                    if (pres->d.v1.presentation_channel_mask_v1 & 0x0EEB0) // Check channel mask
                    {
                        audio_rendering_indication = 3; // Has verticals
                    }
                    else if (pres->d.v1.presentation_channel_mask_v1 & 0x0000C)
                    {
                        audio_rendering_indication = 2; // Has surrounds
                    }
                    else if (pres->d.v1.presentation_channel_mask_v1 & 0x30003)
                    {
                        audio_rendering_indication = 1; // Has fronts
                    }
                }
            }
        }
        AP4_ArdiAtom *ardi = new AP4_ArdiAtom(audio_rendering_indication);
        prsl->AddChild(ardi);
    }

    // Determine KIND atom from DSI, if not already present
    if (!prsl->FindChild("kind"))
    {
        // Select associated audio: 2nd substream for conf 2, 3rd for 3 or 4, search for 5, use first otherwise
        unsigned int substream = 0;
        const char *dash_role_select = AP4_PRES_KIND_VALUE_DASH_MAIN; // default
        if (pres->d.v1.n_substream_groups > 1)
        {
            switch (pres->d.v1.presentation_config_v1)
            {
            case 2:
                substream = 1;
                break;
            case 3:
            case 4:
                substream = 2;
                break;
            case 5:
                for (int i = 0; i < pres->d.v1.n_substream_groups; i++)
                {
                    int done = false;
                    if (pres->d.v1.substream_groups[i].d.v1.b_content_type)
                    {
                        AP4_UI08 content_classifier = pres->d.v1.substream_groups[i].d.v1.content_classifier;
                        switch (content_classifier)
                        {
                        case 2:
                        case 3:
                        case 5:
                        case 6:
                        case 7:
                            substream = i;
                            done = true;
                            break;
                        }
                    }
                    if (done)
                        break;
                }
                break;
            }
        }
        char lang_tag[64] = "";
        if (pres->d.v1.substream_groups[substream].d.v1.b_language_indicator)
        {
            strncat(lang_tag, (const char *)pres->d.v1.substream_groups[substream].d.v1.language_tag_bytes, pres->d.v1.substream_groups[substream].d.v1.n_language_tag_bytes);
        }
        else
        {
            lang_tag[0] = '\0';
        }
        if (pres->d.v1.substream_groups[substream].d.v1.b_content_type)
        {
            switch (pres->d.v1.substream_groups[substream].d.v1.content_classifier)
            {
            case 2:                                                                   // visually impaired
                if ((!strncmp("qas", lang_tag, 3)) || (!strncmp("qtx", lang_tag, 3))) // Audio Description with Spoken Subtitles
                {
                    dash_role_select = AP4_PRES_KIND_VALUE_DASH_DESCRIPTION;
                }
                else
                {
                    if ((!strncmp("qad", lang_tag, 3)) || (!strncmp("qax", lang_tag, 3))) // Audio Description
                    {
                        dash_role_select = AP4_PRES_KIND_VALUE_DASH_DESCRIPTION;
                    }
                    else
                    {
                        if ((!strncmp("qei", lang_tag, 3)) || (!strncmp("qex", lang_tag, 3))) // Audio Emergency Information
                        {
                            dash_role_select = AP4_PRES_KIND_VALUE_DASH_EMERGENCY;
                        }
                    }
                }
                break;
            case 3: // hearing impaired
                dash_role_select = AP4_PRES_KIND_VALUE_DASH_ENHANCED_AUDIO_INTELLIGIBILITY;
                break;
            case 5: // commentary
                dash_role_select = AP4_PRES_KIND_VALUE_DASH_COMMENTARY;
                break;
            case 6: // emergency
                dash_role_select = AP4_PRES_KIND_VALUE_DASH_EMERGENCY;
                break;
            case 7:                                                                   // voice over
                if ((!strncmp("qss", lang_tag, 3)) || (!strncmp("qsx", lang_tag, 3))) // Spoken Subtitles
                {
                    dash_role_select = AP4_PRES_KIND_VALUE_DASH_DESCRIPTION;
                }
                else
                {
                    dash_role_select = AP4_PRES_KIND_VALUE_DASH_COMMENTARY;
                }
                break;
            }
        }
        AP4_KindAtom *kind = new AP4_KindAtom(AP4_PRES_KIND_SCHEME_URI_DASH_URN, dash_role_select);
        prsl->AddChild(kind);
    }
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Preselection::generateGrpl
+---------------------------------------------------------------------*/
AP4_Result
AP4_Preselection::generateGrpl(const char *input_name, AP4_UI32 track_id, AP4_Ac4Frame *reference_frame)
{
    // Implementation to generate 'grpl' box for ac4 track
    if (!reference_frame)
        return AP4_ERROR_INVALID_PARAMETERS;

    // verbosePrintf(track_id, reference_frame);

    // Apply one Preselection for each Presentation
    for (unsigned int idx = 0; idx < reference_frame->m_Info.m_Ac4Dsi.d.v1.n_presentations; idx++)
    {
        AP4_Dac4Atom::Ac4Dsi::PresentationV1 *pres = &reference_frame->m_Info.m_Ac4Dsi.d.v1.presentations[idx];

        // Generate preselection_tag from presentation_id in DSI
        char *tag = new char[50];
        if (pres->d.v1.b_presentation_id)
        {
            unsigned int pid = pres->d.v1.presentation_id;
            if (pres->d.v1.b_extended_presentation_id)
                pid = pres->d.v1.extended_presentation_id;
            if (pid >= 512)
                pid = 511;
            snprintf(tag, sizeof(tag), "%u", pid);
        }
        else
        {
            snprintf(tag, sizeof(tag), "%u", 0);
        }

        // Retrieve new or existing PRSL in global GRPL for this prelection
        const auto section = getSection(input_name, idx);
        if (section == NULL)
        {
            printf("Warning: Cannot find config section for presentation index %u in file %s, will generate default preselection.\n",
                   idx, input_name);

            // cannot find config section for this presentation
            AP4_PrslAtom *prsl = new AP4_PrslAtom(m_GroupId++, tag, false, 0, NULL);
            prsl->AddEntityID(track_id);

            // Apply DSI information to PRSL
            applyDSI(prsl, pres);

            m_Grpl->AddChild(prsl);
        }
        else
        {
            // skip this presentation if marked as skip in config
            if (section->skip) {
                continue;
            }

            AP4_PrslAtom *prsl = new AP4_PrslAtom(section->group_id, tag, false, 0, NULL);
            prsl->AddEntityID(track_id);

            // write the PRSL with config generated by DSI
            applyConfig(prsl, pres, section);

            // Apply DSI information to PRSL if not already present
            applyDSI(prsl, pres);

            m_Grpl->AddChild(prsl);
        }
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Preselection::generateGrpl
+---------------------------------------------------------------------*/
AP4_Result
AP4_Preselection::generateGrpl(const char *input_name, AP4_UI32 track_id, const char *tag)
{
    // Implementation to generate 'grpl' box for non-ac4 track
    if (!tag)
        return AP4_ERROR_INVALID_PARAMETERS;

    const auto section = getSection(input_name, 0);
    if (section == NULL)
    {
        printf("Error: cannot find the config section for %s\n", input_name);
        return AP4_ERROR_INVALID_PARAMETERS;
    }
    if (section->skip)
    {
        return AP4_SUCCESS;
    }

    AP4_PrslAtom *prsl = new AP4_PrslAtom(section->group_id, tag, false, 0, NULL);
    prsl->AddEntityID(track_id);

    applyConfig(prsl, NULL, section);

    m_Grpl->AddChild(prsl);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_Preselection::getGrpl
+---------------------------------------------------------------------*/
AP4_ContainerAtom *
AP4_Preselection::getGrpl()
{
    return m_Grpl;
}