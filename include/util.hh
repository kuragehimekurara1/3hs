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

#ifndef inc_util_hh
#define inc_util_hh

#include <ui/base.hh>
#include <string>

class StatusLine : public ui::BaseWidget
{ UI_WIDGET("StatusLine")
public:
	void run(const std::string& str);
	void reset();

	bool render(ui::Keys&) override;
	float height() override { return 0.0f; }
	float width() override { return 0.0f; }

private:
	ui::ScopedWidget<ui::Text> text;
	time_t in_pos_start;
	float xpos, lastx;
	int flags;

};


/* returns if we were previously focussed */
bool set_focus(bool focus);
/* set the status line */
void set_status(const std::string& text);
void reset_status();
/* sets the action description and returns the old one */
std::string set_desc(const std::string& nlabel);
void lower(std::string& s);
void trim(std::string& str, const std::string& whitespace);
void join(std::string& ret, const std::vector<std::string>& tokens, const std::string& sep);

#endif

