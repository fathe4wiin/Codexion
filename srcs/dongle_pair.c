/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_pair.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/12 21:20:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/12 21:30:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	claim_pair(t_dongle *a, t_dongle *b, t_coder *c)
{
	dequeue_waiter(a, c);
	dequeue_waiter(b, c);
	a->holder = c->id;
	b->holder = c->id;
}

void	leave_pair(t_dongle *a, t_dongle *b, t_coder *c)
{
	dequeue_waiter(a, c);
	dequeue_waiter(b, c);
}

/* 0 means nothing is cooling down, so only another thread can help us. */
long long	pair_wake_at(t_dongle *a, t_dongle *b)
{
	long long	until;

	until = a->ready_at;
	if (b->ready_at > until)
		until = b->ready_at;
	if (until <= get_time_ms())
		return (0);
	return (until);
}

void	table_wait(t_sim *sim, long long until)
{
	struct timespec	ts;

	if (!until)
	{
		pthread_cond_wait(&sim->table_cv, &sim->table_mtx);
		return ;
	}
	ts.tv_sec = (time_t)(until / 1000);
	ts.tv_nsec = (long)((until % 1000) * 1000000L);
	pthread_cond_timedwait(&sim->table_cv, &sim->table_mtx, &ts);
}
