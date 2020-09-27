select t.name, r.lavg
from Trainer t, 
     (select c.owner_id, avg(c.level) lavg
     from CatchedPokemon c, Pokemon p
     where p.id=c.pid and (p.type='Normal' or p.type='Electric')
     group by c.owner_id) r
where r.owner_id=t.id
order by r.lavg;