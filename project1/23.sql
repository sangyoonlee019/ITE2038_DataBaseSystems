select distinct t.name
from Trainer t, CatchedPokemon c
where t.id=c.owner_id and c.level<=10
order by t.name;