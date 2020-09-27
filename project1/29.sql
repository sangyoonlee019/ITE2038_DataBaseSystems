select p.type, count(*)
from CatchedPokemon c, Pokemon p
where p.id=c.pid
group by p.type
order by p.type;