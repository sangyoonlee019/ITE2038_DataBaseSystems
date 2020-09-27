select p.name
from (select p.type, count(*) as tcount
      from Pokemon as p
      group by p.type) as r1,
     (select T.tcount
      from (select p.type, count(*) as tcount
            from Pokemon as p
            group by p.type) as T
      order by T.tcount desc limit 2) as r2,
      Pokemon as p
where r1.tcount=r2.tcount and r1.type=p.type
order by p.name;