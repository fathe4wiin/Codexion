/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_release.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:34:41 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	arm_cooldown(t_dongle *d)
{
	if (!d || !d->sim)
		return ;
	d->ready_at = get_time_ms() + d->sim->cfg.dongle_cd;
}

void	signal_waiter(t_dongle *d)
{
	if (!d)
		return ;
	pthread_cond_broadcast(&d->cv);
}

void	grant_next(t_dongle *d)
{
	t_req	next;

	if (!d || d->queue.size <= 0)
		return ;
	if (!dongle_ready(d))
	{
		signal_waiter(d);
		return ;
	}
	if (heap_peek(&d->queue, &next))
		return ;
	if (heap_pop(&d->queue, &next))
		return ;
	d->holder = next.coder_id;
	signal_waiter(d);
}

void	release_dongle(t_dongle *d, t_coder *c)
{
	if (!d || !c)
		return ;
	pthread_mutex_lock(&d->mtx);
	if (d->holder == c->id)
	{
		d->holder = -1;
		arm_cooldown(d);
		grant_next(d);
	}
	pthread_mutex_unlock(&d->mtx);
}
