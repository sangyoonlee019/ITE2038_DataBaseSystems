select r1.hometown,r1.nickname
from (select t.hometown, c.nickname, c.level
	from Trainer t, CatchedPokemon  c
	where t.id=c.owner_id) r1,
	(select t.hometown, max(c.level) lmax
	from Trainer t, CatchedPokemon  c
	where t.id=c.owner_id
	group by t.hometown) r2
where r1.hometown=r2.hometown and r2.lmax=r1.level
order by r1.hometown;