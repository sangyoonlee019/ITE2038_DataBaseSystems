select T.name
from (select t.name , count(*) as pcount
      from Trainer as t, CatchedPokemon  as c
      where t.id = c.owner_id 
      group by t.id) as T
where T.pcount >= 3 
order by T.pcount desc;