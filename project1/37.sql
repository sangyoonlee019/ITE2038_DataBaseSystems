select t.name, r1.lsum
from Trainer t,
     (select c.owner_id, sum(c.level) lsum
     from CatchedPokemon c
     group by c.owner_id) r1,
     (select sum(c.level) lsum
     from CatchedPokemon c
     group by c.owner_id
     order by lsum desc limit 1) r2
where r1.lsum=r2.lsum and t.id=r1.owner_id
order by t.name;