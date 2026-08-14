/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_wait.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/12 21:30:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/13 19:00:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	req_yields(t_req *r, t_sim *sim)
{
	long long	limit;

	if (!r->blocked)
		return (0);
	limit = (long long)sim->cfg.t_compile + sim->cfg.dongle_cd;
	return (get_time_ms() - r->arrival < limit);
}

int	priority_ok(t_dongle *d, t_coder *c)
{
	t_heap	*h;
	int		self;
	int		peer;

	h = &d->queue;
	self = heap_find(h, c->id);
	if (self < 0)
		return (0);
	if (h->size < 2)
		return (1);
	peer = 1 - self;
	if (req_yields(&h->data[peer], c->sim))
		return (1);
	return (!req_better(&h->data[peer], &h->data[self], h->sched));
}

int	usable_dongle(t_dongle *d, t_coder *c)
{
	if (d->holder >= 0 || !dongle_ready(d))
		return (0);
	return (priority_ok(d, c));
}

/* 0 claimed the pair, 2 changed our flags (peers need waking), 1 idle retry. */
int	claim_or_mark(t_dongle *a, t_dongle *b, t_coder *c)
{
	int	ua;
	int	ub;
	int	moved;

	ua = usable_dongle(a, c);
	ub = usable_dongle(b, c);
	if (ua && ub)
	{
		claim_pair(a, b, c);
		return (0);
	}
	moved = waiter_set_blocked(a, c, !ub);
	moved += waiter_set_blocked(b, c, !ua);
	if (moved)
		return (2);
	return (1);
}
