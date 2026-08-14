/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_dongles.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/14 18:20:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	init_one_dongle(t_dongle *d, int id, t_sched sched)
{
	d->id = id;
	d->holder = -1;
	d->ready_at = 0;
	heap_init(&d->queue, sched);
}

void	init_dongles(t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->n_dongles)
	{
		init_one_dongle(&sim->dongles[i], i, sim->cfg.sched);
		i++;
	}
}
