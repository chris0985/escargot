/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#ifndef __ICUPolyfill__
#define __ICUPolyfill__

#include "RuntimeICUBinder.h"

#define u_getVersion RuntimeICUBinder::ICU::instance().u_getVersion

#define u_tolower RuntimeICUBinder::ICU::instance().u_tolower
#define u_toupper RuntimeICUBinder::ICU::instance().u_toupper
#define u_islower RuntimeICUBinder::ICU::instance().u_islower
#define u_isupper RuntimeICUBinder::ICU::instance().u_isupper
#define u_totitle RuntimeICUBinder::ICU::instance().u_totitle
#define u_charMirror RuntimeICUBinder::ICU::instance().u_charMirror
#define u_countChar32 RuntimeICUBinder::ICU::instance().u_countChar32

#define uloc_getDefault RuntimeICUBinder::ICU::instance().uloc_getDefault
#define uloc_getName RuntimeICUBinder::ICU::instance().uloc_getName
#define uloc_canonicalize RuntimeICUBinder::ICU::instance().uloc_canonicalize
#define uloc_getBaseName RuntimeICUBinder::ICU::instance().uloc_getBaseName

#define u_getIntPropertyValue RuntimeICUBinder::ICU::instance().u_getIntPropertyValue
#define u_getIntPropertyMaxValue RuntimeICUBinder::ICU::instance().u_getIntPropertyMaxValue
#define u_getIntPropertyMinValue RuntimeICUBinder::ICU::instance().u_getIntPropertyMinValue
#define u_charType RuntimeICUBinder::ICU::instance().u_charType
#define u_getCombiningClass RuntimeICUBinder::ICU::instance().u_getCombiningClass
#define u_charDirection RuntimeICUBinder::ICU::instance().u_charDirection
#define u_isblank RuntimeICUBinder::ICU::instance().u_isblank

#define unorm2_getNFCInstance RuntimeICUBinder::ICU::instance().unorm2_getNFCInstance
#define unorm2_getNFDInstance RuntimeICUBinder::ICU::instance().unorm2_getNFDInstance
#define unorm2_getNFKCInstance RuntimeICUBinder::ICU::instance().unorm2_getNFKCInstance
#define unorm2_getNFKDInstance RuntimeICUBinder::ICU::instance().unorm2_getNFKDInstance
#define unorm2_normalize RuntimeICUBinder::ICU::instance().unorm2_normalize
#define unorm2_composePair RuntimeICUBinder::ICU::instance().unorm2_composePair
#define unorm2_getRawDecomposition RuntimeICUBinder::ICU::instance().unorm2_getRawDecomposition

#define vzone_openID RuntimeICUBinder::ICU::instance().vzone_openID
#define vzone_getTZURL RuntimeICUBinder::ICU::instance().vzone_getTZURL
#define vzone_getRawOffset RuntimeICUBinder::ICU::instance().vzone_getRawOffset
#define vzone_getOffset3 RuntimeICUBinder::ICU::instance().vzone_getOffset3
#define vzone_close RuntimeICUBinder::ICU::instance().vzone_close

#define ucol_countAvailable RuntimeICUBinder::ICU::instance().ucol_countAvailable
#define ucol_getAvailable RuntimeICUBinder::ICU::instance().ucol_getAvailable
#define ucol_setAttribute RuntimeICUBinder::ICU::instance().ucol_setAttribute
#define ucol_close RuntimeICUBinder::ICU::instance().ucol_close
#define ucol_getKeywordValuesForLocale RuntimeICUBinder::ICU::instance().ucol_getKeywordValuesForLocale
#define ucol_open RuntimeICUBinder::ICU::instance().ucol_open
#define ucol_strcollIter RuntimeICUBinder::ICU::instance().ucol_strcollIter

#define unum_countAvailable RuntimeICUBinder::ICU::instance().unum_countAvailable
#define unum_getAvailable RuntimeICUBinder::ICU::instance().unum_getAvailable
#define unum_setTextAttribute RuntimeICUBinder::ICU::instance().unum_setTextAttribute
#define unum_setAttribute RuntimeICUBinder::ICU::instance().unum_setAttribute
#define unum_open RuntimeICUBinder::ICU::instance().unum_open
#define unum_formatDouble RuntimeICUBinder::ICU::instance().unum_formatDouble
#define unum_close RuntimeICUBinder::ICU::instance().unum_close

#define udat_getAvailable RuntimeICUBinder::ICU::instance().udat_getAvailable
#define udat_countAvailable RuntimeICUBinder::ICU::instance().udat_countAvailable

#define udat_open RuntimeICUBinder::ICU::instance().udat_open
#define udat_format RuntimeICUBinder::ICU::instance().udat_format
#define udat_close RuntimeICUBinder::ICU::instance().udat_close
#define udat_getCalendar RuntimeICUBinder::ICU::instance().udat_getCalendar

#define uenum_count RuntimeICUBinder::ICU::instance().uenum_count
#define uenum_unext RuntimeICUBinder::ICU::instance().uenum_unext
#define uenum_next RuntimeICUBinder::ICU::instance().uenum_next
#define uenum_reset RuntimeICUBinder::ICU::instance().uenum_reset
#define uenum_close RuntimeICUBinder::ICU::instance().uenum_close

#define unumsys_openAvailableNames RuntimeICUBinder::ICU::instance().unumsys_openAvailableNames
#define unumsys_openByName RuntimeICUBinder::ICU::instance().unumsys_openByName
#define unumsys_close RuntimeICUBinder::ICU::instance().unumsys_close
#define unumsys_isAlgorithmic RuntimeICUBinder::ICU::instance().unumsys_isAlgorithmic
#define unumsys_getName RuntimeICUBinder::ICU::instance().unumsys_getName
#define unumsys_open RuntimeICUBinder::ICU::instance().unumsys_open

#define uiter_setString RuntimeICUBinder::ICU::instance().uiter_setString

#define ucal_getKeywordValuesForLocale RuntimeICUBinder::ICU::instance().ucal_getKeywordValuesForLocale
#define ucal_openTimeZones RuntimeICUBinder::ICU::instance().ucal_openTimeZones
#define ucal_getCanonicalTimeZoneID RuntimeICUBinder::ICU::instance().ucal_getCanonicalTimeZoneID
#define ucal_getType RuntimeICUBinder::ICU::instance().ucal_getType
#define ucal_setGregorianChange RuntimeICUBinder::ICU::instance().ucal_setGregorianChange

#define udatpg_close RuntimeICUBinder::ICU::instance().udatpg_close
#define udatpg_open RuntimeICUBinder::ICU::instance().udatpg_open
#define udatpg_getBestPattern RuntimeICUBinder::ICU::instance().udatpg_getBestPattern

#define ubrk_open RuntimeICUBinder::ICU::instance().ubrk_open
#define ubrk_openRules RuntimeICUBinder::ICU::instance().ubrk_openRules
#define ubrk_next RuntimeICUBinder::ICU::instance().ubrk_next
#define ubrk_setText RuntimeICUBinder::ICU::instance().ubrk_setText
#define ubrk_setUText RuntimeICUBinder::ICU::instance().ubrk_setUText
#define ubrk_close RuntimeICUBinder::ICU::instance().ubrk_close

#define ucnv_open RuntimeICUBinder::ICU::instance().ucnv_open
#define ucnv_compareNames RuntimeICUBinder::ICU::instance().ucnv_compareNames
#define ucnv_close RuntimeICUBinder::ICU::instance().ucnv_close
#define ucnv_toUnicode RuntimeICUBinder::ICU::instance().ucnv_toUnicode
#define ucnv_getDisplayName RuntimeICUBinder::ICU::instance().ucnv_getDisplayName
#define ucnv_getName RuntimeICUBinder::ICU::instance().ucnv_getName

#define ucsdet_open RuntimeICUBinder::ICU::instance().ucsdet_open
#define ucsdet_detectAll RuntimeICUBinder::ICU::instance().ucsdet_detectAll
#define ucsdet_detect RuntimeICUBinder::ICU::instance().ucsdet_detect
#define ucsdet_getName RuntimeICUBinder::ICU::instance().ucsdet_getName
#define ucsdet_getConfidence RuntimeICUBinder::ICU::instance().ucsdet_getConfidence
#define ucsdet_setText RuntimeICUBinder::ICU::instance().ucsdet_setText
#define ucsdet_close RuntimeICUBinder::ICU::instance().ucsdet_close

#define ubidi_open RuntimeICUBinder::ICU::instance().ubidi_open
#define ubidi_getBaseDirection RuntimeICUBinder::ICU::instance().ubidi_getBaseDirection
#define ubidi_countRuns RuntimeICUBinder::ICU::instance().ubidi_countRuns
#define ubidi_close RuntimeICUBinder::ICU::instance().ubidi_close
#define ubidi_setPara RuntimeICUBinder::ICU::instance().ubidi_setPara
#define ubidi_getLogicalRun RuntimeICUBinder::ICU::instance().ubidi_getLogicalRun

#define ublock_getCode RuntimeICUBinder::ICU::instance().ublock_getCode

#define uscript_getScript RuntimeICUBinder::ICU::instance().uscript_getScript
#define uscript_hasScript RuntimeICUBinder::ICU::instance().uscript_hasScript
#define uscript_getShortName RuntimeICUBinder::ICU::instance().uscript_getShortName

#endif
