/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   util.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/04 17:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:35:23 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	print_error(const char *msg)
{
	if (msg)
		write(2, msg, strlen(msg));
	write(2, "\n", 1);
	return (1);
}

int	is_stopped(t_sim *sim)
{
	int	stopped;

	if (!sim)
		return (1);
	pthread_mutex_lock(&sim->stop_mtx);
	stopped = sim->stopped;
	pthread_mutex_unlock(&sim->stop_mtx);
	return (stopped);
}

void	set_stopped(t_sim *sim)
{
	int	i;

	if (!sim)
		return ;
	pthread_mutex_lock(&sim->stop_mtx);
	sim->stopped = 1;
	pthread_mutex_unlock(&sim->stop_mtx);
	i = 0;
	while (sim->dongles && i < sim->n_dongles)
	{
		pthread_mutex_lock(&sim->dongles[i].mtx);
		pthread_cond_broadcast(&sim->dongles[i].cv);
		pthread_mutex_unlock(&sim->dongles[i].mtx);
		i++;
	}
}

long long	get_deadline(t_coder *c)
{
	long long	base;

	if (!c || !c->sim)
		return (0);
	base = c->last_compile;
	if (!base)
		base = c->sim->start_ms;
	return (base + c->sim->cfg.t_burnout);
}

int	dongle_first(t_coder *c)
{
	if (!c || !c->left || !c->right)
		return (0);
	if (c->left->id <= c->right->id)
		return (0);
	return (1);
}
