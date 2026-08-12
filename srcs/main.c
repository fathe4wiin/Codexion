/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bfathi <bfathi@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/06 18:42:00 by bfathi            #+#    #+#             */
/*   Updated: 2026/08/12 03:12:13 by bfathi           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	print_usage(void)
{
	fprintf(stderr, "Error: Invalid syntax or incorrect arguments.\n\n");
	fprintf(stderr, "Usage: ./codexion <coders> <t_burn> <t_comp> <t_debug> ");
	fprintf(stderr, "<t_refact> <req_comps> <cooldown> <sched>\n\n");
	fprintf(stderr, "Arguments constraints:\n");
	fprintf(stderr, "  1. coders      : Integer (> 0)\n");
	fprintf(stderr, "  2. t_burn      : Integer (in ms)\n");
	fprintf(stderr, "  3. t_comp      : Integer (in ms)\n");
	fprintf(stderr, "  4. t_debug     : Integer (in ms)\n");
	fprintf(stderr, "  5. t_refact    : Integer (in ms)\n");
	fprintf(stderr, "  6. req_comps   : Integer (> 0)\n");
	fprintf(stderr, "  7. cooldown    : Integer (in ms)\n");
	fprintf(stderr, "  8. sched       : 'fifo' or 'edf'\n\n");
	fprintf(stderr, "Example: ./codexion 5 800 200 200 100 5 10 edf\n");
}

int	main(int ac, char **av)
{
	t_sim		sim;
	t_config	cfg;

	if (parse_args(ac, av, &cfg))
	{
		print_usage();
		return (1);
	}
	if (init_sim(&sim, &cfg))
		return (1);
	start_simulation(&sim);
	cleanup_sim(&sim);
	return (0);
}
