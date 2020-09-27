select count(distinct c.pid)
from Trainer t,CatchedPokemon c
where t.id=c.owner_id and t.hometown='Sangnok city';