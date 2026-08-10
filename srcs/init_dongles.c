/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_dongles.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by student           #+#    #+#             */
/*   Updated: 2026/08/08 16:00:00 by student          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	rollback_dongles(t_sim *sim, int last)
{
	t_dongle	*d;

	while (last >= 0)
	{
		d = &sim->dongles[last];
		heap_destroy(&d->queue);
		pthread_mutex_destroy(&d->mtx);
		pthread_cond_destroy(&d->cv);
		d->sim = NULL;
		last--;
	}
}

int	init_one_dongle(t_dongle *d, int id, t_sim *sim)
{
	if (!d || !sim)
		return (1);
	d->id = id;
	d->holder = -1;
	d->ready_at = 0;
	if (heap_init(&d->queue, sim->cfg.n_coders, sim->cfg.sched))
		return (1);
	if (pthread_mutex_init(&d->mtx, NULL))
	{
		heap_destroy(&d->queue);
		return (1);
	}
	if (pthread_cond_init(&d->cv, NULL))
	{
		pthread_mutex_destroy(&d->mtx);
		heap_destroy(&d->queue);
		return (1);
	}
	d->sim = sim;
	return (0);
}

int	init_dongles(t_sim *sim)
{
	int	i;

	if (!sim || !sim->dongles)
		return (1);
	i = 0;
	while (i < sim->n_dongles)
	{
		if (init_one_dongle(&sim->dongles[i], i, sim))
		{
			rollback_dongles(sim, i - 1);
			return (1);
		}
		i++;
	}
	return (0);
}
