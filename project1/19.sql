select count(distinct p.type)
from Trainer t,CatchedPokemon c,Gym g,Pokemon p
where t.id=c.owner_id and g.leader_id=t.id and t.hometown='Sangnok city' and p.id=c.pid;