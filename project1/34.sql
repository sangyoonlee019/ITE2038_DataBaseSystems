select p.name, c.level, c.nickname
from Gym g,Trainer t,CatchedPokemon c,Pokemon p
where g.leader_id=t.id and c.owner_id=t.id and p.id=c.pid and c.nickname like 'A%'
order by p.name desc;