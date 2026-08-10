/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:34:11 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	destroy_dongles(t_sim *sim)
{
	int			i;
	t_dongle	*d;

	if (!sim || !sim->dongles)
		return ;
	i = 0;
	while (i < sim->n_dongles)
	{
		d = &sim->dongles[i];
		if (d->sim)
		{
			heap_destroy(&d->queue);
			pthread_mutex_destroy(&d->mtx);
			pthread_cond_destroy(&d->cv);
			d->sim = NULL;
		}
		i++;
	}
}

void	destroy_shared(t_sim *sim)
{
	if (!sim)
		return ;
	pthread_mutex_destroy(&sim->stop_mtx);
	pthread_mutex_destroy(&sim->log_mtx);
}

void	free_sim(t_sim *sim)
{
	if (!sim)
		return ;
	free(sim->coders);
	free(sim->dongles);
	sim->coders = NULL;
	sim->dongles = NULL;
}

void	cleanup_sim(t_sim *sim)
{
	if (!sim)
		return ;
	destroy_dongles(sim);
	if (sim->start_ms)
	{
		destroy_shared(sim);
		sim->start_ms = 0;
	}
	free_sim(sim);
}
