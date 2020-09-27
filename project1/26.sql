select p.name
from Pokemon p, CatchedPokemon c
where c.nickname like '% %' and c.pid=p.id
order by p.name desc;