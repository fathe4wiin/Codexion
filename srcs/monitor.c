/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: student <student@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by student           #+#    #+#             */
/*   Updated: 2026/08/08 16:00:00 by student          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	check_all_done(t_sim *sim)
{
	int	i;

	if (!sim || !sim->coders)
		return (0);
	i = 0;
	while (i < sim->cfg.n_coders)
	{
		if (sim->coders[i].n_compiled < sim->cfg.n_compiles)
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
		base = sim->coders[i].last_compile;
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
		usleep(500);
	}
	return (NULL);
}
