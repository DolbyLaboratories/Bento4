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

#ifndef _AP4_VEXU_ATOM_H_
#define _AP4_VEXU_ATOM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Ap4ContainerAtom.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const AP4_UI32 AP4_ATOM_TYPE_VEXU = AP4_ATOM_TYPE('v','e','x','u');
const AP4_UI32 AP4_ATOM_TYPE_EYES = AP4_ATOM_TYPE('e','y','e','s');
const AP4_UI32 AP4_ATOM_TYPE_STRI = AP4_ATOM_TYPE('s','t','r','i');
const AP4_UI32 AP4_ATOM_TYPE_HERO = AP4_ATOM_TYPE('h','e','r','o');

/*----------------------------------------------------------------------
|   AP4_StriAtom - Stereo View Information Atom
+---------------------------------------------------------------------*/
class AP4_StriAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_StriAtom, AP4_Atom)

    // class methods
    static AP4_StriAtom* Create(AP4_Size size, AP4_ByteStream& stream);
    
    // constructors
    AP4_StriAtom();

    // methods
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);
    virtual AP4_Result InspectFields(AP4_AtomInspector& inspector);

private:
    // methods
    AP4_StriAtom(AP4_UI32 size, AP4_ByteStream& stream);
    
    // members
    AP4_UI08 m_Reserved;
    AP4_UI08 m_EyeViewsReversed;
    AP4_UI08 m_HasAdditionalViews;
    AP4_UI08 m_HasRightEyeView;
    AP4_UI08 m_HasLeftEyeView;
};

/*----------------------------------------------------------------------
|   AP4_HeroAtom - Hero Stereo Eye Description Atom
+---------------------------------------------------------------------*/
class AP4_HeroAtom : public AP4_Atom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_HeroAtom, AP4_Atom)

    // class methods
    static AP4_HeroAtom* Create(AP4_Size size, AP4_ByteStream& stream);
    
    // constructors
    AP4_HeroAtom();
    AP4_HeroAtom(AP4_UI08 hero_eye_indicator);

    // methods
    virtual AP4_Result WriteFields(AP4_ByteStream& stream);

    // accessors
    AP4_UI08 GetHeroEyeIndicator() const { return m_HeroEyeIndicator; }

private:
    // methods
    AP4_HeroAtom(AP4_UI32 size, AP4_ByteStream& stream);
    
    // members
    AP4_UI08 m_HeroEyeIndicator;
};

/*----------------------------------------------------------------------
|   AP4_EyesAtom - Eyes Container Atom
+---------------------------------------------------------------------*/
class AP4_EyesAtom : public AP4_ContainerAtom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_EyesAtom, AP4_ContainerAtom)

    // constructors
    AP4_EyesAtom();
    AP4_EyesAtom(AP4_UI08 hero_eye);
    AP4_Result WriteFields(AP4_ByteStream& stream);
private:
    AP4_StriAtom* m_StriAtom;
    AP4_HeroAtom* m_HeroAtom;
};

/*----------------------------------------------------------------------
|   AP4_VexuAtom - Video Extended Usage Container Atom
+---------------------------------------------------------------------*/
class AP4_VexuAtom : public AP4_ContainerAtom
{
public:
    AP4_IMPLEMENT_DYNAMIC_CAST_D(AP4_VexuAtom, AP4_ContainerAtom)

    // constructors
    AP4_VexuAtom();
    AP4_VexuAtom(AP4_UI08 hero_eye);
    // methods
    AP4_Result WriteFields(AP4_ByteStream& stream);
private:
    AP4_UI08 m_HeroEye;
    AP4_EyesAtom* m_EyesAtom;
};

#endif // _AP4_VEXU_ATOM_H_