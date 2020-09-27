select avg(c.level)
from CatchedPokemon c,Gym g
where c.owner_id=g.leader_id;