select distinct p.name, p.type
from Pokemon p, CatchedPokemon c
where c.pid=p.id and c.level>=30
order by p.name;