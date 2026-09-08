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

 /*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4CmafHandler.h"
#include "Ap4Atom.h"
#include "Ap4Movie.h"
#include "Ap4File.h"
#include "Ap4SampleDescription.h"
#include "Ap4SampleEntry.h"
#include "Ap4Track.h"
#include "Ap4TkhdAtom.h"
#include "Ap4StsdAtom.h"
#include "Ap4StssAtom.h"
#include "Ap4TfhdAtom.h"
#include "Ap4TrunAtom.h"
#include <stdio.h>

/*----------------------------------------------------------------------
|   AP4_CMAFHandler::AP4_CMAFHandler
+---------------------------------------------------------------------*/
AP4_CMAFHandler::AP4_CMAFHandler()
{
    m_initialized = 0;
}

AP4_CMAFHandler::~AP4_CMAFHandler()
{
    for (AP4_List<Entry>::Item* item = m_Codecs.FirstItem(); item; item = item->GetNext()) {
        Entry* entry = item->GetData();
        delete entry;
    }
}

/*----------------------------------------------------------------------
|   AP4_CMAFHandler::Apply
+---------------------------------------------------------------------*/
void AP4_CMAFHandler::AddBrand(AP4_FtypAtom *ftyp, AP4_Array<AP4_UI32>& brands, const AP4_UI32 b)
{
    if (!ftyp->HasCompatibleBrand(b)) {
        brands.Append(b);
        ftyp->SetSize(ftyp->GetSize() + 4);
    }
}

AP4_Result AP4_CMAFHandler::ApplyFtyp(AP4_FtypAtom *ftyp)
{
    if (!ftyp) {
        fprintf(stderr, "Error: Cannot obtain the atom. Call the ApplyFtyp method with a valid ftyp atom.\n");
        return AP4_FAILURE;
    }

    if (m_initialized == 0) {
        fprintf(stderr, "Error: Cannot obtain track information. Call the ApplyMoov method before calling ApplyFtyp.\n");
        return AP4_FAILURE;
    }

    AP4_Array<AP4_UI32>& brands = ftyp->GetCompatibleBrands();

    // add cmf2
    AddBrand(ftyp, brands, AP4_FILE_BRAND_CMF2);

    for (AP4_List<Entry>::Item* item = m_Codecs.FirstItem(); item; item = item->GetNext()) {
        Entry* entry = item->GetData();

        if (entry->dv_profile) {
            // add dolby vision cmaf brands
            AddBrand(ftyp, brands, AP4_FILE_BRAND_DBY1);
            if (entry->dv_profile == 5) {
                AddBrand(ftyp, brands, AP4_FILE_BRAND_DV58);
            }
            else if (entry->dv_profile == 8) {
                AddBrand(ftyp, brands, AP4_FILE_BRAND_DV58);
                if (entry->dv_compatibility_id == 1) {
                    AddBrand(ftyp, brands, AP4_FILE_BRAND_DB1P);
                    AddBrand(ftyp, brands, AP4_FILE_BRAND_CHD1);
                }
                else if (entry->dv_compatibility_id == 4) {
                    AddBrand(ftyp, brands, AP4_FILE_BRAND_DB4H);
                    AddBrand(ftyp, brands, AP4_FILE_BRAND_CLG1);
                }
            }
            else if (entry->dv_profile == 9) {
                AddBrand(ftyp, brands, AP4_FILE_BRAND_DV09);
                if (entry->dv_compatibility_id == 2) {
                    AddBrand(ftyp, brands, AP4_FILE_BRAND_DB2G);
                    AddBrand(ftyp, brands, AP4_FILE_BRAND_CHDF);
                }
            }
            else if (entry->dv_profile == 20) {
                AddBrand(ftyp, brands, AP4_FILE_BRAND_DV20);
                if (entry->dv_compatibility_id == 4) {
                    AddBrand(ftyp, brands, AP4_FILE_BRAND_DB4H);
                    AddBrand(ftyp, brands, AP4_FILE_BRAND_CLG1);
                }
            }
        }
        if (entry->ac4) {
            // add ac4 cmaf brand
            AddBrand(ftyp, brands, AP4_FILE_BRAND_UNIF);
            AddBrand(ftyp, brands, AP4_FILE_BRAND_CA4S);
            AddBrand(ftyp, brands, AP4_FILE_BRAND_DBY1);

            // add ac4 subsample encryption cmaf brand
            if (entry->encrypted) {
                AddBrand(ftyp, brands, AP4_FILE_BRAND_CA4E);
            }
        }
    }

    return AP4_SUCCESS;
}


AP4_Result AP4_CMAFHandler::ApplyMoov(AP4_ContainerAtom *moov)
{
    if (!moov) {
        fprintf(stderr, "Error: Cannot obtain the atom. Call the ApplyMoov method with a valid moov atom.\n");
        return AP4_FAILURE;
    }

    for (AP4_List<AP4_Atom>::Item* item = moov->GetChildren().FirstItem();
                                item;
                                item = item->GetNext()) {
        AP4_ContainerAtom* moov_child = AP4_DYNAMIC_CAST(AP4_ContainerAtom, item->GetData());
        if (!moov_child) continue;

        if (moov_child->GetType() == AP4_ATOM_TYPE_TRAK) {
            AP4_ContainerAtom* trak = moov_child;

            Entry* entry = new Entry();
            entry->trackID = 0;
            entry->ac4 = 0;
            entry->encrypted = 0;
            entry->dv_profile = 0;
            entry->dv_compatibility_id = 0;
            m_Codecs.Add(entry);

            // earse edts atoms if exist
            AP4_ContainerAtom* edts = AP4_DYNAMIC_CAST(AP4_ContainerAtom, trak->FindChild("edts"));
            if (edts) {
                trak->RemoveChild(edts);
            }

            // set the duration to 0 in the tkhd atom
            AP4_TkhdAtom* tkhd = AP4_DYNAMIC_CAST(AP4_TkhdAtom, trak->FindChild("tkhd"));
            if (tkhd) {
                tkhd->SetDuration(0);
                entry->trackID = tkhd->GetTrackId();
            }
            
            AP4_StsdAtom* stsd = AP4_DYNAMIC_CAST(AP4_StsdAtom, trak->FindChild("mdia/minf/stbl/stsd"));
            if (!stsd) continue;

            for (AP4_UI32 i = 0; i < stsd->GetSampleDescriptionCount(); i++) {
                AP4_SampleDescription* description = stsd->GetSampleDescription(i);
                AP4_SampleEntry* sample_entry = stsd->GetSampleEntry(i);
                if (!description) continue;

                if (AP4_DYNAMIC_CAST(AP4_VideoSampleDescription, description)) {
                    AP4_DvccAtom* dvcc = AP4_DYNAMIC_CAST(AP4_DvccAtom, description->GetDetails().GetChild(AP4_ATOM_TYPE_DVCC));
                    if(!dvcc) {
                        dvcc = AP4_DYNAMIC_CAST(AP4_DvccAtom, description->GetDetails().GetChild(AP4_ATOM_TYPE_DVVC));
                    }
                    if (dvcc) {
                        entry->dv_profile = dvcc->GetDvProfile();
                        entry->dv_compatibility_id = dvcc->GetDvBlSignalCompatibilityID();
                    }

                    // add pasp and colr box if the trak is video and does not have them
                    if (sample_entry) {
                        AP4_PaspAtom* pasp = AP4_DYNAMIC_CAST(AP4_PaspAtom, sample_entry->GetChild(AP4_ATOM_TYPE_PASP));
                        if (!pasp) {
                            AP4_PaspAtom* new_pasp = new AP4_PaspAtom(1, 1);
                            sample_entry->AddChild(new_pasp);
                        }
                        AP4_ColrAtom* colr = AP4_DYNAMIC_CAST(AP4_ColrAtom, sample_entry->GetChild(AP4_ATOM_TYPE_COLR));
                        if (!colr) {
                            fprintf(stderr, "Warning: No colr atom found in video track, adding a default colr atom with NCLX colorinformation.\n");
                            AP4_ColrAtom* new_colr = new AP4_ColrAtom(AP4_SAMPLE_COLOR_TYPE_NCLX, 2, 2, 2, 0);
                            sample_entry->AddChild(new_colr);
                        }
                    }
                }
                if (description->GetType() == AP4_SampleDescription::TYPE_AC4) {
                    entry->ac4 = 1;
                }
                if (description->GetFormat() == AP4_ATOM_TYPE_ENCA || description->GetFormat() == AP4_ATOM_TYPE_ENCV) {
                    entry->encrypted = 1;
                }
            }

        }
        else if (moov_child->GetType() == AP4_ATOM_TYPE_MVHD) {
            // set the duration to 0 in the mvhd atom
            AP4_MvhdAtom* mvhd = AP4_DYNAMIC_CAST(AP4_MvhdAtom, moov_child);
            if (mvhd) {
                mvhd->SetDuration(0);
            }
        }
    }

    m_initialized = 1;

    return AP4_SUCCESS;
}

AP4_Result AP4_CMAFHandler::ApplyMoof(AP4_ContainerAtom *moof) {
    if (!moof) {
        fprintf(stderr, "Error: Cannot obtain the atom. Call the ApplyMoof method with a valid moof atom.\n");
        return AP4_FAILURE;
    }

    if (m_initialized == 0) {
        fprintf(stderr, "Error: Cannot obtain track information. Call the ApplyMoov method before calling ApplyMoof.\n");
        return AP4_FAILURE;
    }

    for (AP4_List<AP4_Atom>::Item* item = moof->GetChildren().FirstItem();
                                item;
                                item = item->GetNext()) {
        AP4_ContainerAtom* moof_child = AP4_DYNAMIC_CAST(AP4_ContainerAtom, item->GetData());
        if (!moof_child) continue;

        if (moof_child->GetType() == AP4_ATOM_TYPE_TRAF) {
            AP4_ContainerAtom* traf = moof_child;

            AP4_TfhdAtom* tfhd = AP4_DYNAMIC_CAST(AP4_TfhdAtom, traf->FindChild("tfhd"));
            if (!tfhd) continue;

            AP4_UI32 trackID = tfhd->GetTrackId();
            Entry* entry = GetEntry(trackID);
            if (!entry) continue;

            // set sample_has_redundancy to 2 for AC-4 samples to indicate that the sample has no redundant coding
            if (entry->ac4) {
                for (AP4_List<AP4_Atom>::Item* traf_child_item = traf->GetChildren().FirstItem();
                                        traf_child_item;
                                        traf_child_item = traf_child_item->GetNext()) {
                    AP4_Atom* traf_child = traf_child_item->GetData();
                    if (!traf_child) continue;

                    if (traf_child->GetType() == AP4_ATOM_TYPE_TRUN) {
                        AP4_TrunAtom* trun = AP4_DYNAMIC_CAST(AP4_TrunAtom, traf_child);

                        if (trun && trun->GetFlags() & 0x000400) { // sample-flags-present
                            AP4_Array<AP4_TrunAtom::Entry>& entries = trun->UseEntries();
                            for (AP4_UI32 i = 0; i < entries.ItemCount(); i++) {
                                entries[i].sample_flags &= 0xFFCFFFFF; // clear the sample_has_redundancy bits
                                entries[i].sample_flags |= 0x00200000;
                            }
                        }
                    }
                }
            }
        }
    }

    return AP4_SUCCESS;
}

/*----------------------------------------------------------------------
|   AP4_CMAFHandler::GetEntry
+---------------------------------------------------------------------*/
AP4_CMAFHandler::Entry* AP4_CMAFHandler::GetEntry(AP4_UI32 trackID) {
    for (AP4_List<Entry>::Item* item = m_Codecs.FirstItem(); item; item = item->GetNext()) {
        Entry* entry = item->GetData();
        if (entry && entry->trackID == trackID) {
            return entry;
        }
    }
    return NULL;
}