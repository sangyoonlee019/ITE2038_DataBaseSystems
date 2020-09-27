select avg(c.level)
from Trainer t, CatchedPokemon c, Pokemon p
where c.owner_id = t.id and t.hometown = 'Sangnok City' and p.id = c.pid and p.type='Electric';