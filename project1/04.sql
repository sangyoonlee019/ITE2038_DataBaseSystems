select c.nickname
from CatchedPokemon as c
where c.level >= 50
order by c.nickname;