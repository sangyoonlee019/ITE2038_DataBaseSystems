select c.nickname
from Trainer t, CatchedPokemon c
where c.owner_id=t.id and c.level>=50 and c.owner_id>=6
order by c.nickname;