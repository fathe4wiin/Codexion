/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/11 22:01:13 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	check_all_done(t_sim *sim)
{
	int	i;
	int	compiled;

	if (!sim || !sim->coders)
		return (0);
	i = 0;
	while (i < sim->cfg.n_coders)
	{
		pthread_mutex_lock(&sim->coders[i].state_mtx);
		compiled = sim->coders[i].n_compiled;
		pthread_mutex_unlock(&sim->coders[i].state_mtx);
		if (compiled < sim->cfg.n_compiles)
			return (0);
		i++;
	}
	return (1);
}

int	check_burnouts(t_sim *sim)
{
	int			i;
	long long	now;
	long long	base;

	if (!sim || !sim->coders)
		return (0);
	now = get_time_ms();
	i = 0;
	while (i < sim->cfg.n_coders)
	{
		pthread_mutex_lock(&(sim->coders[i].state_mtx));
		base = sim->coders[i].last_compile;
		pthread_mutex_unlock(&(sim->coders[i].state_mtx));
		if (!base)
			base = sim->start_ms;
		if (now - base >= sim->cfg.t_burnout)
		{
			set_stopped(sim);
			log_burnout(sim, sim->coders[i].id);
			return (1);
		}
		i++;
	}
	return (0);
}

void	*monitor_routine(void *arg)
{
	t_sim	*sim;

	sim = (t_sim *)arg;
	while (sim && !is_stopped(sim))
	{
		if (check_burnouts(sim))
			break ;
		if (check_all_done(sim))
		{
			set_stopped(sim);
			break ;
		}
		usleep(200);
	}
	return (NULL);
}
