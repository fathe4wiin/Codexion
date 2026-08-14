/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_compile.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/11 21:55:09 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	bump_compile(t_coder *c)
{
	pthread_mutex_lock(&c->state_mtx);
	c->n_compiled++;
	pthread_mutex_unlock(&c->state_mtx);
}

int	do_compile(t_coder *c)
{
	if (is_stopped(c->sim))
		return (1);
	pthread_mutex_lock(&c->state_mtx);
	c->last_compile = get_time_ms();
	pthread_mutex_unlock(&c->state_mtx);
	log_msg(c->sim, c->id, ST_COMPILE);
	return (act_sleep(c->sim, c->sim->cfg.t_compile));
}

int	compile_cycle(t_coder *c)
{
	int	st;

	if (!c || coder_should_exit(c))
		return (1);
	if (take_two_dongles(c))
		return (1);
	st = do_compile(c);
	release_two_dongles(c);
	if (st)
		return (1);
	bump_compile(c);
	return (0);
}
