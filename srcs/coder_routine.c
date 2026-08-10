/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_routine.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/08 16:00:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/10 19:34:34 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	coder_should_exit(t_coder *c)
{
	if (!c || !c->sim)
		return (1);
	return (is_stopped(c->sim));
}

int	coder_loop(t_coder *c)
{
	while (!coder_should_exit(c))
	{
		if (compile_cycle(c))
			break ;
		if (coder_should_exit(c))
			break ;
		do_debug(c);
		if (coder_should_exit(c))
			break ;
		do_refactor(c);
	}
	return (0);
}

void	*coder_routine(void *arg)
{
	t_coder	*c;

	c = (t_coder *)arg;
	if (c)
		coder_loop(c);
	return (NULL);
}
