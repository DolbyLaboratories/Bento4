/*****************************************************************
|
|    AP4 - Elementary Stream Muliplexer
|
|    Copyright 2002-2020 Axiomatic Systems, LLC
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "Ap4.h"
#include "Ap4Ac4Utils.h"
#include "Ap4Preselection.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BANNER "MP4 Elementary Stream Multiplexer - Version 3.0\n"\
               "(Bento4 Version " AP4_VERSION_STRING ")\n"\
               "(c) 2002-20020 Axiomatic Systems, LLC"

const unsigned int AP4_MUX_DEFAULT_VIDEO_FRAME_RATE = 24;
const unsigned int AP4_MUX_READ_BUFFER_SIZE         = 65536;

/*----------------------------------------------------------------------
|   SampleOrder
+---------------------------------------------------------------------*/
struct SampleOrder {
    SampleOrder(AP4_UI32 decode_order, AP4_UI32 display_order) :
        m_DecodeOrder(decode_order),
        m_DisplayOrder(display_order) {}
    AP4_UI32 m_DecodeOrder;
    AP4_UI32 m_DisplayOrder;
};

/*----------------------------------------------------------------------
|   Parameter
+---------------------------------------------------------------------*/
struct Parameter {
    Parameter(const char* name, const char* value) :
        m_Name(name),
        m_Value(value) {}
    AP4_String m_Name;
    AP4_String m_Value;
};

struct Gapless_roll {
    Gapless_roll(double hoff, double toff) :
        pre_roll(hoff),
        post_roll(toff) {}
    double pre_roll;
    double post_roll;
};

static bool g_Verbose = false;
AP4_Preselection* g_Preselection = NULL;

/*----------------------------------------------------------------------
|   PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
PrintUsageAndExit()
{
    fprintf(stderr, 
            BANNER 
            "\n\nusage: mp4mux [options] --track [<type>:]<input>[#<params>] [--track [<type>:]<input>[#<params>] ...] <output>\n"
            "\n"
            "  <params>, when specified, are expressed as a comma-separated list of\n"
            "  one or more <name>=<value> parameters\n"
            "\n"
            "Supported types:\n"
            "  h264: H264/AVC NAL units\n"
            "    optional params:\n"
            "      dv_profile: integer number for Dolby vision profile ID (valid value: 9)\n"
            "      dv_bc: integer number for Dolby vision BL signal cross-compatibility ID (must be 2 if dv_profile is set to 9)\n"
            "      dv_md_compression: integer number for Dolby vision metadata compression (valid value: 0, 1, 2, 3. default = 0)\n"
            "      frame_rate: if not specified, the frame rate will be inferred from the input stream.\n"
            "      format: avc1 (default) or avc3  for AVC tracks and Dolby Vision back-compatible tracks\n"
            "      dv_feature_flags: a hex number to indicate 10-bit Dolby Vision features\n"
            "            (example: if set 0x200, content has been authored for a Dolby Vision 2 experience. default = 0)\n"
            "  h265: H265/HEVC NAL units\n"
            "    optional params:\n"
            "      dv_profile: integer number for Dolby vision profile ID (valid value: 5,8,20)\n"
            "      dv_bc: integer number for Dolby vision BL signal cross-compatibility ID (mandatory if dv_profile is set to 8)\n"
            "      dv_md_compression: integer number for Dolby vision metadata compression (valid value: 0, 1, 2, 3. default = 0)\n"
            "      frame_rate: if not specified, the frame rate will be inferred from the input stream.\n"
            "      format: hev1 or hvc1 (default) for HEVC tracks and Dolby Vision backward-compatible tracks\n"
            "              dvhe or dvh1 (default) for Dolby vision tracks\n"
            "      amve: customized amve values when AMVE SEI type does not exist in VES\n"
            "            (valid format : ambient_illuminance:ambient_light_x:ambient_light_y)\n"
            "            example: 3140000:15635:16450\n"
            "      dv_feature_flags: a hex number to indicate 10-bit Dolby Vision features\n"
            "            (example: if set 0x200, content has been authored for a Dolby Vision 2 experience. default = 0)\n"
            "      user_sei: add user data unregistered SEI message (valid value: 0, 1)\n"
            "      set_vexu: add vexu box into MV-HEVC MP4 file(valid value: 0, 1. default = 1 for profile 20, default = 0 for others)\n"
            "      hero_eye: set the hero_eye_indicator of HeroStereoEyeDescriptionBox (0 = none, 1 = left, 2 = right. default = 1 for profile 20, default = 0 for others)\n"
            "  aac:  AAC in ADTS format\n"
            "  ac3:  Dolby Digital\n"
            "  ec3:  Dolby Digital Plus\n"
            "  ac4:  Dolby AC-4\n"
            "      pre_roll: floating point number in second for the point at which the track starts playing, only support "
            "              aac, ec3, and ac4 (default = 0) (pre_roll > 0 means to cut off the track from the begining)"
            "      post_roll: floating point number in second for the point at which the track stops playing, only support "
            "              aac, ec3, and ac4 (default = 0) (post_roll > 0 means to cut off the track at the end)\n"
            "  mlp:  Dolby True HD\n"

            "  mp4:  MP4 track(s) from an MP4 file\n"
            "    optional params:\n"
            "      track: audio, video, or integer track ID (default=all tracks)\n"
            "\n"
            "Common optional parameters for all types:\n"
            "  language: language code (3-character ISO 639-2 Alpha-3 code)\n"
            "  label: description of the track\n"
            "    optionally, prepend label's language, separated with ':' (default='en-US')\n"
            "    language is a IETF BCP 47 compliant language tag string\n"
            "    multiple label parameter per track are supported\n"
            "\n"
            "If no type is specified for an input, the type will be inferred from the file extension\n"
            "\n"
            "Options:\n"
            "  --verbose: show more details\n"
            "  --preselection: add preselection information combine with configuration file in .ini format\n"
            "                  please check docs/ for the sample configuration file\n");
    exit(1);
}
/**

 */

/*----------------------------------------------------------------------
|   ParseParameters
+---------------------------------------------------------------------*/
static AP4_Result
ParseParameters(const char* params_str, AP4_Array<Parameter>& parameters)
{
    AP4_Array<AP4_String> params;
    char* p = NULL;
    const char* cursor = params_str;
    bool b_in_single_quote = false;
    bool b_in_double_quote = false;
    bool b_backslash = false;

    // Split by commas, honoring single and double quoted sections
    // as integral parts (commas contained in quotes are ignored for
    // splitting), and skipping commas that are preceeded by a backslash
    do {
        if (*cursor == '\\' && (! b_backslash)) {
            b_backslash = true;
        } else {
            if ((*cursor == '"') && (! b_in_single_quote) && (! b_backslash)) {
                b_in_double_quote = ! b_in_double_quote;
            } else if ((*cursor == '\'') && (! b_in_double_quote) && (! b_backslash)) {
                b_in_single_quote = ! b_in_single_quote;
            } else if (
                        (
                            (*cursor == ',') &&
                            (! b_in_single_quote) &&
                            (! b_in_double_quote) &&
                            (!b_backslash)
                        )
                        ||
                       (*cursor == '\0')
                )
            {
                // Append completed param string to params array, but discard empty param (,,)
                if (p != NULL) {
                    AP4_String param;
                    param.Assign(p, (AP4_Size)strlen(p));
                    params.Append(param);
                    free(p);
                    p = NULL;
                }
            } else {
                // Append current char to p (skipping ',', '/', '"', and ''' if already handled)
                size_t len = (p != NULL) ? strlen(p) : 0;
                char* n = (char *)malloc(len + 2);
                strncpy(n, p, len);
                n[len] = *cursor;
                n[len + 1] = '\0';
                free(p);
                p = n;
            }
            b_backslash = false;
        }
    } while (*cursor++);

    if (p != NULL) free(p);
    // Split each param by equal into name and value, if applicable

    for (unsigned int i=0; i<params.ItemCount(); i++) {
        AP4_String& param = params[i];
        AP4_String name;
        AP4_String value;
        int equal = param.Find('=');
        if (equal >= 0) {
            name.Assign(param.GetChars(), equal);
            value = param.GetChars()+equal+1;
        } else {
            name = param;
            value = NULL;
        }
        parameters.Append(Parameter(name.GetChars(), value.GetChars()));
    }
    
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AssembleInputBoxPayload
+---------------------------------------------------------------------*/
static AP4_Result
AssembleInputBoxPayload(const char* params_str, AP4_Array<AP4_UI32>& values)
{
    const char* cursor = params_str;
    const char* start = params_str;
    do {
        if (*cursor == ':' || *cursor == '\0') {
            AP4_UI32 value;
            AP4_String param;
            param.Assign(start, (unsigned int)(cursor - start));
            value = atoi(param.GetChars());
            values.Append(value);
            if (*cursor == ':') {
                start = cursor + 1;
            }
        }
    } while (*cursor++);
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   ComputeDoviLevel
+---------------------------------------------------------------------*/
static AP4_Result
ComputeDoviLevel(AP4_UI32 video_width, AP4_UI32 video_height, double frame_rate, AP4_UI32& dv_level)
{
    double level = video_width * video_height * frame_rate;

    if (level <= 1280*720*24 && video_width <= 1280) {
        dv_level = 1;
    } else if (level<= 1280*720*30 && video_width <= 1280) {
        dv_level = 2;
    } else if (level <= 1920*1080*24 && video_width <= 1920) {
        dv_level = 3;
    } else if (level <= 1920*1080*30 && video_width <= 2560) {
        dv_level = 4;
    } else if (level <= 1920*1080*60 && video_width <= 3840) {
        dv_level = 5;
    } else if (level <= 3840*2160*24 && video_width <= 3840) {
        dv_level = 6;
    } else if (level <= 3840*2160*30 && video_width <= 3840) {
        dv_level = 7;
    } else if (level <= 3840*2160*48 && video_width <= 3840) {
        dv_level = 8;
    } else if (level <= 3840*2160*60 && video_width <= 3840) {
        dv_level = 9;
    } else if (level <= 3840*2160*120 && video_width <= 7680) {
        if(video_width <= 3840) {
            dv_level = 10;
        } else {
            dv_level = 11;
        }
    } else if (level <= 7680*4320*60 && video_width <= 7680) {
        dv_level = 12;
    } else if (level <= (double)7680*4320*120 && video_width <= 7680) {
        dv_level = 13;
    }

    return AP4_SUCCESS;
}

AP4_UI32 ComputeDeltaDivisor(unsigned int pic_struct_present_flag, unsigned int field_pic_flag, unsigned int pic_struct) {
    // Table E-6 of ITU-T H.264(06/2019)
    uint8_t DeltaTfiDivisorIdx = 1;
    if (!pic_struct_present_flag && !field_pic_flag) {
        DeltaTfiDivisorIdx = 2;
    } else {
        switch (pic_struct) {
            case 1:
            case 2:
                break;
            case 0:
            case 3:
            case 4:
                DeltaTfiDivisorIdx = 2;
                break;
            case 5:
            case 6:
                DeltaTfiDivisorIdx = 3;
                break;
            case 7:
                DeltaTfiDivisorIdx = 4;
                break;
            case 8:
                DeltaTfiDivisorIdx = 6;
                break;
            default:
                break;
        }
    }
    return DeltaTfiDivisorIdx;
}

/*----------------------------------------------------------------------
|   CheckDoviInputParameters
+---------------------------------------------------------------------*/
static AP4_Result
CheckDoviInputParameters(AP4_Array<Parameter>& parameters)
{
    AP4_UI32 profile = 0;
    AP4_UI32 bc = -1;
    AP4_UI32 md_compression = 0;
    AP4_UI32 format = 0;

    //check: profile set correctly
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "dv_profile") {
            profile = atoi(parameters[i].m_Value.GetChars());
            if ((profile != 5) && (profile != 8) && (profile != 9) &&
                (profile != 10) && (profile != 20) && (profile != 32) && (profile != 34)) {
                fprintf(stderr, "ERROR: invalid dolby vision profile id\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    //check: bl signal compatibility id value range
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "dv_bc") {
            bc = atoi(parameters[i].m_Value.GetChars());
            if (bc > 7) {
                fprintf(stderr, "ERROR: invalid dolby vision bl signal compatibility id\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    //check: dv_md_compression value range
    for (unsigned int i = 0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "dv_md_compression") {
            md_compression = atoi(parameters[i].m_Value.GetChars());
            if (md_compression > 3) {
                fprintf(stderr, "ERROR: invalid dolby vision metadata compression value.\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "format") {
            if (parameters[i].m_Value == "hvc1") {
                format = AP4_SAMPLE_FORMAT_HVC1;
            } else if (parameters[i].m_Value == "hev1") {
                format = AP4_SAMPLE_FORMAT_HEV1;
            } else if (parameters[i].m_Value == "dvh1") {
                format = AP4_SAMPLE_FORMAT_DVH1;
            } else if (parameters[i].m_Value == "dvhe") {
                format = AP4_SAMPLE_FORMAT_DVHE;
            } else if (parameters[i].m_Value == "avc1") {
                format = AP4_SAMPLE_FORMAT_AVC1;
            } else if (parameters[i].m_Value == "avc3") {
                format = AP4_SAMPLE_FORMAT_AVC3;
            } else if (parameters[i].m_Value == "dvav") {
                format = AP4_SAMPLE_FORMAT_DVAV;
            } else if (parameters[i].m_Value == "dva1") {
                format = AP4_SAMPLE_FORMAT_DVA1;
            } else if (parameters[i].m_Value == "davc") {
                format = AP4_SAMPLE_FORMAT_DAVC;
            } else if (parameters[i].m_Value == "dvh8") {
                format = AP4_SAMPLE_FORMAT_DVH8;
            }
            else {
                fprintf(stderr, "ERROR: format name is invalid\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }
    }

    //check: sample entry box name is set correctly
    //if (format) {
    //    if ((format == AP4_SAMPLE_FORMAT_DVAV) || (format == AP4_SAMPLE_FORMAT_DVA1) ||
    //        (format == AP4_SAMPLE_FORMAT_AVC1) || (format == AP4_SAMPLE_FORMAT_AVC3)) {
    //        if (profile != 9) {
    //            fprintf(stderr, "ERROR: sample entry name is mismatch with profile\n");
    //            return AP4_ERROR_INVALID_PARAMETERS;
    //        }
    //    } else {
    //        if ((profile != 8) && (profile != 5)) {
    //            fprintf(stderr, "ERROR: sample entry name is mismatch with profile\n");
    //            return AP4_ERROR_INVALID_PARAMETERS;
    //        }
    //    }
    //}

    if (format) {
        if(profile == 5) {
            // profile 5 does not compatible with other profile, only use dvhe or dvh1
            if ((format != AP4_SAMPLE_FORMAT_DVHE) && (format != AP4_SAMPLE_FORMAT_DVH1)) {
                fprintf(stderr, "ERROR: sample entry name is not correct for profile 5\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        } else if (profile == 8) {
            // profile 8 has CCID with 1, 2, 4, may be compatible with HDR10, SDR or HLG, should
            // not use dvhe or dvh1
            if ((format != AP4_SAMPLE_FORMAT_HVC1) && (format != AP4_SAMPLE_FORMAT_HEV1)) {
                fprintf(stderr, "ERROR: sample entry name is not correct for profile 8\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        } else if (profile == 9) {
            // profile 9 only has CCID with 2, which is SDR compliant, should not use dvav or dva1
            if ((format != AP4_SAMPLE_FORMAT_AVC1) && (format != AP4_SAMPLE_FORMAT_AVC3)) {
                fprintf(stderr, "ERROR: sample entry name is not correct for profile 9\n");
                return AP4_ERROR_INVALID_PARAMETERS;
            }
        }else if (profile == 32) {
            if (format != AP4_SAMPLE_FORMAT_DAVC) {
                fprintf(stderr, "ERROR: sample entry name is not correct for profile 32\n");
                return AP4_ERROR_INVALID_PARAMETERS;
          }
        } else if (profile == 34) {
            if (format != AP4_SAMPLE_FORMAT_DVH8) {
                fprintf(stderr, "ERROR: sample entry name is not correct for profile 34\n");
                return AP4_ERROR_INVALID_PARAMETERS;
          }
        }
    }

    //check: for profile 8/9, bl signal compatibility id must be set
    if ((bc == (AP4_UI32)-1) && ((profile == 8) || (profile == 9))) {
        fprintf(stderr, "ERROR: dolby vision bl signal compatibility id must be set when profile is 8 or 9\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 32, bl signal compatibility id must be: 3
    if ((profile == 32) && (bc != 3)) {
        fprintf(stderr, "ERROR: for dolby vision profile 32, bl signal compatibility id must be: 3\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 34, bl signal compatibility id must be: 3
    if ((profile == 34) && (bc != 3)) {
        fprintf(stderr, "ERROR: for dolby vision profile 34, bl signal compatibility id must be: 3\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 8, bl signal compatibility id must be: 1,2,4
    if ((profile == 8) && (bc != 1) && (bc != 2) && (bc != 4)) {
        fprintf(stderr, "ERROR: for dolby vision profile 8, bl signal compatibility id must be: 1, 2 or 4\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 9, bl signal compatibility id must be: 2
    if ((profile == 9) && (bc != 2) ) {
        fprintf(stderr, "ERROR: for dolby vision profile 9, bl signal compatibility id must be 2\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 5, bl signal compatibility id must be 0
    if ((profile == 5) && (bc != 0) && (bc != (AP4_UI32)-1)) {
        fprintf(stderr, "ERROR: for dolby vision profile 5, bl signal compatibility id must be 0\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    if ((profile < 8) && (md_compression != 0)) {
        fprintf(stderr, "ERROR: for dolby vision profile 7 and earlier, dv_md_compression must be 0\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    //check: for profile 10, bl signal compatibility id must be: 0,1,4
    if ((profile == 10) && (bc != 0) && (bc != 1) && (bc != 4)) {
        fprintf(stderr, "ERROR: for dolby vision profile 10, bl signal compatibility id must be: 0, 1 or 4\n");
        return AP4_ERROR_INVALID_PARAMETERS;
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetLabl (Labl atom from parameter)
+---------------------------------------------------------------------*/
static AP4_LablAtom*
GetLabl(AP4_Flags   is_group_label,
        AP4_UI16    label_id,
        const char* default_lang,
        AP4_String param)
{
    // Assemble LABL atom
    AP4_String label = NULL;
    AP4_String label_lang = default_lang; // E.g., "en-US"
    int colon = param.Find(':');
    if (colon >= 0) {
        label_lang.Assign(param.GetChars(), colon);
        label = param.GetChars()+colon+1;
    } else {
        label = param;
    }
    return new AP4_LablAtom(is_group_label, label_id, label_lang.GetChars(), label.GetChars());
}

/*----------------------------------------------------------------------
|   AddUdtaChild (Add atom inside UDTA atom)
+---------------------------------------------------------------------*/
static AP4_Result
AddUdtaChild(AP4_ContainerAtom* parent, AP4_Atom* child)
{
    // Find or create UDTA atom in parent
    AP4_ContainerAtom* udta = NULL;
    AP4_Atom* atom = parent->FindChild("udta");
    if (atom == NULL) {
        udta = new AP4_ContainerAtom(AP4_ATOM_TYPE_UDTA);
        parent->AddChild(udta);
    } else {
        udta = (AP4_ContainerAtom*)atom;
    }
    udta->AddChild(child);

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   ApplyTrackParams (informative atoms)
+---------------------------------------------------------------------*/
static AP4_Result
ApplyTrackParams(AP4_Track* track, AP4_Array<Parameter>& parameters)
{
    // Parse track params
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "label") {
            AP4_String& param = parameters[i].m_Value;
            AP4_LablAtom* labl = GetLabl(false, 0, "en-US", param);
            AddUdtaChild(track->UseTrakAtom(), labl);
        }
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   SampleFileStorage
+---------------------------------------------------------------------*/
class SampleFileStorage
{
public:
    static AP4_Result Create(const char* basename, SampleFileStorage*& sample_file_storage);
    ~SampleFileStorage() {
        m_Stream->Release();
        remove(m_Filename.GetChars());
    }
    
    AP4_ByteStream* GetStream() { return m_Stream; }
    
private:
    SampleFileStorage(const char* basename) : m_Stream(NULL) {
        AP4_Size name_length = (AP4_Size)AP4_StringLength(basename);
        char* filename = new char[name_length+2];
        AP4_CopyMemory(filename, basename, name_length);
        filename[name_length]   = '_';
        filename[name_length+1] = '\0';
        m_Filename = filename;
        delete[] filename;
    }

    AP4_ByteStream* m_Stream;
    AP4_String      m_Filename;
};

/*----------------------------------------------------------------------
|   SampleFileStorage::Create
+---------------------------------------------------------------------*/
AP4_Result
SampleFileStorage::Create(const char* basename, SampleFileStorage*& sample_file_storage)
{
    sample_file_storage = NULL;
    SampleFileStorage* object = new SampleFileStorage(basename);
    AP4_Result result = AP4_FileByteStream::Create(object->m_Filename.GetChars(),
                                                   AP4_FileByteStream::STREAM_MODE_WRITE,
                                                   object->m_Stream);
    if (AP4_FAILED(result)) {
        return result;
    }
    sample_file_storage = object;
    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   SortSamples
+---------------------------------------------------------------------*/
static void
SortSamples(SampleOrder* array, unsigned int n)
{
    if (n < 2) {
        return;
    }
    SampleOrder pivot = array[n / 2];
    SampleOrder* left  = array;
    SampleOrder* right = array + n - 1;
    while (left <= right) {
        if (left->m_DisplayOrder < pivot.m_DisplayOrder) {
            ++left;
            continue;
        }
        if (right->m_DisplayOrder > pivot.m_DisplayOrder) {
            --right;
            continue;
        }
        SampleOrder temp = *left;
        *left++ = *right;
        *right-- = temp;
    }
    SortSamples(array, (unsigned int)(right - array + 1));
    SortSamples(left, (unsigned int)(array + n - left));
}

/*----------------------------------------------------------------------
|   GetLanguageFromParameters
+---------------------------------------------------------------------*/
static const char*
GetLanguageFromParameters(AP4_Array<Parameter>& parameters, const char* defaut_value)
{
    // check if we have a language parameter
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "language") {
            const char* language = parameters[i].m_Value.GetChars();

            // the language must be a 3-character ISO 639-2 Alpha-3 code
            if (strlen(language) != 3) {
                fprintf(stderr, "ERROR: language codes must be 3-character ISO 639-2 Alpha-3 codes\n");
                return NULL;
            }

            return language;
        }
    }

    return defaut_value;
}

/*----------------------------------------------------------------------
|   GetAc4LanguageFromDsi
+---------------------------------------------------------------------*/
static void
GetAc4LanguageFromDsi(const AP4_Dac4Atom::Ac4Dsi& ac4_dsi, char language[4])
{
    if (ac4_dsi.ac4_dsi_version != 1) {
        return;
    }
    bool found_language = false;
    for (unsigned int presentation_idx = 0; presentation_idx < ac4_dsi.d.v1.n_presentations; presentation_idx++) {
        const AP4_Dac4Atom::Ac4Dsi::PresentationV1& presentation = ac4_dsi.d.v1.presentations[presentation_idx];
        if (presentation.presentation_version != 1 && presentation.presentation_version != 2) {
            continue;
        }
        for (unsigned int substream_group_idx = 0; substream_group_idx < presentation.d.v1.n_substream_groups; substream_group_idx++) {
            const AP4_Dac4Atom::Ac4Dsi::SubStreamGroupV1& substream_group = presentation.d.v1.substream_groups[substream_group_idx];
            if (substream_group.d.v1.b_content_type &&
                (substream_group.d.v1.content_classifier == 0 || substream_group.d.v1.content_classifier == 4) &&
                substream_group.d.v1.b_language_indicator) {
                char substream_group_language[4];
                if (AP4_Ac4ConvertLanguageTagToMdhdLanguage(substream_group.d.v1.language_tag_bytes,
                                                            substream_group.d.v1.n_language_tag_bytes,
                                                            substream_group_language)) {
                    if (found_language && strcmp(language, substream_group_language)) {
                        fprintf(stderr, "WARNING: AC-4 DSI has multiple languages, mdhd language defaults to 'und'\n");
                        language[0] = 'u';
                        language[1] = 'n';
                        language[2] = 'd';
                        language[3] = '\0';
                        return;
                    }
                    memcpy(language, substream_group_language, 4);
                    found_language = true;
                } else {
                    fprintf(stderr, "WARNING: AC-4 substream group language tag cannot be mapped to mdhd language\n");
                }
            }
        }
    }

    if (found_language) {
        fprintf(stdout, "INFO: AC-4 DSI language mapped to mdhd language '%s'\n", language);
    }
}

/*----------------------------------------------------------------------
|   ParseGaplessOffsetParameter
+---------------------------------------------------------------------*/
static Gapless_roll
ParseGaplessOffsetParameter(AP4_Array<Parameter>& parameters)
{
    Gapless_roll offset(0, 0);
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "pre_roll") {
            offset.pre_roll = atof(parameters[i].m_Value.GetChars());
        }
        if (parameters[i].m_Name == "post_roll") {
            offset.post_roll = atof(parameters[i].m_Value.GetChars());
        }
    }
    
    return offset;
}

/*----------------------------------------------------------------------
|   ConvertRollEditList
+---------------------------------------------------------------------*/
static AP4_Result
ConvertRollEditList(AP4_UI64            duration,
                    AP4_UI32            movie_time_scale,
                    AP4_UI32            media_time_scale,
                    Gapless_roll&       offset,
                    AP4_ElstAtom*       new_elst)
{
    // check if there is offset
    if (offset.pre_roll == 0 && offset.post_roll == 0) {
        AP4_ElstEntry entry = AP4_ElstEntry(duration, 0, 1);
        new_elst->AddEntry(entry);
        return AP4_SUCCESS;
    }

    AP4_SI64 pre = AP4_SI64(offset.pre_roll * movie_time_scale + 0.5);
    AP4_SI64 post = AP4_SI64(offset.post_roll * movie_time_scale + 0.5);
    AP4_UI64 media_time = 0;
    AP4_UI64 edit_duration = duration;
    
    // check if the rolling is acceptable??
    if (duration < AP4_UI64(abs(pre) + abs(post))) {
        fprintf(stderr, "ERROR: The track is too short to handle pre_roll and post_roll, duration (%llu)s\n", duration / movie_time_scale);
        return AP4_FAILURE;
    }

    // edit list with media_time = -1 at the begining of presentation
    if (pre < 0) {
        AP4_ElstEntry entry = AP4_ElstEntry(-pre, -1, 1);
        new_elst->AddEntry(entry);
    }

    // edit list of the track playing
    if (pre > 0) {
        media_time = AP4_SI64(offset.pre_roll * media_time_scale + 0.5);
        edit_duration -= pre;
    }
    if (post > 0) {
        edit_duration -= post;
    }
    AP4_ElstEntry entry = AP4_ElstEntry(edit_duration, media_time, 1);
    new_elst->AddEntry(entry);
    
    // edit list with media_time = -1 at the end of presentation
    if (post < 0) {
        AP4_ElstEntry entry = AP4_ElstEntry(-post, -1, 1);
        new_elst->AddEntry(entry);
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AddAacTrack
+---------------------------------------------------------------------*/
static void
AddAacTrack(AP4_Movie&            movie,
            const char*           input_name,
            AP4_Array<Parameter>& parameters,
            SampleFileStorage&    sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;
    // check if we have a pre_roll parameter or a post_roll parameter, default 0
    Gapless_roll offset = ParseGaplessOffsetParameter(parameters);
    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // create an ADTS parser
    AP4_AdtsParser parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get AAC frames
    AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;
    bool eos = false;
    for(;;) {
        // try to get a frame
        AP4_AacFrame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (g_Verbose) {
                printf("AAC frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameLength,
                       (int)frame.m_Info.m_SamplingFrequency,
                       frame.m_Info.m_ChannelConfiguration);
            }
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_DataBuffer dsi;
                unsigned char aac_dsi[2];

                unsigned int object_type = 2; // AAC LC by default
                aac_dsi[0] = (object_type<<3) | (frame.m_Info.m_SamplingFrequencyIndex>>1);
                aac_dsi[1] = ((frame.m_Info.m_SamplingFrequencyIndex&1)<<7) | (frame.m_Info.m_ChannelConfiguration<<3);

                dsi.SetData(aac_dsi, 2);
                AP4_MpegAudioSampleDescription* sample_description = 
                    new AP4_MpegAudioSampleDescription(
                    AP4_OTI_MPEG4_AUDIO,   // object type
                    (AP4_UI32)frame.m_Info.m_SamplingFrequency,
                    16,                    // sample size
                    frame.m_Info.m_ChannelConfiguration,
                    &dsi,                  // decoder info
                    6144,                  // buffer size
                    128000,                // max bitrate
                    128000);               // average bitrate
                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                sample_rate = (AP4_UI32)frame.m_Info.m_SamplingFrequency;
            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameLength);
            sample_data.SetDataSize(frame.m_Info.m_FrameLength);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameLength);
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameLength);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(), position, frame.m_Info.m_FrameLength, 1024, sample_description_index, 0, 0, true);
            sample_count++;
        } else {
            if (result == AP4_ERROR_CORRUPTED_BITSTREAM) {
                fprintf(stderr, "WARN: The stream in corrupted, muxing stopped. The muxed MP4 only contains the first %d samples.\n", sample_count);
                break;
            } else if ((result == AP4_ERROR_NOT_ENOUGH_DATA) && (parser.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE -1))){
                fprintf(stderr, "WARN: The frame %d size is larger than the max buffer size, muxing stopped. The muxed MP4 only contains the first %d samples.\n", sample_count + 1, sample_count);
                break;
            }
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            } else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }
    
    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                     sample_table,
                                     0,                 // track id
                                     sample_rate,       // movie time scale
                                     sample_count*1024, // track duration
                                     sample_rate,       // media time scale
                                     sample_count*1024, // media duration
                                     language,          // language
                                     0, 0);             // width, height
    // create an 'edts' container
    AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
    AP4_ElstAtom* new_elst = new AP4_ElstAtom();
    AP4_UI64 duration = 0;
    AP4_UI32 movie_time_sacle = movie.GetTimeScale();
    if(!movie_time_sacle) {
        duration = sample_count * 1024;
        movie_time_sacle = track->GetMediaTimeScale();
    } else {
        duration = AP4_ConvertTime(1024*sample_table->GetSampleCount(), sample_rate, movie.GetTimeScale());
    }
    if (AP4_FAILED(ConvertRollEditList(duration, movie_time_sacle, track->GetMediaTimeScale(), offset, new_elst))) {
        input->Release();
        return;
    }
    new_edts->AddChild(new_elst);
    track->SetEditList(new_edts, movie_time_sacle);
    // cleanup
    input->Release();

    movie.AddTrack(track);

    ApplyTrackParams(track, parameters);
}

/*----------------------------------------------------------------------
 |   AddAc3Track
 +---------------------------------------------------------------------*/
static void
AddAc3Track(AP4_Movie&             movie,
            const char*             input_name,
            AP4_Array<Parameter>&   parameters,
            SampleFileStorage&      sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable(); // chunk_size is used to control chunk size in 'stsc' box

    // create an AC-3 Sync Frame parser
    AP4_Ac3Parser parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get AC-3 frames
    AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;

    bool eos = false;
    for(;;) {
        // try to get a frame
        AP4_Ac3Frame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (g_Verbose) {
                printf("AC-3 frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameSize,
                       frame.m_Info.m_SampleRate,
                       frame.m_Info.m_ChannelCount);
            }
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_Dac3Atom::StreamInfo *ac3_stream_info = &frame.m_Info.m_Ac3StreamInfo;

                AP4_Ac3SampleDescription* sample_description =
                new AP4_Ac3SampleDescription(
                                             frame.m_Info.m_SampleRate,       // sample rate
                                             16,                              // sample size
                                             2,                               // channel count
                                             frame.m_Info.m_FrameSize,        // Access Unit size
                                             ac3_stream_info);            // AC-3 SubStream

                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                sample_rate      = frame.m_Info.m_SampleRate;
            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameSize);
            sample_data.SetDataSize(frame.m_Info.m_FrameSize);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameSize);
            if (frame.m_LittleEndian) {
                AP4_ByteSwap16(sample_data.UseData(), (unsigned int)frame.m_Info.m_FrameSize);
            }
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameSize);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(), position, frame.m_Info.m_FrameSize, 1536, sample_description_index, 0, 0, true);
            sample_count++;
        } else {
            if (result == AP4_ERROR_CORRUPTED_BITSTREAM) {
                fprintf(stderr, "WARN: The stream in corrupted, so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count);
                break;
            } else if ((result == AP4_ERROR_NOT_ENOUGH_DATA) && (parser.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE -1))){
                fprintf(stderr, "WARN: The frame %d size is larger than pre-defined buffer size (%d bytes), so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count + 1, AP4_BITSTREAM_BUFFER_SIZE, sample_count);
                break;
            }
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            } else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }

    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                     sample_table,
                                     0,                   // track id
                                     sample_rate,         // movie time scale
                                     sample_count * 1536, // track duration
                                     sample_rate,         // media time scale
                                     sample_count * 1536, // media duration
                                     language,            // language
                                     0, 0);

    // add an edit list with MediaTime==0 to ac3 track defautly.
    if(1) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        AP4_UI32 edts_timescale = sample_rate;
        if(!movie.GetTimeScale()) {
            duration = sample_count * 1536;
        } else {
            duration = AP4_ConvertTime(1536*sample_table->GetSampleCount(), sample_rate, movie.GetTimeScale());
            edts_timescale = movie.GetTimeScale();
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, 0, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->SetEditList(new_edts, edts_timescale);
    }

    // cleanup
    input->Release();

    movie.AddTrack(track);

    ApplyTrackParams(track, parameters);
}

/*----------------------------------------------------------------------
|   AddEac3Track
+---------------------------------------------------------------------*/
static void
AddEac3Track(AP4_Movie&             movie,
            const char*             input_name,
            AP4_Array<Parameter>&   parameters,
            SampleFileStorage&      sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // check if we have a pre_roll parameter or a post_roll parameter, default 0
    Gapless_roll offset = ParseGaplessOffsetParameter(parameters);
    
    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable(); // The parameter chunk_size is used to control chunk size in 'stsc' box

    // create an E-AC-3 Sync Frame parser
    AP4_Eac3Parser parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get E-AC-3 frames
    AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;

    bool eos = false;
    for(;;) {
        // try to get a frame
        AP4_Eac3Frame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (g_Verbose) {
                printf("E-AC-3 frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameSize,
                       frame.m_Info.m_SampleRate,
                       frame.m_Info.m_ChannelCount);
            }
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_Dec3Atom::SubStream *eac3_substream = &frame.m_Info.m_Eac3SubStream;
                const unsigned int obj_num = frame.m_Info.complexity_index_type_a;

                AP4_Eac3SampleDescription* sample_description =
                    new AP4_Eac3SampleDescription(
                    frame.m_Info.m_SampleRate,      // sample rate
                    16,                             // sample size
                    2,                              // channel count
                    frame.m_Info.m_FrameSize,       // Access Unit size
                    eac3_substream,                 // E-AC-3 SubStream
                    obj_num);                       // DD+JOC object numbers
                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                sample_rate      = frame.m_Info.m_SampleRate;
            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameSize);
            sample_data.SetDataSize(frame.m_Info.m_FrameSize);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameSize);
            if (frame.m_LittleEndian) {
                AP4_ByteSwap16(sample_data.UseData(), (unsigned int)frame.m_Info.m_FrameSize);
            }
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameSize);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(), position, frame.m_Info.m_FrameSize, 1536, sample_description_index, 0, 0, true);
            sample_count++;
        } else {
            if (result == AP4_ERROR_CORRUPTED_BITSTREAM) {
                fprintf(stderr, "WARN: The stream in corrupted, so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count);
                break;
            } else if ((result == AP4_ERROR_NOT_ENOUGH_DATA) && (parser.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE -1))){
                fprintf(stderr, "WARN: The frame %d size is larger than pre-defined buffer size (8191 bytes), so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count + 1, sample_count);
                break;
            }
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            } else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }

    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                     sample_table,
                                     0,                   // track id
                                     sample_rate,         // movie time scale
                                     sample_count * 1536, // track duration
                                     sample_rate,         // media time scale
                                     sample_count * 1536, // media duration
                                     language,            // language
                                     0, 0);               // width, height

    // add an edit list based on RollOffset to ec3 track, default 0.
    if(1) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        AP4_UI32 movie_time_scale = movie.GetTimeScale();
        if(!movie_time_scale) {
            duration = sample_count * 1536;
            movie_time_scale = track->GetMediaTimeScale();
        } else {
            duration = AP4_ConvertTime(1536*sample_table->GetSampleCount(), sample_rate, movie.GetTimeScale());
        }
        if (AP4_FAILED(ConvertRollEditList(duration, movie_time_scale, track->GetMediaTimeScale(), offset, new_elst))) {
            input->Release();
            return;
        }
        new_edts->AddChild(new_elst);
        track->SetEditList(new_edts, movie_time_scale);
    }

    // cleanup
    input->Release();

    movie.AddTrack(track);

    ApplyTrackParams(track, parameters);
}

/*----------------------------------------------------------------------
|   AddAc4Track
+---------------------------------------------------------------------*/
static void
AddAc4Track(AP4_Movie&            movie,
            const char*           input_name,
            AP4_Array<Parameter>& parameters,
            SampleFileStorage&    sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    bool language_parameter_present = false;
    for (unsigned int i = 0; i < parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "language") {
            language_parameter_present = true;
            break;
        }
    }
    const char* language = GetLanguageFromParameters(parameters, NULL);
    if (language_parameter_present && !language) return;
    
    // check if we have a pre_roll parameter or a post_roll parameter, default 0
    Gapless_roll offset = ParseGaplessOffsetParameter(parameters);

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable(); // The parameter chunk_size is used to control chunk size in 'stsc' box

    // create an AC-4 Sync Frame parser
    AP4_Ac4Parser parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get AC-4 frames
    // AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;
    AP4_Cardinal sample_duration = 0;
    AP4_Cardinal media_time_scale = 0;
    bool b_frame0 = false;
    AP4_Ac4Frame frame0; // Keep initial 'frame' available for subsequent processing
    bool eos = false;
    for(;;) {
        // try to get a frame
        AP4_Ac4Frame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (g_Verbose) {
                printf("AC-4 frame [%06d]: size = %d, %d kHz, %d ch\n",
                       sample_count,
                       frame.m_Info.m_FrameSize,
                       (int)frame.m_Info.m_Ac4Dsi.d.v1.fs,
                       2);
            }
            if (!initialized) {
                initialized = true;
                b_frame0 = true;

                // create a sample description for our samples
                AP4_Dac4Atom::Ac4Dsi *ac4Dsi = &frame.m_Info.m_Ac4Dsi;

                AP4_Ac4SampleDescription* sample_description =
                    new AP4_Ac4SampleDescription(
                    frame.m_Info.m_Ac4Dsi.d.v1.fs,  // sample rate
                    16,                             // sample size
                    2,                             // channel count
                    frame.m_Info.m_FrameSize,       // DIS size, can't calcuate the DSI size in advance, so assume the maximum value is frame size.
                    ac4Dsi);                        // AC-4 DSI
                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                /* sample_rate = frame.m_Info.m_Ac4Dsi.d.v1.fs */;
                sample_duration  = frame.m_Info.m_SampleDuration;
                media_time_scale = frame.m_Info.m_MediaTimeScale;

                // Clone first frame
                frame0 = frame;
                // Allocate array to hold presentations
                frame0.m_Info.m_Ac4Dsi.d.v1.presentations = new AP4_Dac4Atom::Ac4Dsi::PresentationV1[frame.m_Info.m_Ac4Dsi.d.v1.n_presentations];
                memcpy(frame0.m_Info.m_Ac4Dsi.d.v1.presentations, ac4Dsi->d.v1.presentations, sizeof(AP4_Dac4Atom::Ac4Dsi::PresentationV1) * frame.m_Info.m_Ac4Dsi.d.v1.n_presentations);
            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameSize);
            sample_data.SetDataSize(frame.m_Info.m_FrameSize);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameSize);
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameSize);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(), position, frame.m_Info.m_FrameSize, sample_duration, sample_description_index, 0, 0, (frame.m_Info.m_Iframe == 1));
            sample_count++;
            // Skip the CRC word for 0xAC41 stream
            frame.m_Source->SkipBytes(frame.m_Info.m_CRCSize);
        } else {
            if (result == AP4_ERROR_CORRUPTED_BITSTREAM) {
                fprintf(stderr, "WARN: The stream in corrupted, so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count);
                break;
            } else if ((result == AP4_ERROR_NOT_ENOUGH_DATA) && (parser.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE -1))){
                fprintf(stderr, "WARN: The frame %d size is larger than pre-defined buffer size (%d bytes), so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count + 1, AP4_BITSTREAM_BUFFER_SIZE, sample_count);
                break;
            }
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            } else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }

    char dsi_language[4] = "und";
    if (!language) {
        if (b_frame0) {
            GetAc4LanguageFromDsi(frame0.m_Info.m_Ac4Dsi, dsi_language);
        }
        language = dsi_language;
    }

    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
                                     sample_table,
                                     0,                                        // track id
                                     media_time_scale,                         // movie time scale
                                     AP4_UI64(sample_count) * sample_duration, // track duration
                                     media_time_scale,                         // media time scale
                                     AP4_UI64(sample_count) * sample_duration, // media duration
                                     language,                                 // language
                                     0, 0);                                    // width, height

    // add an edit list based on RollOffset to ac4 track, default 0.
    if(1) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        AP4_UI32 movie_time_scale = movie.GetTimeScale();
        if(!movie_time_scale) {
            duration = AP4_UI64(sample_count) * sample_duration;
            movie_time_scale = track->GetMediaTimeScale();
        } else {
            duration = AP4_ConvertTime(sample_duration*sample_table->GetSampleCount(), media_time_scale, movie.GetTimeScale());
        }
        if (AP4_FAILED(ConvertRollEditList(duration, movie_time_scale, track->GetMediaTimeScale(), offset, new_elst))) {
            input->Release();
            return;
        }
        new_edts->AddChild(new_elst);
        track->SetEditList(new_edts, movie_time_scale);
    }

    // cleanup
    input->Release();

    movie.AddTrack(track); // Assigns track_id

    // Preselections: Adding presentations to Group List of preselections
    if (g_Preselection) {
        g_Preselection->generateGrpl(input_name, track->GetId(), &frame0);
    }
    ApplyTrackParams(track, parameters);

    // Free clone
    if (b_frame0) {
        delete[] frame0.m_Info.m_Ac4Dsi.d.v1.presentations;
    }

}

/*----------------------------------------------------------------------
|   AddMlpTrack
+---------------------------------------------------------------------*/
static void
AddMlpTrack(AP4_Movie&             movie,
    const char*             input_name,
    AP4_Array<Parameter>&   parameters,
    SampleFileStorage&      sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable(); // chunk_size is used to control chunk size in 'stsc' box

    // create an MLP Frame parser
    AP4_MlpParser  parser;
    bool           initialized = false;
    unsigned int   sample_description_index = 0;

    // read from the input, feed, and get AC-3 frames
    AP4_UI32     sample_rate = 0;
    AP4_Cardinal sample_count = 0;
    AP4_Cardinal sample_duration = 0;
    AP4_Cardinal media_time_scale = 0;

    bool eos = false;
    for (;;) {
        // try to get a frame
        AP4_MlpFrame frame;
        result = parser.FindFrame(frame);
        if (AP4_SUCCEEDED(result)) {
            if (g_Verbose) {
                printf("MLP frame [%06d]: size = %d, %d kHz, %d ch\n",
                    sample_count,
                    frame.m_Info.m_FrameSize,
                    frame.m_Info.m_SampleRate,
                    frame.m_Info.m_ChannelCount);
            }
            if (!initialized) {
                initialized = true;

                // create a sample description for our samples
                AP4_DmlpAtom::StreamInfo *mlp_stream_info = &frame.m_Info.m_MlpStreamInfo;
                AP4_MlpSampleDescription* sample_description =
                    new AP4_MlpSampleDescription(
                        frame.m_Info.m_SampleRate,       // sample rate
                        16,                              // sample size
                        2,                               // channel count
                        frame.m_Info.m_FrameSize,        // Access Unit size
                        mlp_stream_info);                // MLP stream info

                sample_description_index = sample_table->GetSampleDescriptionCount();
                sample_table->AddSampleDescription(sample_description);
                sample_rate = frame.m_Info.m_SampleRate;
                sample_duration = frame.m_Info.m_SampleDuration;
                media_time_scale = frame.m_Info.m_MediaTimeScale;
            }

            // read and store the sample data
            AP4_Position position = 0;
            sample_storage.GetStream()->Tell(position);
            AP4_DataBuffer sample_data(frame.m_Info.m_FrameSize);
            sample_data.SetDataSize(frame.m_Info.m_FrameSize);
            frame.m_Source->ReadBytes(sample_data.UseData(), frame.m_Info.m_FrameSize);
            sample_storage.GetStream()->Write(sample_data.GetData(), frame.m_Info.m_FrameSize);

            // add the sample to the table
            sample_table->AddSample(*sample_storage.GetStream(),
                                    position,
                                    frame.m_Info.m_FrameSize,
                                    frame.m_Info.m_SampleDuration,
                                    sample_description_index,
                                    0, 0,
                                    (frame.m_Info.m_Iframe == 1));
            sample_count++;
        }
        else {
            if (result == AP4_ERROR_CORRUPTED_BITSTREAM) {
                fprintf(stderr, "WARN: The stream in corrupted, so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count);
                break;
            }
            else if ((result == AP4_ERROR_NOT_ENOUGH_DATA) && (parser.GetBytesAvailable() == (AP4_BITSTREAM_BUFFER_SIZE - 1))) {
                fprintf(stderr, "WARN: The frame %d size is larger than pre-defined buffer size (%d bytes), so stop muxing. The muxed MP4 only contains the first %d samples.\n", sample_count + 1, AP4_BITSTREAM_BUFFER_SIZE, sample_count);
                break;
            }
            else if (result == AP4_ERROR_INVALID_FORMAT) {
                fprintf(stderr, "WARN: The stream format is invalid, Dolby TrueHD stream shall begin with either a SMPTE header or instant major sync.");
                break;
            }
            if (eos) break;
        }

        // read some data and feed the parser
        AP4_UI08 input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size to_read = parser.GetBytesFree();
        if (to_read) {
            AP4_Size bytes_read = 0;
            if (to_read > sizeof(input_buffer)) to_read = sizeof(input_buffer);
            result = input->ReadPartial(input_buffer, to_read, bytes_read);
            if (AP4_SUCCEEDED(result)) {
                AP4_Size to_feed = bytes_read;
                result = parser.Feed(input_buffer, &to_feed);
                if (AP4_FAILED(result)) {
                    AP4_Debug("ERROR: parser.Feed() failed (%d)\n", result);
                    return;
                }
            }
            else {
                if (result == AP4_ERROR_EOS) {
                    eos = true;
                    parser.Feed(NULL, NULL, AP4_BITSTREAM_FLAG_EOS);
                }
            }
        }
    }

    // create an audio track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_AUDIO,
        sample_table,
        0,                   // track id
        media_time_scale,         // movie time scale
        sample_count * sample_duration, // track duration
        media_time_scale,         // media time scale
        sample_count * sample_duration, // media duration
        language,            // language
        0, 0                 // width, height
        );

    if (1) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        AP4_UI32 edts_timescale = media_time_scale;
        if (!movie.GetTimeScale()) {
            duration = sample_count * sample_duration;
        }
        else {
            duration = AP4_ConvertTime(1000 * sample_table->GetSampleCount(), sample_rate, movie.GetTimeScale());
            edts_timescale = movie.GetTimeScale();
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, 0, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->SetEditList(new_edts, edts_timescale);
    }

    // cleanup
    input->Release();

    movie.AddTrack(track);
}


/*----------------------------------------------------------------------
|   AddH264Track
+---------------------------------------------------------------------*/
static void
AddH264Track(AP4_Movie&            movie,
             const char*           input_name,
             AP4_Array<Parameter>& parameters,
             AP4_Array<AP4_UI32>&  brands,
             SampleFileStorage&    sample_storage)
{
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // see if the frame rate is specified
    double user_frame_rate = 0.0;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "frame_rate") {
            double frame_rate = atof(parameters[i].m_Value.GetChars());
            frame_rate = AP4_NormalizeFrameRate(frame_rate);
            if (frame_rate == 0.0) {
                fprintf(stderr, "ERROR: invalid video frame rate %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
            user_frame_rate = frame_rate;
        }
    }
    
    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // allocate an array to keep track of sample order
    AP4_Array<SampleOrder> sample_orders;
    
    // parse the input
    AP4_AvcFrameParser parser;
    for (;;) {
        bool eos;
        unsigned char input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        bool     found_access_unit = false;
        do {
            AP4_AvcFrameParser::AccessUnitInfo access_unit_info;

            found_access_unit = false;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset],
                                 bytes_in_buffer,
                                 bytes_consumed,
                                 access_unit_info,
                                 eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
            if (access_unit_info.nal_units.ItemCount()) {
                // we got one access unit
                found_access_unit = true;
                if (g_Verbose) {
                    printf("H264 Access Unit, %d NAL units, decode_order=%d, display_order=%d\n",
                           access_unit_info.nal_units.ItemCount(),
                           access_unit_info.decode_order,
                           access_unit_info.display_order);
                }
                
                // compute the total size of the sample data
                unsigned int sample_data_size = 0;
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
                }

                // store the sample data
                AP4_Position position = 0;
                sample_storage.GetStream()->Tell(position);
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_storage.GetStream()->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
                    sample_storage.GetStream()->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
                }

                // add the sample to the track
                sample_table->AddSample(*sample_storage.GetStream(), position, sample_data_size, 1000, 0, 0, 0, access_unit_info.is_idr);
            
                // remember the sample order
                sample_orders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));
                
                // free the memory buffers
                access_unit_info.Reset();
            }

            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer || found_access_unit);
        if (eos) break;
    }

    // adjust the sample CTS/DTS offsets based on the sample orders
    if (sample_orders.ItemCount() > 1) {
        unsigned int start = 0;
        for (unsigned int i=1; i<=sample_orders.ItemCount(); i++) {
            if (i == sample_orders.ItemCount() || sample_orders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&sample_orders[start], i-start);
                start = i;
            }
        }
    }
    unsigned int max_delta = 0;
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        if (sample_orders[i].m_DecodeOrder > i) {
            unsigned int delta =sample_orders[i].m_DecodeOrder-i;
            if (delta > max_delta) {
                max_delta = delta;
            }
        }
    }
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        sample_table->UseSample(sample_orders[i].m_DecodeOrder).SetCts(1000ULL*(AP4_UI64)(i+max_delta));
    }

    // check the video parameters
    AP4_AvcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        sps = parser.GetSequenceParameterSets()[i];
        if (sps) {
            break;
        }
    }
    if (sps == NULL) {
        fprintf(stderr, "ERROR: no sequence parameter set found in video\n");
        input->Release();
        return;
    }
    unsigned int video_width = 0;
    unsigned int video_height = 0;
    sps->GetInfo(video_width, video_height);
    if (g_Verbose) {
        printf("VIDEO: %dx%d\n", video_width, video_height);
    }
    // Compute the video frame rate based on the timing info in the SPS and the slice header
    const AP4_AvcSliceHeader* slice_header = parser.GetSliceHeader();
    double frame_rate = 0.0;
    if (sps->timing_info_present_flag) {
        frame_rate = (double)(sps->time_scale) / (double)(sps->num_units_in_tick);
        frame_rate = frame_rate / (double)ComputeDeltaDivisor(sps->pic_struct_present_flag, slice_header->field_pic_flag, sps->pic_struct);
    } else {
        fprintf(stderr, "WARNING: no timing info in SPS, use user specified frame rate %.2f\n", user_frame_rate);
        frame_rate = user_frame_rate;
    }
    frame_rate = AP4_NormalizeFrameRate(frame_rate);
    if (user_frame_rate != 0.0 && AP4_FrameRatesDiffer(user_frame_rate, frame_rate)) {
        fprintf(stderr, "WARN: user specified frame rate %.2f does not match the frame rate in the SPS (%.2f)\n", user_frame_rate, frame_rate);
        frame_rate = user_frame_rate;
    }
    if (frame_rate == 0.0 && user_frame_rate == 0.0) {
        fprintf(stderr, "ERROR: no timing info in SPS, cannot determine frame rate. Please specify the frame rate manually.\n");
        exit(1);
    }

    // collect the SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps_array.Append(parser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_AVC_PPS_MAX_ID; i++) {
        if (parser.GetPictureParameterSets()[i]) {
            pps_array.Append(parser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }
    
    // setup the video the sample descripton
    AP4_AvcSampleDescription* sample_description =
        new AP4_AvcSampleDescription(AP4_SAMPLE_FORMAT_AVC1,
                                     video_width,
                                     video_height,
                                     24,
                                     "AVC Coding",
                                     sps->profile_idc,
                                     sps->level_idc,
                                     sps->constraint_set0_flag<<7 |
                                     sps->constraint_set1_flag<<6 |
                                     sps->constraint_set2_flag<<5 |
                                     sps->constraint_set3_flag<<4,
                                     4,
                                     sps->chroma_format_idc,
                                     sps->bit_depth_luma_minus8,
                                     sps->bit_depth_chroma_minus8,
                                     sps_array,
                                     pps_array);
    sample_table->AddSampleDescription(sample_description);

    AP4_UI32 movie_timescale      = 1000;
    AP4_UI32 media_timescale      = AP4_FrameRateToTimeScale(frame_rate);
    AP4_UI64 video_track_duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie_timescale);
    AP4_UI64 video_media_duration = 1000*sample_table->GetSampleCount();

    // create a video track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                     sample_table,
                                     0,                    // auto-select track id
                                     movie_timescale,      // movie time scale
                                     video_track_duration, // track duration
                                     media_timescale,      // media time scale
                                     video_media_duration, // media duration
                                     language,             // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );
    // Use an edit list to compensate for the inital cts offset
    if (max_delta) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        AP4_UI32 edts_timescale = media_timescale;
        if(!movie.GetTimeScale()) {
            duration = video_media_duration;
        } else {
            duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie.GetTimeScale());
            edts_timescale = movie.GetTimeScale();
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, max_delta*1000ULL, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->SetEditList(new_edts, edts_timescale);
    }
    // update the brands list
    brands.Append(AP4_FILE_BRAND_AVC1);

    // cleanup
    input->Release();

    movie.AddTrack(track);

    ApplyTrackParams(track, parameters);
}

/*----------------------------------------------------------------------
|   AddH264DoviTrack
+---------------------------------------------------------------------*/
static void
AddH264DoviTrack(AP4_Movie&            movie,
                 const char*           input_name,
                 AP4_Array<Parameter>& parameters,
                 AP4_Array<AP4_UI32>&  brands,
                 SampleFileStorage&    sample_storage)
{
    double frame_rate = 0.0;
    //based on the Dovi iso spec, set the following values to const 
    const AP4_UI32 dv_major_version = 1;
    const AP4_UI32 dv_minor_version = 0;
    const bool     dv_rpu_flag = 1;
    const bool     dv_el_flag = 0;
    const bool     dv_bl_flag = 1;

    AP4_UI32 dv_profile = 0;
    AP4_UI32 dv_bl_signal_comp_id = 0;
    AP4_UI32 dv_level = 0;
    AP4_UI32 dv_md_compression = 0;
    AP4_UI32 dv_feature_flags = 0;

    AP4_UI32 format = 0;

    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // see if the frame rate is specified
    double user_frame_rate = 0;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "frame_rate") {
            user_frame_rate = atof(parameters[i].m_Value.GetChars());
            user_frame_rate = AP4_NormalizeFrameRate(user_frame_rate);
            if (user_frame_rate == 0.0) {
                fprintf(stderr, "ERROR: invalid video frame rate %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
        } else if (parameters[i].m_Name == "format") {
            if (parameters[i].m_Value == "avc1") {
                format = AP4_SAMPLE_FORMAT_AVC1;
            } else if (parameters[i].m_Value == "avc3") {
                format = AP4_SAMPLE_FORMAT_AVC3;
            } else if (parameters[i].m_Value == "dva1") {
                format = AP4_SAMPLE_FORMAT_DVA1;
            } else if (parameters[i].m_Value == "dvav") {
                format = AP4_SAMPLE_FORMAT_DVAV;
            } else if (parameters[i].m_Value == "davc") {
                format = AP4_SAMPLE_FORMAT_DAVC;
            }
        } else if (parameters[i].m_Name == "dv_profile") {
            dv_profile = atoi(parameters[i].m_Value.GetChars());
        } else if (parameters[i].m_Name == "dv_bc") {
            dv_bl_signal_comp_id = atoi(parameters[i].m_Value.GetChars());
        } else if (parameters[i].m_Name == "dv_md_compression") {
            dv_md_compression = atoi(parameters[i].m_Value.GetChars());
        } else if (parameters[i].m_Name == "dv_feature_flags") {
            dv_feature_flags = (AP4_UI32)strtol(parameters[i].m_Value.GetChars(), NULL, 0);
            if (dv_feature_flags > 0x3FF) {
                fprintf(stderr, "ERROR: invalid dv_feature_flags %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
        }
    }

    //if not set format, set the default as 'avc1'.
    if (!format) {
        if (dv_profile == 32) {
            format = AP4_SAMPLE_FORMAT_DAVC;
        } else {
            format = AP4_SAMPLE_FORMAT_AVC1;
        }
    }

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // allocate an array to keep track of sample order
    AP4_Array<SampleOrder> sample_orders;

    // parse the input
    AP4_AvcFrameParser parser;

    if (format == AP4_SAMPLE_FORMAT_AVC3 || format == AP4_SAMPLE_FORMAT_DVAV) {
        parser.SetParameterControl(true);
    } else if (format == AP4_SAMPLE_FORMAT_AVC1 || format == AP4_SAMPLE_FORMAT_DVA1) {
        parser.SetParameterControl(false);
    }

    for (;;) {
        bool eos;
        unsigned char input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        bool     found_access_unit = false;
        do {
            AP4_AvcFrameParser::AccessUnitInfo access_unit_info;
    
            found_access_unit = false;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset],
                                 bytes_in_buffer,
                                 bytes_consumed,
                                 access_unit_info,
                                 eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
            if (access_unit_info.nal_units.ItemCount()) {
                // we got one access unit
                found_access_unit = true;
                if (g_Verbose) {
                    printf("H264 Access Unit, %d NAL units, decode_order=%d, display_order=%d\n",
                           access_unit_info.nal_units.ItemCount(),
                           access_unit_info.decode_order,
                           access_unit_info.display_order);
                }
                
                // compute the total size of the sample data
                unsigned int sample_data_size = 0;
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
                }

                // store the sample data
                AP4_Position position = 0;
                sample_storage.GetStream()->Tell(position);
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_storage.GetStream()->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
                    sample_storage.GetStream()->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
                }

                // add the sample to the track
                sample_table->AddSample(*sample_storage.GetStream(), position, sample_data_size, 1000, 0, 0, 0, access_unit_info.is_idr);

                // remember the sample order
                sample_orders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));

                // free the memory buffers
                access_unit_info.Reset();
            }

            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer || found_access_unit);
        if (eos) break;
    }

    // adjust the sample CTS/DTS offsets based on the sample orders
    if (sample_orders.ItemCount() > 1) {
        unsigned int start = 0;
        for (unsigned int i=1; i<=sample_orders.ItemCount(); i++) {
            if (i == sample_orders.ItemCount() || sample_orders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&sample_orders[start], i-start);
                start = i;
            }
        }
    }
    unsigned int max_delta = 0;
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        if (sample_orders[i].m_DecodeOrder > i) {
            unsigned int delta =sample_orders[i].m_DecodeOrder-i;
            if (delta > max_delta) {
                max_delta = delta;
            }
        }
    }
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        sample_table->UseSample(sample_orders[i].m_DecodeOrder).SetCts(1000ULL*(AP4_UI64)(i+max_delta));
    }

    // check the video parameters
    AP4_AvcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps = parser.GetSequenceParameterSets()[i];
            break;
        }
    }
    if (sps == NULL) {
        fprintf(stderr, "ERROR: no sequence parameter set found in video\n");
        input->Release();
        return;
    }
    unsigned int video_width = 0;
    unsigned int video_height = 0;
    sps->GetInfo(video_width, video_height);
    if (g_Verbose) {
        printf("VIDEO: %dx%d\n", video_width, video_height);
    }
    // Compute the video frame rate based on the timing info in the SPS and the slice header
    const AP4_AvcSliceHeader* slice_header = parser.GetSliceHeader();
    if (sps->timing_info_present_flag) {
        frame_rate = (double)(sps->time_scale) / (double)(sps->num_units_in_tick);
        frame_rate = frame_rate / (double)ComputeDeltaDivisor(sps->pic_struct_present_flag, slice_header->field_pic_flag, sps->pic_struct);
    } else {
        fprintf(stderr, "WARNING: no timing info in SPS, use user specified frame rate %.2f\n", user_frame_rate);
        frame_rate = user_frame_rate;
    }
    frame_rate = AP4_NormalizeFrameRate(frame_rate);
    // compute the dolby vision level
    ComputeDoviLevel(video_width, video_height, frame_rate, dv_level);
    if (user_frame_rate != 0.0 && AP4_FrameRatesDiffer(user_frame_rate, frame_rate)) {
        fprintf(stderr, "WARN: user specified frame rate %.2f does not match the frame rate in the SPS (%.2f)\n", user_frame_rate, frame_rate);
        frame_rate = user_frame_rate;
    }
    if (frame_rate == 0.0 && user_frame_rate == 0.0) {
        fprintf(stderr, "ERROR: no timing info in SPS, cannot determine frame rate. Please specify the frame rate manually.\n");
        exit(1);
    }
    // collect the SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_AVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps_array.Append(parser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_AVC_PPS_MAX_ID; i++) {
        if (parser.GetPictureParameterSets()[i]) {
            pps_array.Append(parser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }

    // setup the video the sample descripton
    AP4_AvcDoviSampleDescription* sample_description =
        new AP4_AvcDoviSampleDescription(format,
                                     video_width,
                                     video_height,
                                     24,
                                     "DOVI Coding",
                                     sps->profile_idc,
                                     sps->level_idc,
                                     sps->constraint_set0_flag<<7 |
                                     sps->constraint_set1_flag<<6 |
                                     sps->constraint_set2_flag<<5 |
                                     sps->constraint_set3_flag<<4,
                                     4,
                                     sps_array,
                                     pps_array,
                                     sps->chroma_format_idc,
                                     sps->bit_depth_luma_minus8,
                                     sps->bit_depth_chroma_minus8,
                                     dv_major_version,
                                     dv_minor_version,
                                     dv_profile,
                                     dv_level,
                                     dv_rpu_flag,
                                     dv_el_flag,
                                     dv_bl_flag,
                                     dv_bl_signal_comp_id,
                                     dv_md_compression,
                                     dv_feature_flags);
    sample_table->AddSampleDescription(sample_description);

    AP4_UI32 movie_timescale      = 1000;
    AP4_UI32 media_timescale      = AP4_FrameRateToTimeScale(frame_rate);
    AP4_UI64 video_track_duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie_timescale);
    AP4_UI64 video_media_duration = 1000*sample_table->GetSampleCount();

    // create a video track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                     sample_table,
                                     0,                    // auto-select track id
                                     movie_timescale,      // movie time scale
                                     video_track_duration, // track duration
                                     media_timescale,     // media time scale
                                     video_media_duration, // media duration
                                     language,              // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );
    // Using edit list to compensate the inital cts offset
    if(max_delta) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        AP4_UI32 edts_timescale = media_timescale;
        if(!movie.GetTimeScale()) {
            duration = video_media_duration;
        } else {
            duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie.GetTimeScale());
            edts_timescale = movie.GetTimeScale();
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, max_delta*1000ULL, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->SetEditList(new_edts, edts_timescale);
    }
    // update the brands list
    brands.Append(format);

    // cleanup
    input->Release();

    movie.AddTrack(track);

    ApplyTrackParams(track, parameters);
}

/*----------------------------------------------------------------------
|   AddH265Track
+---------------------------------------------------------------------*/
static void
AddH265Track(AP4_Movie&            movie,
             const char*           input_name,
             AP4_Array<Parameter>& parameters,
             AP4_Array<AP4_UI32>&  brands,
             SampleFileStorage&    sample_storage)
{
    unsigned int video_width = 0;
    unsigned int video_height = 0;
    AP4_UI32     format = AP4_SAMPLE_FORMAT_HVC1;
    
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // see if the frame rate is specified
    double user_frame_rate = 0.0;
    bool add_user_sei = false;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "frame_rate") {
            double frame_rate = atof(parameters[i].m_Value.GetChars());
            frame_rate = AP4_NormalizeFrameRate(frame_rate);
            if (frame_rate == 0.0) {
                fprintf(stderr, "ERROR: invalid video frame rate %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
            user_frame_rate = frame_rate;
        } else if (parameters[i].m_Name == "format") {
            if (parameters[i].m_Value == "hev1") {
                format = AP4_SAMPLE_FORMAT_HEV1;
            } else if (parameters[i].m_Value == "hvc1") {
                format = AP4_SAMPLE_FORMAT_HVC1;
            }
        } else if (parameters[i].m_Name == "user_sei") {
            add_user_sei = (AP4_UI08)strtol(parameters[i].m_Value.GetChars(), NULL, 0);
        }
    }
    
    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // allocate an array to keep track of sample order
    AP4_Array<SampleOrder> sample_orders;
    
    // parse the input
    AP4_HevcFrameParser parser;
    if (format == AP4_SAMPLE_FORMAT_HEV1) {
        parser.SetParameterControl(true);
    } else if (format == AP4_SAMPLE_FORMAT_HVC1) {
        parser.SetParameterControl(false);
    }
    for (;;) {
        bool eos;
        unsigned char input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        bool     found_access_unit = false;
        do {
            AP4_HevcFrameParser::AccessUnitInfo access_unit_info;
            
            found_access_unit = false;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset],
                                 bytes_in_buffer,
                                 bytes_consumed,
                                 access_unit_info,
                                 eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
            if (access_unit_info.nal_units.ItemCount()) {
                // we got one access unit
                found_access_unit = true;
                if (g_Verbose) {
                    printf("H265 Access Unit, %d NAL units, decode_order=%d, display_order=%d\n",
                           access_unit_info.nal_units.ItemCount(),
                           access_unit_info.decode_order,
                           access_unit_info.display_order);
                }
                
                // compute the total size of the sample data
                unsigned int sample_data_size = 0;
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
                }
                
                // store the sample data
                AP4_Position position = 0;
                sample_storage.GetStream()->Tell(position);
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    sample_storage.GetStream()->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
                    sample_storage.GetStream()->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
                }
                
                // add the sample to the track
                sample_table->AddSample(*sample_storage.GetStream(), position, sample_data_size, 1000, 0, 0, 0, access_unit_info.is_random_access);
            
                // remember the sample order
                sample_orders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));
                
                // free the memory buffers
                access_unit_info.Reset();
            }
        
            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer || found_access_unit);
        if (eos) break;
    }
    
    // adjust the sample CTS/DTS offsets based on the sample orders
    if (sample_orders.ItemCount() > 1) {
        unsigned int start = 0;
        for (unsigned int i=1; i<=sample_orders.ItemCount(); i++) {
            if (i == sample_orders.ItemCount() || sample_orders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&sample_orders[start], i-start);
                start = i;
            }
        }
    }
    unsigned int max_delta = 0;
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        if (sample_orders[i].m_DecodeOrder > i) {
            unsigned int delta =sample_orders[i].m_DecodeOrder-i;
            if (delta > max_delta) {
                max_delta = delta;
            }
        }
    }
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        sample_table->UseSample(sample_orders[i].m_DecodeOrder).SetCts(1000ULL*(AP4_UI64)(i+max_delta));
    }
    
    // check that we have at least one SPS
    AP4_HevcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        sps = parser.GetSequenceParameterSets()[i];
        if (sps) break;
    }
    if (sps == NULL) {
        fprintf(stderr, "ERROR: no sequence parameter set found in video\n");
        input->Release();
        return;
    }
    
    // collect parameters from the first SPS entry
    // TODO: synthesize from multiple SPS entries if we have more than one
    AP4_UI08 general_profile_space =               sps->profile_tier_level.general_profile_space;
    AP4_UI08 general_tier_flag =                   sps->profile_tier_level.general_tier_flag;
    AP4_UI08 general_profile =                     sps->profile_tier_level.general_profile_idc;
    AP4_UI32 general_profile_compatibility_flags = sps->profile_tier_level.general_profile_compatibility_flags;
    AP4_UI64 general_constraint_indicator_flags =  sps->profile_tier_level.general_constraint_indicator_flags;
    AP4_UI08 general_level =                       sps->profile_tier_level.general_level_idc;
    AP4_UI32 min_spatial_segmentation =            0; // TBD (should read from VUI if present)
    AP4_UI08 parallelism_type =                    0; // unknown
    AP4_UI08 chroma_format =                       sps->chroma_format_idc;
    AP4_UI08 luma_bit_depth =                      8; // hardcoded temporarily, should be read from the bitstream
    AP4_UI08 chroma_bit_depth =                    8; // hardcoded temporarily, should be read from the bitstream
    AP4_UI16 average_frame_rate =                  0; // unknown
    AP4_UI08 constant_frame_rate =                 0; // unknown
    AP4_UI08 num_temporal_layers =                 0; // unknown
    AP4_UI08 temporal_id_nested =                  0; // unknown
    AP4_UI08 nalu_length_size =                    4;

    sps->GetInfo(video_width, video_height);
    if (g_Verbose) {
        printf("VIDEO: %dx%d\n", video_width, video_height);
    }
    // calculate the frame rate from the VPS and SPS timing info
    AP4_HevcVideoParameterSet* vps = NULL;
    for (unsigned int i=0; i<=AP4_HEVC_VPS_MAX_ID; i++) {
        vps = parser.GetVideoParameterSets()[i];
        if (vps) break;
    }
    unsigned int vps_num_units = 0;
    unsigned int time_scale = 0;
    double frame_rate = 0.0;
    vps->GetTimeScaleInfo(time_scale, vps_num_units);
    if(vps_num_units > 0)
    {
        frame_rate = (double)time_scale/(double)vps_num_units;
    } else {
        // When vps_num_units_in_tick is present in the VPS referred to by the SPS, vui_num_units_in_tick, when present, shall be equal to vps_num_units_in_tick, and when not present, is inferred to be equal to vps_num_units_in_tick.
        unsigned int sps_num_units = 0;
        sps->GetTimeScaleInfo(time_scale, sps_num_units);
        if (sps_num_units) {
            frame_rate = (double)time_scale/(double)sps_num_units;
        } else {
            fprintf(stderr, "WARNING: no timing info in VPS/SPS, use user specified frame rate %.2f\n", user_frame_rate);
            frame_rate = user_frame_rate;
        }
    }
    frame_rate = AP4_NormalizeFrameRate(frame_rate);
    if (user_frame_rate != 0.0 && AP4_FrameRatesDiffer(user_frame_rate, frame_rate)) {
        fprintf(stderr, "WARNING: user specified frame rate %.2f does not match the frame rate %.2f in the bitstream\n", user_frame_rate, frame_rate);
        frame_rate = user_frame_rate;
    }
    if (user_frame_rate == 0 && frame_rate == 0) {
        fprintf(stderr, "ERROR: no timing info in VPS/SPS, cannot determine frame rate. Please specify the frame rate manually.\n");
        exit(1);
    }
    // collect the VPS, SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> vps_array;
    for (unsigned int i=0; i<=AP4_HEVC_VPS_MAX_ID; i++) {
        if (parser.GetVideoParameterSets()[i]) {
            vps_array.Append(parser.GetVideoParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps_array.Append(parser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_HEVC_PPS_MAX_ID; i++) {
        if (parser.GetPictureParameterSets()[i]) {
            pps_array.Append(parser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> user_array;
    if (add_user_sei) {
        AP4_HevcSEIMessage* user_sei = parser.GetSeiMessage(SEI_USER_DATA_UNREGISTERED);
        if (user_sei) {
            user_array.Append(user_sei->raw_bytes);
        }
    }
    AP4_DataBuffer three_dimension_sei(0);
    // setup the video the sample descripton
    AP4_UI08 parameters_completeness = (format == AP4_SAMPLE_FORMAT_HVC1 ? 1 : 0);
    AP4_HevcSampleDescription* sample_description =
        new AP4_HevcSampleDescription(format,
                                      video_width,
                                      video_height,
                                      24,
                                      "HEVC Coding",
                                      general_profile_space,
                                      general_tier_flag,
                                      general_profile,
                                      general_profile_compatibility_flags,
                                      general_constraint_indicator_flags,
                                      general_level,
                                      min_spatial_segmentation,
                                      parallelism_type,
                                      chroma_format,
                                      luma_bit_depth,
                                      chroma_bit_depth,
                                      average_frame_rate,
                                      constant_frame_rate,
                                      num_temporal_layers,
                                      temporal_id_nested,
                                      nalu_length_size,
                                      vps_array,
                                      parameters_completeness,
                                      sps_array,
                                      parameters_completeness,
                                      pps_array,
                                      parameters_completeness,
                                      user_array,
                                      three_dimension_sei);
    
    sample_table->AddSampleDescription(sample_description);
    
    AP4_UI32 movie_timescale      = 1000;
    AP4_UI32 media_timescale      = AP4_FrameRateToTimeScale(frame_rate);
    AP4_UI64 video_track_duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie_timescale);
    AP4_UI64 video_media_duration = 1000*sample_table->GetSampleCount();

    // create a video track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                     sample_table,
                                     0,                    // auto-select track id
                                     movie_timescale,      // movie time scale
                                     video_track_duration, // track duration
                                     media_timescale,     // media time scale
                                     video_media_duration, // media duration
                                     language,             // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );

    // Use an edit list to compensate for the inital cts offset
    if (max_delta) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        AP4_UI32 edts_timescale = media_timescale;
        if(!movie.GetTimeScale()) {
            duration = video_media_duration;
        } else {
            duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), media_timescale, movie.GetTimeScale());
            edts_timescale = movie.GetTimeScale();
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, max_delta*1000ULL, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->SetEditList(new_edts, edts_timescale);
    }
    // update the brands list
    brands.Append(AP4_FILE_BRAND_HVC1);

    // cleanup
    input->Release();

    movie.AddTrack(track);

    ApplyTrackParams(track, parameters);
}

/*----------------------------------------------------------------------
|   AddH265DoviTrack
+---------------------------------------------------------------------*/
static void
AddH265DoviTrack(AP4_Movie&        movie,
             const char*           input_name,
             AP4_Array<Parameter>& parameters,
             AP4_Array<AP4_UI32>&  brands,
             SampleFileStorage&    sample_storage)
{
    AP4_UI32 video_width = 0;
    AP4_UI32 video_height = 0;

    AP4_UI32 time_scale = 0;

    //based on the Dovi iso spec, set the following values to const 
    AP4_UI32 dv_major_version = 1;
    const AP4_UI32 dv_minor_version = 0;
    const bool         dv_rpu_flag = 1;
    const bool         dv_el_flag = 0;
    const bool         dv_bl_flag = 1;

    AP4_UI32 dv_profile = 0;
    AP4_UI32 dv_bl_signal_comp_id = 0;
    AP4_UI32 dv_level = 0;
    AP4_UI32 dv_md_compression = 0;
    AP4_UI32 dv_feature_flags = 0;
    AP4_UI32 format = 0;
    
    AP4_ByteStream* input;
    AP4_Result result = AP4_FileByteStream::Create(input_name, AP4_FileByteStream::STREAM_MODE_READ, input);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file '%s' (%d))\n", input_name, result);
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, "und");
    if (!language) return;

    // see if the frame rate/format/dv_profile/dv_bc is specified
    double user_frame_rate = 0.0;
    bool add_user_sei = false;
    bool set_vexu = true;
    int hero_eye = 1;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "frame_rate") {
            double frame_rate = atof(parameters[i].m_Value.GetChars());
            frame_rate = AP4_NormalizeFrameRate(frame_rate);
            if (frame_rate == 0.0) {
                fprintf(stderr, "ERROR: invalid video frame rate %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
            user_frame_rate = frame_rate;
        } else if (parameters[i].m_Name == "format") {
            if (parameters[i].m_Value == "hvc1") {
                format = AP4_SAMPLE_FORMAT_HVC1;
            } else if (parameters[i].m_Value == "hev1") {
                format = AP4_SAMPLE_FORMAT_HEV1;
            } else if (parameters[i].m_Value == "dvh1") {
                format = AP4_SAMPLE_FORMAT_DVH1;
            } else if (parameters[i].m_Value == "dvhe") {
                format = AP4_SAMPLE_FORMAT_DVHE;
            } else if (parameters[i].m_Value == "dvh8") {
                format = AP4_SAMPLE_FORMAT_DVH8;
            }
        } else if (parameters[i].m_Name == "dv_profile") {
            dv_profile = atoi(parameters[i].m_Value.GetChars());
        } else if (parameters[i].m_Name == "dv_bc") {
            dv_bl_signal_comp_id = atoi(parameters[i].m_Value.GetChars());
        } else if (parameters[i].m_Name == "dv_md_compression") {
            dv_md_compression = atoi(parameters[i].m_Value.GetChars());
        } else if (parameters[i].m_Name == "dv_feature_flags") {
            dv_feature_flags = (AP4_UI32)strtol(parameters[i].m_Value.GetChars(), NULL, 0);
            if (dv_feature_flags > 0x3FF) {
                fprintf(stderr, "ERROR: invalid dv_feature_flags %s\n", parameters[i].m_Value.GetChars());
                input->Release();
                return;
            }
        } else if (parameters[i].m_Name == "user_sei") {
            add_user_sei = (AP4_UI08)strtol(parameters[i].m_Value.GetChars(), NULL, 0);
        } else if (parameters[i].m_Name == "set_vexu") {
            set_vexu = (AP4_UI08)strtol(parameters[i].m_Value.GetChars(), NULL, 0);
        } else if (parameters[i].m_Name == "hero_eye") {
            hero_eye = (AP4_UI08)strtol(parameters[i].m_Value.GetChars(), NULL, 0);
        }
    }
    
    //if not set format, set the default one based on profile 
    if (!format) {
        if (dv_profile == 5) {
            format = AP4_SAMPLE_FORMAT_DVH1;
        } else if (dv_profile == 8) {
            if (dv_bl_signal_comp_id) {
                format = AP4_SAMPLE_FORMAT_HVC1;
            } else {
                format = AP4_SAMPLE_FORMAT_DVH1;
            }
        } else if (dv_profile == 34) {
            format = AP4_SAMPLE_FORMAT_DVH8;
        } else if (dv_profile == 20) {
            if (dv_bl_signal_comp_id) {
                format = AP4_SAMPLE_FORMAT_HVC1;
            } else {
                format = AP4_SAMPLE_FORMAT_DVH1;
            }
        }
    }

    // create a sample table
    AP4_SyntheticSampleTable* sample_table = new AP4_SyntheticSampleTable();

    // allocate an array to keep track of sample order
    AP4_Array<SampleOrder> sample_orders;
    
    // parse the input
    AP4_HevcFrameParser parser;
    if (format == AP4_SAMPLE_FORMAT_HEV1 || format == AP4_SAMPLE_FORMAT_DVHE) {
        parser.SetParameterControl(true);
    } else if (format == AP4_SAMPLE_FORMAT_HVC1 || format == AP4_SAMPLE_FORMAT_DVH1) {
        parser.SetParameterControl(false);
    }
    for (;;) {
        bool eos;
        unsigned char input_buffer[AP4_MUX_READ_BUFFER_SIZE];
        AP4_Size bytes_in_buffer = 0;
        result = input->ReadPartial(input_buffer, sizeof(input_buffer), bytes_in_buffer);
        if (AP4_SUCCEEDED(result)) {
            eos = false;
        } else if (result == AP4_ERROR_EOS) {
            eos = true;
        } else {
            fprintf(stderr, "ERROR: failed to read from input file\n");
            break;
        }
        AP4_Size offset = 0;
        bool     found_access_unit = false;
        do {
            AP4_HevcFrameParser::AccessUnitInfo access_unit_info;
            
            found_access_unit = false;
            AP4_Size bytes_consumed = 0;
            result = parser.Feed(&input_buffer[offset],
                                 bytes_in_buffer,
                                 bytes_consumed,
                                 access_unit_info,
                                 eos);
            if (AP4_FAILED(result)) {
                fprintf(stderr, "ERROR: Feed() failed (%d)\n", result);
                break;
            }
            if (access_unit_info.nal_units.ItemCount()) {
                // we got one access unit
                found_access_unit = true;
                if (g_Verbose) {
                    printf("H265 Access Unit, %d NAL units, decode_order=%d, display_order=%d\n",
                           access_unit_info.nal_units.ItemCount(),
                           access_unit_info.decode_order,
                           access_unit_info.display_order);
                }
                
                // compute the total size of the sample data
                unsigned int sample_data_size = 0;
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    // 4 -> start code length
                    sample_data_size += 4+access_unit_info.nal_units[i]->GetDataSize();
                }
                
                // store the sample data
                AP4_Position position = 0;
                sample_storage.GetStream()->Tell(position);
                for (unsigned int i=0; i<access_unit_info.nal_units.ItemCount(); i++) {
                    // length prefix
                    sample_storage.GetStream()->WriteUI32(access_unit_info.nal_units[i]->GetDataSize());
                    sample_storage.GetStream()->Write(access_unit_info.nal_units[i]->GetData(), access_unit_info.nal_units[i]->GetDataSize());
                }
                
                // add the sample to the track
                sample_table->AddSample(*sample_storage.GetStream(), position, sample_data_size, 1000, 0, 0, 0, access_unit_info.is_random_access);
            
                // remember the sample order
                sample_orders.Append(SampleOrder(access_unit_info.decode_order, access_unit_info.display_order));
                
                // free the memory buffers
                access_unit_info.Reset();
            }
        
            offset += bytes_consumed;
            bytes_in_buffer -= bytes_consumed;
        } while (bytes_in_buffer || found_access_unit);
        if (eos) break;
    }
    
    // adjust the sample CTS/DTS offsets based on the sample orders
    if (sample_orders.ItemCount() > 1) {
        unsigned int start = 0;
        for (unsigned int i=1; i<=sample_orders.ItemCount(); i++) {
            if (i == sample_orders.ItemCount() || sample_orders[i].m_DisplayOrder == 0) {
                // we got to the end of the GOP, sort it by display order
                SortSamples(&sample_orders[start], i-start);
                start = i;
            }
        }
    }
    unsigned int max_delta = 0;
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        if (sample_orders[i].m_DecodeOrder > i) {
            unsigned int delta =sample_orders[i].m_DecodeOrder-i;
            if (delta > max_delta) {
                max_delta = delta;
            }
        }
    }
    for (unsigned int i=0; i<sample_orders.ItemCount(); i++) {
        sample_table->UseSample(sample_orders[i].m_DecodeOrder).SetCts(1000ULL*(AP4_UI64)(i+max_delta));
    }
    
    // check that we have at least one SPS
    AP4_HevcSequenceParameterSet* sps = NULL;
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        sps = parser.GetSequenceParameterSets()[i];
        if (sps) break;
    }
    if (sps == NULL) {
        fprintf(stderr, "ERROR: no sequence parameter set found in video\n");
        input->Release();
        return;
    }
    
    // collect parameters from the first SPS entry
    // TODO: synthesize from multiple SPS entries if we have more than one
    AP4_UI08 general_profile_space =               sps->profile_tier_level.general_profile_space;
    AP4_UI08 general_tier_flag =                   sps->profile_tier_level.general_tier_flag;
    AP4_UI08 general_profile =                     sps->profile_tier_level.general_profile_idc;
    AP4_UI32 general_profile_compatibility_flags = sps->profile_tier_level.general_profile_compatibility_flags;
    AP4_UI64 general_constraint_indicator_flags =  sps->profile_tier_level.general_constraint_indicator_flags;
    AP4_UI08 general_level =                       sps->profile_tier_level.general_level_idc;
    AP4_UI32 min_spatial_segmentation =            0; // TBD (should read from VUI if present)
    AP4_UI08 parallelism_type =                    0; // unknown
    AP4_UI08 chroma_format =                       sps->chroma_format_idc;
    AP4_UI08 luma_bit_depth =                      sps->bit_depth_luma_minus8 + 8;
    AP4_UI08 chroma_bit_depth =                    sps->bit_depth_chroma_minus8 + 8;
    AP4_UI16 average_frame_rate =                  0; // unknown
    AP4_UI08 constant_frame_rate =                 0; // unknown
    AP4_UI08 num_temporal_layers =                 0; // unknown
    AP4_UI08 temporal_id_nested =                  0; // unknown
    AP4_UI08 nalu_length_size =                    4;
    AP4_UI08 colour_primaries =                    sps->vui_parameters.colour_primaries;
    AP4_UI08 transfer_characteristics =            sps->vui_parameters.transfer_characteristics;
    AP4_UI08 matrix_coeffs =                       sps->vui_parameters.matrix_coeffs;
    AP4_UI08 video_full_range_flag =               sps->vui_parameters.video_full_range_flag;
    AP4_SampleAspectRatio sample_aspect_ratio =    sps->vui_parameters.GetSampleAspectRatio();
    
    sps->GetInfo(video_width, video_height);
    if (g_Verbose) {
        printf("VIDEO: %dx%d\n", video_width, video_height);
    }
    
    // collect the VPS, SPS and PPS into arrays
    AP4_Array<AP4_DataBuffer> vps_array;
    for (unsigned int i=0; i<=AP4_HEVC_VPS_MAX_ID; i++) {
        if (parser.GetVideoParameterSets()[i]) {
            vps_array.Append(parser.GetVideoParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> sps_array;
    for (unsigned int i=0; i<=AP4_HEVC_SPS_MAX_ID; i++) {
        if (parser.GetSequenceParameterSets()[i]) {
            sps_array.Append(parser.GetSequenceParameterSets()[i]->raw_bytes);
        }
    }
    AP4_Array<AP4_DataBuffer> pps_array;
    for (unsigned int i=0; i<=AP4_HEVC_PPS_MAX_ID; i++) {
        if (parser.GetPictureParameterSets()[i]) {
            pps_array.Append(parser.GetPictureParameterSets()[i]->raw_bytes);
        }
    }

    AP4_Array<AP4_DataBuffer> user_array;
    if (add_user_sei) {
        AP4_HevcSEIMessage* user_sei = parser.GetSeiMessage(SEI_USER_DATA_UNREGISTERED);
        if (user_sei) {
            user_array.Append(user_sei->raw_bytes);
        }
    }
    AP4_HevcSEIMessage* three_dimension_sei = parser.GetSeiMessage(SEI_3D_REFERENCE_DISPLAY);
    if (dv_profile == 20) {
        if (!three_dimension_sei) {
            fprintf(stderr, "WARNING: SEI_3D_REFERENCE_DISPLAY not found, dv_profile is 20, skip\n");
        }
    }
    AP4_HevcVideoParameterSet* vps = parser.GetVideoParameterSets()[0];
    unsigned int vps_num_units = 0;
    double frame_rate = 0.0;
    vps->GetTimeScaleInfo(time_scale, vps_num_units);
    if(vps_num_units > 0)
    {
        frame_rate = (double)time_scale/(double)vps_num_units;
    } else {
        // When vps_num_units_in_tick is present in the VPS referred to by the SPS, vui_num_units_in_tick, when present, shall be equal to vps_num_units_in_tick, and when not present, is inferred to be equal to vps_num_units_in_tick.
        unsigned int sps_num_units = 0;
        sps->GetTimeScaleInfo(time_scale, sps_num_units);
        if (sps_num_units) {
            frame_rate = (double)time_scale/(double)sps_num_units;
        } else {
            fprintf(stderr, "WARNING: no timing info in VPS/SPS , use user specified frame rate %.2f\n", user_frame_rate);
            frame_rate = user_frame_rate;
        }
    }
    frame_rate = AP4_NormalizeFrameRate(frame_rate);
    //set dolby vision level
    ComputeDoviLevel(video_width, video_height, frame_rate, dv_level);
    if (user_frame_rate != 0.0 && AP4_FrameRatesDiffer(user_frame_rate, frame_rate)) {
        fprintf(stderr, "WARNING: user specified frame rate %.2f does not match the frame rate %.2f in the bitstream\n", user_frame_rate, frame_rate);
        frame_rate = user_frame_rate;
    }
    if (user_frame_rate == 0 && frame_rate == 0) {
        fprintf(stderr, "ERROR: no timing info in VPS/SPS, cannot determine frame rate. Please specify the frame rate manually.\n");
        exit(1);
    } 

    char* input_mdcv = NULL;
    char* input_clli = NULL;
    char* input_amve = NULL;
    for (unsigned int i = 0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "mdcv") {
            input_mdcv = parameters[i].m_Value.UseChars();
        }
        else if (parameters[i].m_Name == "clli") {
            input_clli = parameters[i].m_Value.UseChars();
        }
        else if (parameters[i].m_Name == "amve") {
            input_amve = parameters[i].m_Value.UseChars();
        }
    }

    AP4_Array<AP4_HevcSEIMessage> sei_array;
    AP4_HevcSEIMessage* mdcv = parser.GetSeiMessage(SEI_MASTERING_DISPLAY_COLOR_VOLUME);
    if (mdcv == NULL) {
        mdcv = new AP4_HevcSEIMessage(SEI_MASTERING_DISPLAY_COLOR_VOLUME);
    }
    sei_array.Append(*mdcv);
    AP4_HevcSEIMessage* clli = parser.GetSeiMessage(SEI_LIGHT_LEVEL_INFORMATION);
    if (clli == NULL) {
        clli = new AP4_HevcSEIMessage(SEI_LIGHT_LEVEL_INFORMATION);
    }
    sei_array.Append(*clli);
    AP4_HevcSEIMessage* amve = parser.GetSeiMessage(SEI_AMBIENT_VIEWING_ENVIRONMENT);
    if (amve == NULL) {
        if (input_amve != NULL) {
            AP4_Array<AP4_UI32> amve_values;
            AssembleInputBoxPayload(input_amve, amve_values);
            amve = new AP4_HevcSEIMessage(SEI_AMBIENT_VIEWING_ENVIRONMENT, amve_values);
            sei_array.Append(*amve);
        }
    }
    else {
        sei_array.Append(*amve);
    }

    AP4_UI08 parameters_completeness = ((format == AP4_SAMPLE_FORMAT_HVC1 || format == AP4_SAMPLE_FORMAT_DVH1) ? 1 : 0);
    if (dv_profile == 20) {
        dv_major_version = 3; //  For profile 20, this value must be set to 3
        int i = 0;
        AP4_HevcPictureParameterSet* pps = parser.GetPictureParameterSets()[i];
        bool all_tiles_enabled = true;
        bool all_entropy_sync_enabled = true;
        bool all_flags_zero = true;
        while (pps) {
            if (pps->tiles_enabled_flag != 1) all_tiles_enabled = false;
            if (pps->entropy_coding_sync_enabled_flag != 1) all_entropy_sync_enabled = false;
            if (!(pps->tiles_enabled_flag == 0 && pps->entropy_coding_sync_enabled_flag == 0))
                all_flags_zero = false;
            i++;
            pps = parser.GetPictureParameterSets()[i];
        }
        if (all_flags_zero) {
            parallelism_type = 1; // slice-based
        } else if (all_tiles_enabled) {
            parallelism_type = 2; // tile-based
        } else if (all_entropy_sync_enabled) {
            parallelism_type = 3; // entropy coding sync
        } else {
            parallelism_type = 0; // mixed or unknown
        }
        AP4_HevcSequenceParameterSet* sps = parser.GetSequenceParameterSets()[0];
        if (sps) {
            min_spatial_segmentation = sps->vui_parameters.min_spatial_segmentation_idc;
        }
        AP4_HevcVideoParameterSet* vps = parser.GetVideoParameterSets()[0];
        if (vps) {
            num_temporal_layers = vps->vps_max_sub_layers_minus1 + 1;
            temporal_id_nested = vps->vps_temporal_id_nesting_flag;
        }
    }
    AP4_DataBuffer three_dimension_sei_buffer(0);
    if (three_dimension_sei) {
        three_dimension_sei_buffer.AppendData(three_dimension_sei->raw_bytes.GetData(), three_dimension_sei->raw_bytes.GetDataSize());
    }
    // setup the video the sample descripton
    AP4_HevcDoviSampleDescription* sample_description =
        new AP4_HevcDoviSampleDescription(format,
                                      parser,
                                      video_width,
                                      video_height,
                                      24,
                                      "DOVI Coding",
                                      general_profile_space,
                                      general_tier_flag,
                                      general_profile,
                                      general_profile_compatibility_flags,
                                      general_constraint_indicator_flags,
                                      general_level,
                                      min_spatial_segmentation,
                                      parallelism_type,
                                      chroma_format,
                                      luma_bit_depth,
                                      chroma_bit_depth,
                                      average_frame_rate,
                                      constant_frame_rate,
                                      num_temporal_layers,
                                      temporal_id_nested,
                                      nalu_length_size,
                                      vps_array,
                                      parameters_completeness,
                                      sps_array,
                                      parameters_completeness,
                                      pps_array,
                                      parameters_completeness,
                                      dv_major_version,
                                      dv_minor_version,
                                      dv_profile,
                                      dv_level,
                                      dv_rpu_flag,
                                      dv_el_flag,
                                      dv_bl_flag,
                                      dv_bl_signal_comp_id,
                                      dv_md_compression,
                                      dv_feature_flags,
                                      sei_array,
                                      colour_primaries,
                                      transfer_characteristics,
                                      matrix_coeffs,
                                      video_full_range_flag,
                                      sample_aspect_ratio.horizontal_size,
                                      sample_aspect_ratio.vertical_size,
                                      user_array,
                                      three_dimension_sei_buffer,
                                      set_vexu,
                                      hero_eye);
    
    sample_table->AddSampleDescription(sample_description);

    AP4_UI32 movie_timescale       = 1000;
    AP4_UI32 video_media_timescale = AP4_FrameRateToTimeScale(frame_rate);
    AP4_UI32 video_track_duration  = AP4_ConvertTime(1000 * sample_table->GetSampleCount(), video_media_timescale, movie_timescale);
    AP4_UI64 video_media_duration  = 1000 * sample_table->GetSampleCount();

    // create a video track
    AP4_Track* track = new AP4_Track(AP4_Track::TYPE_VIDEO,
                                     sample_table,
                                     0,                    // auto-select track id
                                     movie_timescale,      // movie time scale
                                     video_track_duration, // track duration
                                     video_media_timescale,// media time scale
                                     video_media_duration, // media duration
                                     language,             // language
                                     video_width<<16,      // width
                                     video_height<<16      // height
                                     );
    // Using edit list to compensate the inital cts offset
    if(max_delta) {
        // create an 'edts' container
        AP4_ContainerAtom* new_edts = new AP4_ContainerAtom(AP4_ATOM_TYPE_EDTS);
        AP4_ElstAtom* new_elst = new AP4_ElstAtom();
        AP4_UI64 duration = 0;
        AP4_UI32 edts_timescale = video_media_timescale;
        if(!movie.GetTimeScale()) {
            duration = video_media_duration;
        } else {
            duration = AP4_ConvertTime(1000*sample_table->GetSampleCount(), video_media_timescale, movie.GetTimeScale());
            edts_timescale = movie.GetTimeScale();
        }
        AP4_ElstEntry new_elst_entry = AP4_ElstEntry(duration, max_delta*1000ULL, 1);
        new_elst->AddEntry(new_elst_entry);
        new_edts->AddChild(new_elst);
        track->SetEditList(new_edts, edts_timescale);
    }
    // update the brands list
    brands.Append(format);
    if ((dv_profile == 8 || dv_profile == 20) && dv_bl_signal_comp_id == 4) {
        if (transfer_characteristics == 18) {
            brands.Append(AP4_FILE_BRAND_DB4H);
        } else if (transfer_characteristics == 14) {
            brands.Append(AP4_FILE_BRAND_DB4G);
        }
    }

    // cleanup
    input->Release();

    movie.AddTrack(track);

    ApplyTrackParams(track, parameters);
}

/*----------------------------------------------------------------------
|   AddMp4Tracks
+---------------------------------------------------------------------*/
static void
AddMp4Tracks(AP4_Movie&            movie,
             const char*           input_name,
             AP4_Array<Parameter>& parameters,
             AP4_Array<AP4_UI32>&  /*brands*/)
{
    // open the input
    AP4_ByteStream* input_stream = NULL;
    AP4_Result result = AP4_FileByteStream::Create(input_name,
                                                   AP4_FileByteStream::STREAM_MODE_READ, 
                                                   input_stream);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: cannot open input file %s (%d)\n", input_name, result);
        return;
    }
    
    AP4_File file(*input_stream, true);
    input_stream->Release();
    AP4_Movie* input_movie = file.GetMovie();
    if (input_movie == NULL) {
        return;
    }

    // check if we have a language parameter
    const char* language = GetLanguageFromParameters(parameters, NULL);

    // check the parameters to decide which track(s) to import
    unsigned int track_id = 0;
    for (unsigned int i=0; i<parameters.ItemCount(); i++) {
        if (parameters[i].m_Name == "track") {
            if (parameters[i].m_Value == "audio") {
                AP4_Track* track = input_movie->GetTrack(AP4_Track::TYPE_AUDIO);
                if (track == NULL) {
                    fprintf(stderr, "ERROR: no audio track found in %s\n", input_name);
                    return;
                } else {
                    track_id = track->GetId();
                }
            } else if (parameters[i].m_Value == "video") {
                AP4_Track* track = input_movie->GetTrack(AP4_Track::TYPE_VIDEO);
                if (track == NULL) {
                    fprintf(stderr, "ERROR: no video track found in %s\n", input_name);
                    return;
                } else {
                    track_id = track->GetId();
                }
            } else {
                track_id = (unsigned int)strtoul(parameters[i].m_Value.GetChars(), NULL, 10);
                if (track_id == 0) {
                    fprintf(stderr, "ERROR: invalid track ID specified");
                    return;
                }
            }
        }
    }
    
    if (g_Verbose) {
        if (track_id == 0) {
            printf("MP4 Import: importing all tracks from %s\n", input_name);
        } else {
            printf("MP4 Import: importing track %d from %s\n", track_id, input_name);
        }
    }
    
    AP4_List<AP4_Track>::Item* track_item = input_movie->GetTracks().FirstItem();
    while (track_item) {
        AP4_Track* track = track_item->GetData();
        if (track_id == 0 || track->GetId() == track_id) {
            track = track->Clone();

            // reset the track ID so that it can be re-assigned
            track->SetId(0);

            // override the language if specified in the parameters
            if (language) {
                track->SetTrackLanguage(language);
            }

            movie.AddTrack(track);

            // one 'pres' parameter per track
            if (g_Preselection) {
                g_Preselection->generateGrpl(input_name, track->GetId(), "mp4");
            }
            ApplyTrackParams(track, parameters);
        }
        track_item = track_item->GetNext();
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    if (argc < 2) {
        PrintUsageAndExit();
    }
    
    const char* output_filename = NULL;
    AP4_Array<char*> input_names;

    while (char* arg = *++argv) {
        if (!strcmp(arg, "--help")) {
            PrintUsageAndExit();
        } else if (!strcmp(arg, "--verbose")) {
            g_Verbose = true;
        } else if (!strcmp(arg, "--preselection")) {
            g_Preselection = new AP4_Preselection(*++argv);
        } else if (!strcmp(arg, "--track")) {
            input_names.Append(*++argv);
        } else if (output_filename == NULL) {
            output_filename = arg;
        } else {
            fprintf(stderr, "ERROR: unexpected argument '%s'\n", arg);
            return 1;
        }
    }

    if (input_names.ItemCount() == 0) {
        fprintf(stderr, "ERROR: no input\n");
        return 1;
    }
    if (output_filename == NULL) {
        fprintf(stderr, "ERROR: output filename missing\n");
        return 1;
    }

    // create the movie object to hold the tracks
    AP4_UI64 creation_time = 0;
    time_t now = time(NULL);
    if (now != (time_t)-1) {
        // adjust the time based on the MPEG time origin
        creation_time = (AP4_UI64)now + 0x7C25B080;
    }
    AP4_Movie* movie = new AP4_Movie(0, 0, creation_time, creation_time);

    // setup the brands
    AP4_Array<AP4_UI32> brands;
    brands.Append(AP4_FILE_BRAND_ISOM);
    brands.Append(AP4_FILE_BRAND_MP42);

    // create a temp file to store the sample data
    SampleFileStorage* sample_storage = NULL;
    AP4_Result result = SampleFileStorage::Create(output_filename, sample_storage);
    if (AP4_FAILED(result)) {
        fprintf(stderr, "ERROR: failed to create temporary sample data storage (%d)\n", result);
        return 1;
    }
    
    // add all the tracks
    bool hasDolby = false;
    AP4_UI08 dv_bc = 0;
    AP4_UI08 profiles = 0;
    for (unsigned int i=0; i<input_names.ItemCount(); i++) {
        char*       input_name = input_names[i];
        const char* input_type = NULL;
        char*       input_params = NULL;

        char* separator = strchr(input_name, ':');
        if (separator) {
            input_type = input_name;
            input_name = separator+1;
            *separator = '\0';
        }
        separator = strchr(input_name, '#');
        if (separator) {
            input_params = separator+1;
            *separator = '\0';
        }
        
        if (input_type == NULL) {
            // no type, try to guess
            separator = strrchr(input_name, '.');
            if (separator) {
                input_type = separator+1;
                for (unsigned int j=1; separator[j]; j++) {
                    separator[j] = (char)tolower(separator[j]);
                }
                if (!strcmp("264", input_type)) {
                    input_type = "h264";
                } else if (!strcmp("avc", input_type)) {
                    input_type = "h264";
                } else if (!strcmp("265", input_type)) {
                    input_type = "h265";
                } else if (!strcmp("hevc", input_type)) {
                    input_type = "h265";
                } else if (!strcmp("adts", input_type)) {
                    input_type = "aac";
                } else if (!strcmp("m4a", input_type) ||
                           !strcmp("m4v", input_type) ||
                           !strcmp("mov", input_type)) {
                    input_type = "aac";
                }
            } else {
                fprintf(stderr, "ERROR: unable to determine type for input '%s'\n", input_name);
                delete sample_storage;
                return 1;
            }
        } else {
            // extract file type and compare with input type
            separator = strrchr(input_name, '.');
            const char* file_type = NULL;
            if (separator) {
                file_type = separator+1;
                for (unsigned int j=1; separator[j]; j++) {
                    separator[j] = (char)tolower(separator[j]);
                }
                if (!strcmp("264", file_type)) {
                    file_type = "h264";
                } else if (!strcmp("avc", file_type)) {
                    file_type = "h264";
                } else if (!strcmp("265", file_type)) {
                    file_type = "h265";
                } else if (!strcmp("hevc", file_type)) {
                    file_type = "h265";
                } else if (!strcmp("adts", file_type)) {
                    file_type = "aac";
                } else if (!strcmp("m4a", file_type) ||
                           !strcmp("m4v", file_type) ||
                           !strcmp("mov", file_type)) {
                    file_type = "aac";
                }
                if (strcmp(file_type, input_type) != 0) {
                    fprintf(stderr, "ERROR: file type '%s' is not identical with input type '%s'\n", file_type, input_type);
                    delete sample_storage;
                    return 1;
                } 
            }
        }
        
        // parse parameters
        AP4_Array<Parameter> parameters;
        if (input_params) {
            ParseParameters(input_params, parameters);
        }

        bool isDovi = false;
        for (unsigned int i=0; i<parameters.ItemCount(); i++) {
            if (parameters[i].m_Name == "dv_profile") {
                isDovi = true;
                if (parameters[i].m_Value == "8"){
                    profiles = 8;
                } else if (parameters[i].m_Value == "10"){
                    profiles = 10;
                } else if (parameters[i].m_Value == "20"){
                    profiles = 20;
                }
            }
            if (parameters[i].m_Name == "dv_bc") {
                if (parameters[i].m_Value == "1") {
                    dv_bc |= 1;
                } else if (parameters[i].m_Value == "2") {
                    dv_bc |= 2;
                } else if (parameters[i].m_Value == "3") {
                    dv_bc |= 3;
                }
                else if (parameters[i].m_Value == "4") {
                    dv_bc |= 4;
                }
            }
        }

        if (!strcmp(input_type, "h264")) {
            if (isDovi) {
                if (CheckDoviInputParameters(parameters) != AP4_SUCCESS) {
                    fprintf(stderr, "ERROR: dolby vision input parameter error\n");
                    delete sample_storage;
                    return 1;
                } else {
                    AddH264DoviTrack(*movie, input_name, parameters, brands, *sample_storage);
                    hasDolby = true;
                }
            } else {
                AddH264Track(*movie, input_name, parameters, brands, *sample_storage);
            }
        } else if (!strcmp(input_type, "h265")) {
            if (isDovi) {
                if (CheckDoviInputParameters(parameters) != AP4_SUCCESS) {
                    fprintf(stderr, "ERROR: dolby vision input parameter error\n");
                    delete sample_storage;
                    return 1;
                } else {
                    AddH265DoviTrack(*movie, input_name, parameters, brands, *sample_storage);
                    hasDolby = true;
                }
            } else {
                AddH265Track(*movie, input_name, parameters, brands, *sample_storage);
            }
        } else if (!strcmp(input_type, "aac")) {
            AddAacTrack(*movie, input_name, parameters, *sample_storage);
        } else if (!strcmp(input_type, "ac3")) {
            AddAc3Track(*movie, input_name, parameters, *sample_storage);
            hasDolby = true;
        } else if (!strcmp(input_type, "ec3")) {
            AddEac3Track(*movie, input_name, parameters, *sample_storage);
            hasDolby = true;
        } else if (!strcmp(input_type, "ac4")) {
            AddAc4Track(*movie, input_name, parameters, *sample_storage);
            hasDolby = true;
        } else if (!strcmp(input_type, "mlp")) {
            AddMlpTrack(*movie, input_name, parameters, *sample_storage);
            hasDolby = true;
        }
        else if (!strcmp(input_type, "mp4")) {
            AddMp4Tracks(*movie, input_name, parameters, brands);
        } else {
            fprintf(stderr, "ERROR: unsupported input type '%s'\n", input_type);
            delete sample_storage;
            return 1;
        }
    }

    // for Dolby Content, add the 'dby1' brand
    if (hasDolby) {
        brands.Append(AP4_FILE_BRAND_DBY1);
    }
    if (dv_bc & 1) {
        brands.Append(AP4_FILE_BRAND_DB1P);
    }
    if (dv_bc & 2) {
        brands.Append(AP4_FILE_BRAND_DB2G);
    }

    // Declare UNIF brand if preselections are present
    if (g_Preselection) {
        brands.Append(AP4_FILE_BRAND_UNIF);
    }

    movie->GetMvhdAtom()->SetNextTrackId(movie->GetTracks().ItemCount() + 1);

    // open the output
    AP4_ByteStream* output = NULL;
    result = AP4_FileByteStream::Create(output_filename, AP4_FileByteStream::STREAM_MODE_WRITE, output);
    if (AP4_FAILED(result)) {
        AP4_Debug("ERROR: cannot open output '%s' (%d)\n", output_filename, result);
        delete sample_storage;
        return 1;
    }
    
    {
        // create a multimedia file
        AP4_File file(movie);

        // set the file type
        AP4_Array<AP4_UI32> unique_brands;
        for (unsigned int i = 0; i < brands.ItemCount(); i++) {
            bool found = false;
            for (unsigned int j = 0; j < unique_brands.ItemCount(); j++) {
                if (brands[i] == unique_brands[j]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                unique_brands.Append(brands[i]);
            }
        }
        file.SetFileType(AP4_FILE_BRAND_MP42, 1, &unique_brands[0], unique_brands.ItemCount());

        // Preselections: Adding Track Group Description, stored in top-level META atom, if any were created
        if (g_Preselection) {
            // Create META Atom in File
            AP4_ContainerAtom* meta = new AP4_ContainerAtom(AP4_ATOM_TYPE_META, (AP4_UI32)0, (AP4_UI32)0);
            file.AddChild(meta);

            // Create a NULL-Handler Atom
            AP4_HdlrAtom* hdlr = new AP4_HdlrAtom(AP4_HANDLER_TYPE_NULL, "NULL Handler");
            meta->AddChild(hdlr);

            // Add already filled GRPL as child to META
            meta->AddChild(g_Preselection->getGrpl());
        }

        // write the file to the output
        AP4_FileWriter::Write(file, *output);
    }
    
    // cleanup
    delete sample_storage;
    output->Release();
    
    return 0;
}
