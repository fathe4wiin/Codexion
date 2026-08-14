/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init_sim.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:35:23 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	init_sim(t_sim *sim, t_config *cfg)
{
	if (!sim || !cfg)
		return (1);
	memset(sim, 0, sizeof(*sim));
	sim->cfg = *cfg;
	sim->n_dongles = cfg->n_coders;
	if (alloc_sim(sim))
		return (1);
	init_dongles(sim);
	if (init_shared(sim) || init_coders(sim))
	{
		cleanup_sim(sim);
		return (1);
	}
	return (0);
}

int	alloc_sim(t_sim *sim)
{
	if (!sim)
		return (1);
	sim->coders = malloc(sizeof(t_coder) * (size_t)sim->cfg.n_coders);
	if (!sim->coders)
		return (1);
	sim->dongles = malloc(sizeof(t_dongle) * (size_t)sim->n_dongles);
	if (!sim->dongles)
	{
		free(sim->coders);
		sim->coders = NULL;
		return (1);
	}
	memset(sim->coders, 0, sizeof(t_coder) * (size_t)sim->cfg.n_coders);
	memset(sim->dongles, 0, sizeof(t_dongle) * (size_t)sim->n_dongles);
	return (0);
}

int	init_shared(t_sim *sim)
{
	if (pthread_mutex_init(&sim->table_mtx, NULL))
		return (1);
	if (pthread_cond_init(&sim->table_cv, NULL))
	{
		pthread_mutex_destroy(&sim->table_mtx);
		return (1);
	}
	if (pthread_mutex_init(&sim->log_mtx, NULL))
	{
		pthread_cond_destroy(&sim->table_cv);
		pthread_mutex_destroy(&sim->table_mtx);
		return (1);
	}
	sim->start_ms = 1;
	return (0);
}

void	stamp_start(t_sim *sim)
{
	if (!sim)
		return ;
	sim->start_ms = get_time_ms();
}
