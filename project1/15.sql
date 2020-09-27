select r2.owner_id, r2.ccount
from (select count(*) ccount
      from CatchedPokemon c
      group by c.owner_id
      order by ccount desc limit 1) r,
      (select c.owner_id, count(*) ccount
      from CatchedPokemon c
      group by c.owner_id) r2
where r2.ccount=r.ccount