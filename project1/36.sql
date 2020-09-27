select t.name
from CatchedPokemon c, Trainer t,
     (select e.after_id candidate
      from Evolution e) r1
where r1.candidate not in (select e.before_id
      from Evolution e) and t.id=c.owner_id and c.pid=r1.candidate
order by t.name;