/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/13 18:40:00 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	destroy_shared(t_sim *sim)
{
	pthread_mutex_destroy(&sim->table_mtx);
	pthread_cond_destroy(&sim->table_cv);
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
	destroy_coders(sim);
	if (sim->start_ms)
	{
		destroy_shared(sim);
		sim->start_ms = 0;
	}
	free_sim(sim);
}
