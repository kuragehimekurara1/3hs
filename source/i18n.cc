/* This file is part of 3hs
 * Copyright (C) 2021-2022 hShop developer team
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <ui/base.hh> /* for UI_GLYPH_* */

#include "settings.hh"
#include "panic.hh"
#include "i18n.hh"

#include <3ds.h>

#include "i18n_tab.cc"

const char *i18n::getstr(str::type id)
{
	I18NStringStore *store = RAW(get_nsettings()->lang, id);
	panic_assert(!(store->info & INFO_ISFUNC), "attempt to get parameter-less function");
	return store->string;
}

const char *i18n::getstr_param(str::type id, const std::vector<std::string>& params)
{
	I18NStringStore *store = RAW(get_nsettings()->lang, id);
	if(store->info & INFO_ISFUNC)
		return store->function(params);
	else
		return store->string;
}

const char *i18n::getsurestr(str::type sid)
{
	ensure_settings();
	return i18n::getstr(sid);
}

// https://www.3dbrew.org/wiki/Country_Code_List
//  only took over those that we actually use
namespace CountryCode
{
	enum _codes {
		canada         =  18,
		greece         =  79,
		hungary        =  80,
		latvia         =  84,
		poland         =  97,
		romania        =  99,
		spain          = 105,
		united_kingdom = 110,
	};
}

// Not documented so gotten through a test application
namespace ProvinceCode
{
	enum _codes {
		uk_wales      =  5,
		sp_catalonia  = 11,
		japan_osaka   = 28,
		japan_okinawa = 48,
	};
}

lang::type i18n::default_lang()
{
	u8 syslang = 0;
	u8 countryinfo[4];
	if(R_FAILED(CFGU_GetSystemLanguage(&syslang)))
		syslang = CFG_LANGUAGE_EN;
	/* countryinfo */
	if(R_FAILED(CFGU_GetConfigInfoBlk2(4, 0x000B0000, countryinfo)))
	{
		countryinfo[2] = 0;
		countryinfo[3] = 0;
	}

	switch(countryinfo[3])
	{
	case CountryCode::hungary: return lang::hungarian;
	case CountryCode::romania: return lang::romanian;
	case CountryCode::latvia: return lang::latvian;
	case CountryCode::poland: return lang::polish;
	case CountryCode::greece: return lang::greek;
	case CountryCode::spain:
		if(countryinfo[2] == ProvinceCode::sp_catalonia)
			return lang::catalan;
		break;
	case CountryCode::united_kingdom:
		if(countryinfo[2] == ProvinceCode::uk_wales)
			return lang::welsh;
		break;
	}

	switch(syslang)
	{
	case CFG_LANGUAGE_JP:
		switch(countryinfo[2])
		{
		case ProvinceCode::japan_okinawa: return lang::ryukyuan;
		case ProvinceCode::japan_osaka: return lang::jp_osaka;
		}
		return lang::japanese;
	case CFG_LANGUAGE_FR:
		return countryinfo[3] == CountryCode::canada
			? lang::fr_canada : lang::french;
	case CFG_LANGUAGE_DE: return lang::german;
	case CFG_LANGUAGE_IT: return lang::italian;
	case CFG_LANGUAGE_ES: return lang::spanish;
	case CFG_LANGUAGE_ZH: return lang::schinese;
	case CFG_LANGUAGE_KO: return lang::korean;
	case CFG_LANGUAGE_NL: return lang::dutch;
	case CFG_LANGUAGE_PT: return lang::portuguese;
	case CFG_LANGUAGE_RU: return lang::russian;
	case CFG_LANGUAGE_TW: return lang::tchinese;
	case CFG_LANGUAGE_EN: // fallthrough
	default: return lang::english;
	}
}

