select p.name
from Pokemon p
where p.id not in (select c.pid
                   from CatchedPokemon c)
order by p.name;