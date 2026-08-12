/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_wait.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/12 21:30:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/12 21:30:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/*
** A request that waited longer than one dongle cycle stops letting others
** pass, which bounds how many turns a blocked request can lose.
*/
int	req_stale(t_req *r, t_sim *sim)
{
	long long	limit;

	limit = (long long)sim->cfg.t_compile + sim->cfg.dongle_cd;
	return (get_time_ms() - r->arrival >= limit);
}

int	req_yields(t_req *r, t_sim *sim)
{
	if (!r->blocked)
		return (0);
	return (!req_stale(r, sim));
}

/*
** Both dongles are locked here, so the pair is taken atomically or not at all.
** When it fails, each queue entry records whether the other dongle blocks us,
** which lets coders that do not compete for that dongle move ahead.
*/
int	claim_or_mark(t_dongle *a, t_dongle *b, t_coder *c)
{
	int	ua;
	int	ub;

	ua = usable_dongle(a, c);
	ub = usable_dongle(b, c);
	if (ua && ub)
	{
		claim_pair(a, b, c);
		return (0);
	}
	heap_set_blocked(&a->queue, c->id, !ub);
	heap_set_blocked(&b->queue, c->id, !ua);
	return (1);
}

void	wait_pair_tick(t_dongle *a, t_dongle *b)
{
	t_dongle		*p;
	struct timespec	ts;
	long long		until;

	p = a;
	if (b->id < a->id)
		p = b;
	until = get_time_ms() + 1;
	ts.tv_sec = (time_t)(until / 1000);
	ts.tv_nsec = (long)((until % 1000) * 1000000L);
	pthread_mutex_lock(&p->mtx);
	pthread_cond_timedwait(&p->cv, &p->mtx, &ts);
	pthread_mutex_unlock(&p->mtx);
}
