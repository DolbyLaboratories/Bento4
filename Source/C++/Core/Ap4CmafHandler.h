/*****************************************************************
|
|    AP4 - Ap4CmafHandler 
|
|    Copyright 2002-2026 Axiomatic Systems, LLC
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
/**
* @file 
* @brief Ap4CmafHandler
*/

#ifndef _AP4_CMAF_COMPLIANCE_H_
#define _AP4_CMAF_COMPLIANCE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4Types.h"
#include "Ap4Atom.h"
#include "Ap4FtypAtom.h"
#include "Ap4ContainerAtom.h"
#include "Ap4Movie.h"

/*----------------------------------------------------------------------
|   AP4_CMAFHandler
+---------------------------------------------------------------------*/
class AP4_CMAFHandler {
private:
    struct Entry {
        AP4_UI32 trackID;
        AP4_UI08 ac4;
        AP4_UI08 encrypted;
        AP4_UI32 dv_profile;
        AP4_UI32 dv_compatibility_id;
    };
    AP4_List<Entry> m_Codecs;
    AP4_UI08 m_initialized;
    Entry* GetEntry(AP4_UI32 trackID);
    void AddBrand(AP4_FtypAtom *ftyp, AP4_Array<AP4_UI32>& brands, const AP4_UI32 b);

public:
    AP4_CMAFHandler();
    ~AP4_CMAFHandler();

    AP4_Result ApplyMoof(AP4_ContainerAtom* moof);
    AP4_Result ApplyFtyp(AP4_FtypAtom* ftyp);
    AP4_Result ApplyMoov(AP4_ContainerAtom* moov);
};

#endif // _AP4_CMAF_COMPLIANCE_H_