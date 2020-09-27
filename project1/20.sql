select distinct t.name,count(*) ccount
from Trainer t,CatchedPokemon c
where t.id=c.owner_id and t.hometown='Sangnok city'
group by t.id
order by ccount;