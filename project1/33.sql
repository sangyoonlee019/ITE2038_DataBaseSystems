select sum(c.level)
from Trainer t, CatchedPokemon c
where t.id=c.owner_id and t.name='Matis';