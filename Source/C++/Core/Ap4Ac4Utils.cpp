/*****************************************************************
|
|    AP4 - AC-4 Utilities
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
#include "Ap4Ac4Utils.h"
#include <ctype.h>
#include <string.h>

/*----------------------------------------------------------------------
|   AP4_Ac4MapIso6391ToIso6392T
+---------------------------------------------------------------------*/
static const char*
AP4_Ac4MapIso6391ToIso6392T(const char language[2])
{
    struct LanguageMapEntry {
        char iso_639_1[2];
        char iso_639_2[3];
    };
    static const LanguageMapEntry language_map[] = {
        {{'a','a'}, {'a','a','r'}}, {{'a','b'}, {'a','b','k'}}, {{'a','e'}, {'a','v','e'}}, {{'a','f'}, {'a','f','r'}},
        {{'a','k'}, {'a','k','a'}}, {{'a','m'}, {'a','m','h'}}, {{'a','n'}, {'a','r','g'}}, {{'a','r'}, {'a','r','a'}},
        {{'a','s'}, {'a','s','m'}}, {{'a','v'}, {'a','v','a'}}, {{'a','y'}, {'a','y','m'}}, {{'a','z'}, {'a','z','e'}},
        {{'b','a'}, {'b','a','k'}}, {{'b','e'}, {'b','e','l'}}, {{'b','g'}, {'b','u','l'}}, {{'b','h'}, {'b','i','h'}},
        {{'b','i'}, {'b','i','s'}}, {{'b','m'}, {'b','a','m'}}, {{'b','n'}, {'b','e','n'}}, {{'b','o'}, {'b','o','d'}},
        {{'b','r'}, {'b','r','e'}}, {{'b','s'}, {'b','o','s'}}, {{'c','a'}, {'c','a','t'}}, {{'c','e'}, {'c','h','e'}},
        {{'c','h'}, {'c','h','a'}}, {{'c','o'}, {'c','o','s'}}, {{'c','r'}, {'c','r','e'}}, {{'c','s'}, {'c','e','s'}},
        {{'c','u'}, {'c','h','u'}}, {{'c','v'}, {'c','h','v'}}, {{'c','y'}, {'c','y','m'}}, {{'d','a'}, {'d','a','n'}},
        {{'d','e'}, {'d','e','u'}}, {{'d','v'}, {'d','i','v'}}, {{'d','z'}, {'d','z','o'}}, {{'e','e'}, {'e','w','e'}},
        {{'e','l'}, {'e','l','l'}}, {{'e','n'}, {'e','n','g'}}, {{'e','o'}, {'e','p','o'}}, {{'e','s'}, {'s','p','a'}},
        {{'e','t'}, {'e','s','t'}}, {{'e','u'}, {'e','u','s'}}, {{'f','a'}, {'f','a','s'}}, {{'f','f'}, {'f','u','l'}},
        {{'f','i'}, {'f','i','n'}}, {{'f','j'}, {'f','i','j'}}, {{'f','o'}, {'f','a','o'}}, {{'f','r'}, {'f','r','a'}},
        {{'f','y'}, {'f','r','y'}}, {{'g','a'}, {'g','l','e'}}, {{'g','d'}, {'g','l','a'}}, {{'g','l'}, {'g','l','g'}},
        {{'g','n'}, {'g','r','n'}}, {{'g','u'}, {'g','u','j'}}, {{'g','v'}, {'g','l','v'}}, {{'h','a'}, {'h','a','u'}},
        {{'h','e'}, {'h','e','b'}}, {{'h','i'}, {'h','i','n'}}, {{'h','o'}, {'h','m','o'}}, {{'h','r'}, {'h','r','v'}},
        {{'h','t'}, {'h','a','t'}}, {{'h','u'}, {'h','u','n'}}, {{'h','y'}, {'h','y','e'}}, {{'h','z'}, {'h','e','r'}},
        {{'i','a'}, {'i','n','a'}}, {{'i','d'}, {'i','n','d'}}, {{'i','e'}, {'i','l','e'}}, {{'i','g'}, {'i','b','o'}},
        {{'i','i'}, {'i','i','i'}}, {{'i','k'}, {'i','p','k'}}, {{'i','o'}, {'i','d','o'}}, {{'i','s'}, {'i','s','l'}},
        {{'i','t'}, {'i','t','a'}}, {{'i','u'}, {'i','k','u'}}, {{'j','a'}, {'j','p','n'}}, {{'j','v'}, {'j','a','v'}},
        {{'k','a'}, {'k','a','t'}}, {{'k','g'}, {'k','o','n'}}, {{'k','i'}, {'k','i','k'}}, {{'k','j'}, {'k','u','a'}},
        {{'k','k'}, {'k','a','z'}}, {{'k','l'}, {'k','a','l'}}, {{'k','m'}, {'k','h','m'}}, {{'k','n'}, {'k','a','n'}},
        {{'k','o'}, {'k','o','r'}}, {{'k','r'}, {'k','a','u'}}, {{'k','s'}, {'k','a','s'}}, {{'k','u'}, {'k','u','r'}},
        {{'k','v'}, {'k','o','m'}}, {{'k','w'}, {'c','o','r'}}, {{'k','y'}, {'k','i','r'}}, {{'l','a'}, {'l','a','t'}},
        {{'l','b'}, {'l','t','z'}}, {{'l','g'}, {'l','u','g'}}, {{'l','i'}, {'l','i','m'}}, {{'l','n'}, {'l','i','n'}},
        {{'l','o'}, {'l','a','o'}}, {{'l','t'}, {'l','i','t'}}, {{'l','u'}, {'l','u','b'}}, {{'l','v'}, {'l','a','v'}},
        {{'m','g'}, {'m','l','g'}}, {{'m','h'}, {'m','a','h'}}, {{'m','i'}, {'m','r','i'}}, {{'m','k'}, {'m','k','d'}},
        {{'m','l'}, {'m','a','l'}}, {{'m','n'}, {'m','o','n'}}, {{'m','r'}, {'m','a','r'}}, {{'m','s'}, {'m','s','a'}},
        {{'m','t'}, {'m','l','t'}}, {{'m','y'}, {'m','y','a'}}, {{'n','a'}, {'n','a','u'}}, {{'n','b'}, {'n','o','b'}},
        {{'n','d'}, {'n','d','e'}}, {{'n','e'}, {'n','e','p'}}, {{'n','g'}, {'n','d','o'}}, {{'n','l'}, {'n','l','d'}},
        {{'n','n'}, {'n','n','o'}}, {{'n','o'}, {'n','o','r'}}, {{'n','r'}, {'n','b','l'}}, {{'n','v'}, {'n','a','v'}},
        {{'n','y'}, {'n','y','a'}}, {{'o','c'}, {'o','c','i'}}, {{'o','j'}, {'o','j','i'}}, {{'o','m'}, {'o','r','m'}},
        {{'o','r'}, {'o','r','i'}}, {{'o','s'}, {'o','s','s'}}, {{'p','a'}, {'p','a','n'}}, {{'p','i'}, {'p','l','i'}},
        {{'p','l'}, {'p','o','l'}}, {{'p','s'}, {'p','u','s'}}, {{'p','t'}, {'p','o','r'}}, {{'q','u'}, {'q','u','e'}},
        {{'r','m'}, {'r','o','h'}}, {{'r','n'}, {'r','u','n'}}, {{'r','o'}, {'r','o','n'}}, {{'r','u'}, {'r','u','s'}},
        {{'r','w'}, {'k','i','n'}}, {{'s','a'}, {'s','a','n'}}, {{'s','c'}, {'s','r','d'}}, {{'s','d'}, {'s','n','d'}},
        {{'s','e'}, {'s','m','e'}}, {{'s','g'}, {'s','a','g'}}, {{'s','i'}, {'s','i','n'}}, {{'s','k'}, {'s','l','k'}},
        {{'s','l'}, {'s','l','v'}}, {{'s','m'}, {'s','m','o'}}, {{'s','n'}, {'s','n','a'}}, {{'s','o'}, {'s','o','m'}},
        {{'s','q'}, {'s','q','i'}}, {{'s','r'}, {'s','r','p'}}, {{'s','s'}, {'s','s','w'}}, {{'s','t'}, {'s','o','t'}},
        {{'s','u'}, {'s','u','n'}}, {{'s','v'}, {'s','w','e'}}, {{'s','w'}, {'s','w','a'}}, {{'t','a'}, {'t','a','m'}},
        {{'t','e'}, {'t','e','l'}}, {{'t','g'}, {'t','g','k'}}, {{'t','h'}, {'t','h','a'}}, {{'t','i'}, {'t','i','r'}},
        {{'t','k'}, {'t','u','k'}}, {{'t','l'}, {'t','g','l'}}, {{'t','n'}, {'t','s','n'}}, {{'t','o'}, {'t','o','n'}},
        {{'t','r'}, {'t','u','r'}}, {{'t','s'}, {'t','s','o'}}, {{'t','t'}, {'t','a','t'}}, {{'t','w'}, {'t','w','i'}},
        {{'t','y'}, {'t','a','h'}}, {{'u','g'}, {'u','i','g'}}, {{'u','k'}, {'u','k','r'}}, {{'u','r'}, {'u','r','d'}},
        {{'u','z'}, {'u','z','b'}}, {{'v','e'}, {'v','e','n'}}, {{'v','i'}, {'v','i','e'}}, {{'v','o'}, {'v','o','l'}},
        {{'w','a'}, {'w','l','n'}}, {{'w','o'}, {'w','o','l'}}, {{'x','h'}, {'x','h','o'}}, {{'y','i'}, {'y','i','d'}},
        {{'y','o'}, {'y','o','r'}}, {{'z','a'}, {'z','h','a'}}, {{'z','h'}, {'z','h','o'}}, {{'z','u'}, {'z','u','l'}}
    };

    for (unsigned int i = 0; i < sizeof(language_map)/sizeof(language_map[0]); i++) {
        if (language_map[i].iso_639_1[0] == language[0] &&
            language_map[i].iso_639_1[1] == language[1]) {
            return language_map[i].iso_639_2;
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   AP4_Ac4ConvertLanguageTagToMdhdLanguage
+---------------------------------------------------------------------*/
bool
AP4_Ac4ConvertLanguageTagToMdhdLanguage(const AP4_UI08* language_tag,
                                        AP4_UI08        language_tag_size,
                                        char            language[4])
{
    char primary_language[3];
    unsigned int primary_language_size = 0;
    for (unsigned int i = 0; i < language_tag_size; i++) {
        char c = (char)language_tag[i];
        if (c == '-' || c == '_') {
            break;
        }
        if (primary_language_size == sizeof(primary_language)) {
            return false;
        }
        if (!isalpha((unsigned char)c)) {
            return false;
        }
        primary_language[primary_language_size++] = (char)tolower((unsigned char)c);
    }

    if (primary_language_size == 2) {
        const char* mapped_language = AP4_Ac4MapIso6391ToIso6392T(primary_language);
        if (!mapped_language) {
            return false;
        }
        memcpy(language, mapped_language, 3);
        language[3] = '\0';
        return true;
    }

    if (primary_language_size == 3) {
        memcpy(language, primary_language, 3);
        language[3] = '\0';
        return true;
    }

    return false;
}

AP4_UI32
AP4_Ac4VariableBits(AP4_BitReader &data, int nBits)
{
    AP4_UI32 value = 0;
    AP4_UI32 b_moreBits;
    do{
        value += data.ReadBits(nBits);
        b_moreBits = data.ReadBit();
        if (b_moreBits == 1) {
            value <<= nBits;
            value += (1<<nBits);
      }
    } while (b_moreBits == 1);
    return value;
}

AP4_Result
AP4_Ac4ChannelCountFromSpeakerGroupIndexMask(unsigned int speakerGroupIndexMask)
{

    unsigned int channelCount= 0;
    if ((speakerGroupIndexMask & 1) != 0) { // 0: L,R 0b1
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 2) != 0) { // 1: C 0b10
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 4) != 0) { // 2: Ls,Rs 0b100
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 8) != 0) { // 3: Lb,Rb 0b1000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 16) != 0) { // 4: Tfl,Tfr 0b10000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 32) != 0) { // 5: Tbl,Tbr 0b100000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 64) != 0) { // 6: LFE 0b1000000
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 128) != 0) { // 7: TL,TR 0b10000000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 256) != 0) { // 8: Tsl,Tsr 0b100000000
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 512) != 0) { // 9: Tfc 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 1024) != 0) { // 10: Tbc 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 2048) != 0) { // 11: Tc 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 4096) != 0) { // 12: LFE2 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 8192) != 0) { // 13: Bfl,Bfr 
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 16384) != 0) { // 14: Bfc 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 32768) != 0) { // 15: Cb 
        channelCount += 1;
    }
    if ((speakerGroupIndexMask & 65536) != 0) { // 16: Lscr,Rscr 
        channelCount += 2; 
    }
    if ((speakerGroupIndexMask & 131072) != 0) { // 17: Lw,Rw 
        channelCount += 2;
    } 
    if ((speakerGroupIndexMask & 262144) != 0) { // 18: Vhl,Vhr 
        channelCount += 2;
    }
    if ((speakerGroupIndexMask & 1) != 0 && (speakerGroupIndexMask & 2) != 0 &&
        channelCount == 3) {
        channelCount = 2;
    }
    return channelCount;
}
