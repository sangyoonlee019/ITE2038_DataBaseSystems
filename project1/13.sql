select p.name, p.id
from Pokemon p, CatchedPokemon c, Trainer t
where t.hometown='Sangnok city' and t.id=c.owner_id and p.id=c.pid
order by p.id;