select h.name, avg(c.level) lavg
from Trainer t, CatchedPokemon c, City h
where t.id=c.owner_id and h.name=t.hometown
group by h.name
order by lavg;