select avg(c.level)
from CatchedPokemon c,Trainer t
where c.owner_id=t.id and t.name='Red';