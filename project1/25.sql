select p.name
from Pokemon p,
     (select distinct c.pid
     from CatchedPokemon c, Trainer t
     where c.owner_id=t.id and t.hometown='Sangnok city') r1,
     (select distinct c.pid
     from CatchedPokemon c, Trainer t
     where c.owner_id=t.id and t.hometown='Brown city') r2
where p.id=r1.pid and p.id=r2.pid
order by p.name;