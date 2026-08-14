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

	pthread_mutex_lock(&sim->table_mtx);
	stopped = sim->stopped;
	pthread_mutex_unlock(&sim->table_mtx);
	return (stopped);
}

void	set_stopped(t_sim *sim)
{
	pthread_mutex_lock(&sim->table_mtx);
	sim->stopped = 1;
	pthread_cond_broadcast(&sim->table_cv);
	pthread_mutex_unlock(&sim->table_mtx);
}

long long	get_deadline(t_coder *c)
{
	long long	base;

	base = c->last_compile;
	if (!base)
		base = c->sim->start_ms;
	return (base + c->sim->cfg.t_burnout);
}
