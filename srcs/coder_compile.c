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
	if (!c)
		return ;
	c->n_compiled++;
}

int	do_compile(t_coder *c)
{
	if (!c || !c->sim || is_stopped(c->sim))
		return (1);
	pthread_mutex_lock(&c->state_mtx);
	c->last_compile = get_time_ms();
	log_msg(c->sim, c->id, ST_COMPILE);
	pthread_mutex_unlock(&c->state_mtx);
	return (act_sleep(c->sim, c->sim->cfg.t_compile));
}

void	release_two_dongles(t_coder *c)
{
	if (!c)
		return ;
	if (c->left == c->right)
	{
		release_dongle(c->left, c);
		return ;
	}
	if (dongle_first(c) == 0)
	{
		release_dongle(c->right, c);
		release_dongle(c->left, c);
	}
	else
	{
		release_dongle(c->left, c);
		release_dongle(c->right, c);
	}
}

int	compile_cycle(t_coder *c)
{
	if (!c || coder_should_exit(c))
		return (1);
	if (take_two_dongles(c))
		return (1);
	if (do_compile(c))
	{
		release_two_dongles(c);
		return (1);
	}
	release_two_dongles(c);
	bump_compile(c);
	return (0);
}
