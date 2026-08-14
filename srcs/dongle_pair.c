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

/*
** A cooldown is a time event nobody can signal, and no earlier event can
** help us since we still need that same dongle, so we wait it out unlocked.
*/
void	table_wait(t_sim *sim, long long until)
{
	if (!until)
	{
		pthread_cond_wait(&sim->table_cv, &sim->table_mtx);
		return ;
	}
	pthread_mutex_unlock(&sim->table_mtx);
	act_sleep(sim, until - get_time_ms());
	pthread_mutex_lock(&sim->table_mtx);
}
