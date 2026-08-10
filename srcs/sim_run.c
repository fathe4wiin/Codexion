/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_run.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by student           #+#    #+#             */
/*   Updated: 2026/08/08 16:00:00 by student          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	spawn_monitor(t_sim *sim)
{
	if (!sim)
		return (1);
	if (pthread_create(&sim->monitor, NULL, monitor_routine, sim))
		return (1);
	return (0);
}

int	spawn_coders(t_sim *sim)
{
	int	i;

	if (!sim || !sim->coders)
		return (1);
	i = 0;
	while (i < sim->cfg.n_coders)
	{
		if (pthread_create(&sim->coders[i].thread, NULL,
				coder_routine, &sim->coders[i]))
		{
			set_stopped(sim);
			return (1);
		}
		i++;
	}
	return (0);
}

int	join_all(t_sim *sim)
{
	int	i;

	if (!sim)
		return (1);
	i = 0;
	while (i < sim->cfg.n_coders)
	{
		pthread_join(sim->coders[i].thread, NULL);
		i++;
	}
	pthread_join(sim->monitor, NULL);
	return (0);
}

int	start_simulation(t_sim *sim)
{
	if (!sim)
		return (1);
	stamp_start(sim);
	if (spawn_monitor(sim) || spawn_coders(sim))
	{
		set_stopped(sim);
		return (1);
	}
	join_all(sim);
	return (0);
}
