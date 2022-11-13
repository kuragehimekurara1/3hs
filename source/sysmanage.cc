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

#include "sysmanage.hh"

#include <ui/confirm.hh>
#include <ui/loading.hh>
#include <ui/base.hh>

#include "panic.hh"
#include "log.hh"
#include "ctr.hh"

#include <algorithm>

void show_delete_unused_tickets()
{
	std::vector<u64> title_tids, ticket_tids, unused_ticket_tids;

	ui::loading([&title_tids, &ticket_tids, &unused_ticket_tids]() -> void {
		panic_if_err_3ds(ctr::list_tickets(ticket_tids));
		panic_if_err_3ds(ctr::list_titles_on(MEDIATYPE_NAND, title_tids));
		panic_if_err_3ds(ctr::list_titles_on(MEDIATYPE_SD, title_tids));
		ctr::list_titles_on(MEDIATYPE_GAME_CARD, title_tids);

		std::copy_if(ticket_tids.begin(), ticket_tids.end(), std::back_inserter(unused_ticket_tids), [title_tids](const u64& tid) -> bool {
			return std::find(title_tids.begin(), title_tids.end(), tid) == title_tids.end();
		});
	});


	if(!unused_ticket_tids.size())
	{
		ui::notice(STRING(found_no_unused_tickets));
		return;
	}

	if(ui::Confirm::exec(PSTRING(found_unused_tickets, unused_ticket_tids.size()), STRING(delete_unused_tickets)))
	{
		ui::LoadingBar bar { unused_ticket_tids.size() };
		for(const u64& title_id : unused_ticket_tids)
		{
			dlog("deleting unused ticket with title id %016llX", title_id);
			panic_if_err_3ds(AM_DeleteTicket(title_id));
			bar.update();
		}
	}
}
