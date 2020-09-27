select t.name, r.lmax
from Trainer t, 
     (select c.owner_id, count(*) ccount, max(c.level) lmax
     from CatchedPokemon c
     group by c.owner_id) r
where r.owner_id=t.id and r.ccount>=4
order by t.name;