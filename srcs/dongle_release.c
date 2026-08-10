/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_release.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by student           #+#    #+#             */
/*   Updated: 2026/08/08 16:00:00 by student          ###   ########.fr       */
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
