select t.name, avg(c.level)
from Trainer t, CatchedPokemon c, Gym g
where t.id=g.leader_id and c.owner_id=t.id
group by g.leader_id
order by t.name;