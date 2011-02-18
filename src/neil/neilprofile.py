

import pstats
p = pstats.Stats('neilprofile')

p.strip_dirs()

#sort_stats(-1).print_stats()

p.sort_stats('time').print_stats()