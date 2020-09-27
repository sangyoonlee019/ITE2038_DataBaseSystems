select distinct t.name,count(*) ccount
from Trainer t,CatchedPokemon c,Gym g
where t.id=c.owner_id and g.leader_id=t.id
group by t.id
order by t.name;